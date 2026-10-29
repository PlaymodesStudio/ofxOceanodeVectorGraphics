#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace twoBodyRepulsionCore {

// Per-Body controls. Each maps to that Body's points, excluding -1 path separators;
// one value broadcasts and shorter vectors repeat their last value.
struct BodyControls {
    std::vector<float> radius{0.0f};         // Disk radius around each point, coordinate units.
    std::vector<float> strength{1.0f};       // Multiplier of the push this Body exerts on the other.
    std::vector<float> fear{0.15f};          // Surface distance at which this Body starts fleeing.
    std::vector<float> repulsion{40.0f};     // Acceleration per fear radius, seconds^-2.
    std::vector<float> returnStrength{8.0f}; // Unit-mass spring stiffness, seconds^-2.
    std::vector<float> damping{3.0f};        // Velocity decay rate, seconds^-1.
};

// Wall at the 0-1 canvas edge. Inactive while margin and feather are both zero.
struct Border {
    double margin = 0.0;  // Hard limit measured from the edge to each disk's surface.
    double feather = 0.0; // Width of the soft push inside the margin.
};

// Coordinates use the addon's paired -1 path separator protocol. Both Bodies share one clock.
class Simulation {
public:
    void clear() { a.clear(); b.clear(); remainder = 0.0; }

    void update(const std::vector<float>& aX, const std::vector<float>& aY, const BodyControls& aControls,
                const std::vector<float>& bX, const std::vector<float>& bY, const BodyControls& bControls,
                const Border& border, double seconds, bool reset = false) {
        a.begin(aX, aY, aControls, reset);
        b.begin(bX, bY, bControls, reset);
        // A reset publishes the exact rest positions. Long host stalls drop excess time.
        if(reset) remainder = 0.0;
        else if(std::isfinite(seconds) && seconds > 0.0) remainder += std::min(seconds, 0.1);
        const int steps = static_cast<int>(std::floor((remainder + 1e-12) / step));
        remainder = std::max(0.0, remainder - steps * step);
        const Wall wall = makeWall(border);
        for(int tick = 0; tick < steps; ++tick) {
            a.accumulate(wall);
            b.accumulate(wall);
            interact();
            a.integrate(wall);
            b.integrate(wall);
        }
        a.publish(); b.publish();
    }

    const std::vector<float>& aX() const { return a.outputX; }
    const std::vector<float>& aY() const { return a.outputY; }
    const std::vector<float>& bX() const { return b.outputX; }
    const std::vector<float>& bY() const { return b.outputY; }

private:
    static constexpr double step = 1.0 / 240.0;
    struct Particle { double x, y, vx, vy, ax, ay; bool separator; };
    struct Response {
        double radius, strength, fear, repulsion, returnStrength, decay, accelerationTime;
        bool responds; // Fear and Repulsion Strength both positive.
    };
    struct Wall { bool active; double margin, feather; };

    struct Body {
        std::vector<Particle> particles;
        std::vector<Response> responses;
        std::vector<float> outputX, outputY;
        const std::vector<float>* restX = nullptr;
        const std::vector<float>* restY = nullptr;
        bool active = false, anyResponds = false;
        double maxRadius = 0.0, maxFear = 0.0, maxStrength = 0.0;
        double minX = 0.0, maxX = 0.0, minY = 0.0, maxY = 0.0;

        void clear() {
            particles.clear(); responses.clear();
            outputX.clear(); outputY.clear();
            restX = restY = nullptr;
            active = false;
        }

