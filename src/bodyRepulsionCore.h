#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace bodyRepulsionCore {

struct Settings {
    std::vector<float> repulsion{40.0f}; // Per-Body acceleration per fear radius, seconds^-2.
    std::vector<float> returnStrength{8.0f}; // Per-Body unit-mass spring stiffness, seconds^-2.
    std::vector<float> damping{3.0f}; // Per-Body velocity decay rate, seconds^-1.
};

// Coordinates use the addon's paired -1 path separator protocol.
class Simulation {
public:
    void clear() {
        particles.clear();
        outputX.clear(); outputY.clear();
        remainder = 0.0;
    }

    void update(const std::vector<float>& bodyX, const std::vector<float>& bodyY,
                const std::vector<float>& forceX, const std::vector<float>& forceY,
                const std::vector<float>& fear, const std::vector<float>& strength,
                const Settings& settings, double seconds,
                bool reset = false) {
        if(!validBody(bodyX, bodyY)) { clear(); return; }
        // Identity is the input index. Changing count or separator layout resets identity.
        bool topologyChanged = particles.size() != bodyX.size();
        if(!topologyChanged) {
            for(size_t i = 0; i < particles.size(); ++i)
                if(particles[i].separator != (bodyX[i] == -1.0f)) { topologyChanged = true; break; }
        }
        if(reset || topologyChanged) {
            particles.resize(bodyX.size());
            for(size_t i = 0; i < particles.size(); ++i)
                particles[i] = {bodyX[i], bodyY[i], 0.0, 0.0, bodyX[i] == -1.0f};
            remainder = 0.0;
        }

        // Mismatched force arrays have no repulsive contribution, but inertia continues.
        std::vector<Repeller> forces;
        if(forceX.size() == forceY.size()) {
            forces.reserve(forceX.size());
            size_t forceIndex = 0;
            for(size_t j = 0; j < forceX.size(); ++j) {
                if(forceX[j] == -1.0f && forceY[j] == -1.0f) continue;
                // Invalid coordinate entries retain their strength slot; separators do not.
                const double multiplier = sample(strength, forceIndex, std::numeric_limits<float>::max());
                ++forceIndex;
                if(std::isfinite(forceX[j]) && std::isfinite(forceY[j]) &&
                   forceX[j] != -1.0f && forceY[j] != -1.0f && multiplier > 0.0)
                    forces.push_back({forceX[j], forceY[j], multiplier});
            }
        }

        // A reset publishes the exact rest positions. Long host stalls drop excess time.
        if(!reset && std::isfinite(seconds) && seconds > 0.0)
            remainder += std::min(seconds, 0.1);
        constexpr double step = 1.0 / 240.0;
        const int steps = static_cast<int>(std::floor((remainder + 1e-12) / step));
        remainder = std::max(0.0, remainder - steps * step);
        // Sample Body controls once per host update, excluding path separators. Drag terms
        // are precomputed per point, avoiding repeated exponentials inside the substeps.
        std::vector<Response> responses(particles.size());
        size_t pointIndex = 0;
        for(size_t i = 0; i < particles.size(); ++i) {
            if(particles[i].separator) continue;
            auto& response = responses[i];
            response.radius = sample(fear, pointIndex, std::numeric_limits<float>::max());
            response.repulsion = sample(settings.repulsion, pointIndex, 1000.0);
            response.returnStrength = sample(settings.returnStrength, pointIndex, 1000.0);
            const double damping = sample(settings.damping, pointIndex, 100.0);
            response.decay = std::exp(-damping * step);
            // Exact velocity integration of drag under constant acceleration per substep.
            response.accelerationTime = damping > 1e-8 ? -std::expm1(-damping * step) / damping : step;
            ++pointIndex;
        }
        for(int tick = 0; tick < steps; ++tick) {
            for(size_t i = 0; i < particles.size(); ++i) {
                auto& p = particles[i];
                if(p.separator) continue;
                const auto& response = responses[i];
                const double radius = response.radius;
                double ax = response.returnStrength * (bodyX[i] - p.x);
                double ay = response.returnStrength * (bodyY[i] - p.y);
                if(radius > 0.0 && response.repulsion > 0.0) {
                    for(const auto& force : forces) {
                        double dx = p.x - force.x, dy = p.y - force.y;
                        const double distance = std::hypot(dx, dy);
                        if(distance >= radius) continue;
                        if(distance > 1e-12 * radius) {
                            dx /= distance; dy /= distance;
                        } else {
                            // No direction exists at exact overlap: deterministic symmetry break.
                            // Identical rest points (including closed-path duplicates) stay together.
                            const double angle = std::fmod(bodyX[i] * 12.9898 + bodyY[i] * 78.233,
                                                          6.283185307179586);
                            dx = std::cos(angle); dy = std::sin(angle);
                        }
                        const double falloff = 1.0 - distance / radius;
                        const double magnitude = response.repulsion * force.strength * radius * falloff * falloff;
                        ax += dx * magnitude; ay += dy * magnitude;
                    }
                }
                p.vx = p.vx * response.decay + ax * response.accelerationTime;
                p.vy = p.vy * response.decay + ay * response.accelerationTime;
                p.x += p.vx * step; p.y += p.vy * step;
                const double limit = std::numeric_limits<float>::max();
                if(!std::isfinite(p.x) || !std::isfinite(p.y) ||
                   !std::isfinite(p.vx) || !std::isfinite(p.vy) ||
                   std::abs(p.x) > limit || std::abs(p.y) > limit) {
                    p.x = bodyX[i]; p.y = bodyY[i]; p.vx = p.vy = 0.0;
                }
            }
        }
        outputX.resize(particles.size()); outputY.resize(particles.size());
        for(size_t i = 0; i < particles.size(); ++i) {
            outputX[i] = particles[i].separator ? -1.0f : static_cast<float>(particles[i].x);
            outputY[i] = particles[i].separator ? -1.0f : static_cast<float>(particles[i].y);
        }
    }

    const std::vector<float>& x() const { return outputX; }
    const std::vector<float>& y() const { return outputY; }

private:
    struct Repeller { double x, y, strength; };
    struct Response { double radius, repulsion, returnStrength, decay, accelerationTime; };
    struct Particle { double x, y, vx, vy; bool separator; };
    std::vector<Particle> particles;
    std::vector<float> outputX, outputY;
    double remainder = 0.0;

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

} // namespace bodyRepulsionCore