        void begin(const std::vector<float>& x, const std::vector<float>& y,
                   const BodyControls& controls, bool reset) {
            if(!validBody(x, y)) { clear(); return; }
            active = true;
            restX = &x; restY = &y;
            // Identity is the input index. Changing count or separator layout resets this Body only.
            bool topologyChanged = particles.size() != x.size();
            if(!topologyChanged) {
                for(size_t i = 0; i < particles.size(); ++i)
                    if(particles[i].separator != (x[i] == -1.0f)) { topologyChanged = true; break; }
            }
            if(reset || topologyChanged) {
                particles.resize(x.size());
                for(size_t i = 0; i < particles.size(); ++i)
                    particles[i] = {x[i], y[i], 0.0, 0.0, 0.0, 0.0, x[i] == -1.0f};
            }
            // Sample controls once per host update, excluding path separators. Drag terms
            // are precomputed per point, avoiding repeated exponentials inside the substeps.
            responses.resize(particles.size());
            anyResponds = false;
            maxRadius = maxFear = maxStrength = 0.0;
            size_t pointIndex = 0;
            for(size_t i = 0; i < particles.size(); ++i) {
                if(particles[i].separator) continue;
                auto& response = responses[i];
                constexpr double unbounded = std::numeric_limits<float>::max();
                response.radius = sample(controls.radius, pointIndex, unbounded);
                response.strength = sample(controls.strength, pointIndex, unbounded);
                response.fear = sample(controls.fear, pointIndex, unbounded);
                response.repulsion = sample(controls.repulsion, pointIndex, 1000.0);
                response.returnStrength = sample(controls.returnStrength, pointIndex, 1000.0);
                const double damping = sample(controls.damping, pointIndex, 100.0);
                response.decay = std::exp(-damping * step);
                // Exact velocity integration of drag under constant acceleration per substep.
                response.accelerationTime = damping > 1e-8 ? -std::expm1(-damping * step) / damping : step;
                response.responds = response.fear > 0.0 && response.repulsion > 0.0;
                anyResponds = anyResponds || response.responds;
                maxRadius = std::max(maxRadius, response.radius);
                maxStrength = std::max(maxStrength, response.strength);
                if(response.responds) maxFear = std::max(maxFear, response.fear);
                ++pointIndex;
            }
        }

        // Spring toward the moving rest position plus the border's soft push.
        void accumulate(const Wall& wall) {
            if(!active) return;
            for(size_t i = 0; i < particles.size(); ++i) {
                auto& p = particles[i];
                if(p.separator) continue;
                const auto& r = responses[i];
                p.ax = r.returnStrength * ((*restX)[i] - p.x);
                p.ay = r.returnStrength * ((*restY)[i] - p.y);
                if(!wall.active || wall.feather <= 0.0 || r.repulsion <= 0.0) continue;
                double low, high;
                limits(wall, r.radius, low, high);
                p.ax += wallPush(p.x, low, high, wall.feather, r.repulsion);
                p.ay += wallPush(p.y, low, high, wall.feather, r.repulsion);
            }
        }

        void integrate(const Wall& wall) {
            if(!active) return;
            for(size_t i = 0; i < particles.size(); ++i) {
                auto& p = particles[i];
                if(p.separator) continue;
                const auto& r = responses[i];
                p.vx = p.vx * r.decay + p.ax * r.accelerationTime;
                p.vy = p.vy * r.decay + p.ay * r.accelerationTime;
                p.x += p.vx * step; p.y += p.vy * step;
                if(wall.active) {
                    double low, high;
                    limits(wall, r.radius, low, high);
                    if(p.x < low) { p.x = low; p.vx = std::max(p.vx, 0.0); }
                    else if(p.x > high) { p.x = high; p.vx = std::min(p.vx, 0.0); }
                    if(p.y < low) { p.y = low; p.vy = std::max(p.vy, 0.0); }
                    else if(p.y > high) { p.y = high; p.vy = std::min(p.vy, 0.0); }
                }
                constexpr double limit = std::numeric_limits<float>::max();
                if(!std::isfinite(p.x) || !std::isfinite(p.y) ||
                   !std::isfinite(p.vx) || !std::isfinite(p.vy) ||
                   std::abs(p.x) > limit || std::abs(p.y) > limit) {
                    p.x = (*restX)[i]; p.y = (*restY)[i]; p.vx = p.vy = 0.0;
                }
            }
        }

        void bounds() {
            minX = minY = std::numeric_limits<double>::infinity();
            maxX = maxY = -std::numeric_limits<double>::infinity();
            for(const auto& p : particles) {
                if(p.separator) continue;
                minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
                minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
            }
        }

        void publish() {
            outputX.resize(particles.size()); outputY.resize(particles.size());
            for(size_t i = 0; i < particles.size(); ++i) {
                outputX[i] = particles[i].separator ? -1.0f : static_cast<float>(particles[i].x);
                outputY[i] = particles[i].separator ? -1.0f : static_cast<float>(particles[i].y);
            }
        }
    };

    Body a, b;
    double remainder = 0.0;

    // Every A/B pair is evaluated once per substep and pushes both sides. Far apart Bodies
    // skip the pair loop through their bounding boxes; near pairs skip the square root.
    void interact() {
        if(!a.active || !b.active) return;
        const bool aMoves = a.anyResponds && b.maxStrength > 0.0;
        const bool bMoves = b.anyResponds && a.maxStrength > 0.0;
        if(!aMoves && !bMoves) return;
        a.bounds(); b.bounds();
        const double reach = a.maxRadius + b.maxRadius +
                             std::max(aMoves ? a.maxFear : 0.0, bMoves ? b.maxFear : 0.0);
        if(a.minX - reach > b.maxX || b.minX - reach > a.maxX ||
           a.minY - reach > b.maxY || b.minY - reach > a.maxY) return;
        for(size_t i = 0; i < a.particles.size(); ++i) {
            auto& pi = a.particles[i];
            if(pi.separator) continue;
            const auto& ri = a.responses[i];
            const double fearI = aMoves && ri.responds ? ri.fear : 0.0;
            const double reachI = ri.radius + b.maxRadius + std::max(fearI, bMoves ? b.maxFear : 0.0);
            if(pi.x - reachI > b.maxX || pi.x + reachI < b.minX ||
               pi.y - reachI > b.maxY || pi.y + reachI < b.minY) continue;
            for(size_t j = 0; j < b.particles.size(); ++j) {
                auto& pj = b.particles[j];
                if(pj.separator) continue;
                const auto& rj = b.responses[j];
                const double fearJ = bMoves && rj.responds ? rj.fear : 0.0;
                const double contact = ri.radius + rj.radius;
                const double cutoff = contact + std::max(fearI, fearJ);
                double dx = pi.x - pj.x, dy = pi.y - pj.y;
                const double squared = dx * dx + dy * dy;
                if(squared >= cutoff * cutoff) continue;
                const double distance = std::sqrt(squared);
                if(distance > 1e-12 * cutoff) {
                    dx /= distance; dy /= distance;
                } else {
                    // No direction exists at exact overlap: deterministic symmetry break from
                    // A's rest point. B takes the opposite direction, so coincident Bodies part.
                    const double angle = std::fmod((*a.restX)[i] * 12.9898 + (*a.restY)[i] * 78.233,
                                                   6.283185307179586);
                    dx = std::cos(angle); dy = std::sin(angle);
                }
                const double surface = std::max(0.0, distance - contact);
                if(surface < fearI && rj.strength > 0.0) {
                    const double falloff = 1.0 - surface / fearI;
                    const double magnitude = ri.repulsion * rj.strength * fearI * falloff * falloff;
                    pi.ax += dx * magnitude; pi.ay += dy * magnitude;
                }
                if(surface < fearJ && ri.strength > 0.0) {
                    const double falloff = 1.0 - surface / fearJ;
                    const double magnitude = rj.repulsion * ri.strength * fearJ * falloff * falloff;
                    pj.ax -= dx * magnitude; pj.ay -= dy * magnitude;
                }
            }
        }
    }

    static Wall makeWall(const Border& border) {
        Wall wall;
        wall.margin = std::isfinite(border.margin) ? std::max(0.0, border.margin) : 0.0;
        wall.feather = std::isfinite(border.feather) ? std::max(0.0, border.feather) : 0.0;
        wall.active = wall.margin > 0.0 || wall.feather > 0.0;
        return wall;
    }
    // Allowed range of a disk centre. An oversized disk is held at the canvas centre.
    static void limits(const Wall& wall, double radius, double& low, double& high) {
        low = wall.margin + radius; high = 1.0 - wall.margin - radius;
        if(low > high) low = high = 0.5;
    }
    // Squared falloff over the feather width, at full strength past the margin.
    static double wallPush(double value, double low, double high, double feather, double repulsion) {
        double push = 0.0;
        const double fromLow = value - low, fromHigh = high - value;
        if(fromLow < feather) {
            const double falloff = 1.0 - std::max(fromLow, 0.0) / feather;
            push += repulsion * feather * falloff * falloff;
        }
        if(fromHigh < feather) {
            const double falloff = 1.0 - std::max(fromHigh, 0.0) / feather;
            push -= repulsion * feather * falloff * falloff;
        }
        return push;
    }
    static double bounded(double value, double maximum) {
        return std::isfinite(value) ? std::max(0.0, std::min(value, maximum)) : 0.0;
    }
    static double sample(const std::vector<float>& values, size_t index, double maximum) {
        return values.empty() ? 0.0 : bounded(values[std::min(index, values.size()-1)], maximum);
    }
    static bool validBody(const std::vector<float>& x, const std::vector<float>& y) {
        if(x.size() != y.size()) return false;
        for(size_t i = 0; i < x.size(); ++i)
            if(!std::isfinite(x[i]) || !std::isfinite(y[i]) ||
               ((x[i] == -1.0f) != (y[i] == -1.0f))) return false;
        return true;
    }
};

} // namespace twoBodyRepulsionCore
