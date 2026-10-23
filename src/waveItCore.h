#pragma once

#include <algorithm>
#include <cmath>
#include <vector>
#include <limits>

namespace waveItCore {
struct Point { double x, y; };
struct Result { std::vector<float> x, y; };

inline Point direction(Point a, Point b) {
    const double length = std::hypot(b.x - a.x, b.y - a.y);
    return length > 0 ? Point{(b.x - a.x) / length, (b.y - a.y) / length} : Point{0, 0};
}

// Closed paths treat the input as one periodic buffer (no duplicated end sample
// required). Open paths map their endpoints to the first and last wave samples.
inline double sample(const std::vector<float>& wave, double t, bool closed,
                     float minimum = 0, float maximum = 1, float fallback = .5f) {
    auto value = [&](size_t i) -> double {
        return std::isfinite(wave[i]) ? std::max(minimum, std::min(maximum, wave[i])) : fallback;
    };
    if(wave.empty()) return fallback;
    if(wave.size() == 1) return value(0);
    const double position = std::max(0.0, std::min(1.0, t)) *
        (closed ? wave.size() : wave.size() - 1);
    const size_t base = static_cast<size_t>(position);
    const size_t a = closed ? base % wave.size() : std::min(base, wave.size() - 1);
    const size_t b = closed ? (a + 1) % wave.size() : std::min(a + 1, wave.size() - 1);
    return value(a) + (value(b) - value(a)) * (position - base);
}

struct Frame {
    Result coordinates;
    std::vector<Point> normals;
    std::vector<double> targets, stiffness, motionInfluence;
    std::vector<size_t> representatives;
    std::vector<bool> closed;
};

inline void preparePath(const std::vector<float>& x, const std::vector<float>& y,
                   size_t begin, size_t end, const std::vector<float>& wave,
                   const std::vector<float>& amount, const std::vector<float>& stiffness,
                   const std::vector<float>& motionInfluence, Frame& frame) {
    std::vector<Point> points;
    std::vector<size_t> indices;
    points.reserve(end - begin);
    indices.reserve(end - begin);
    // Collapse adjacent duplicates for tangent calculation, retaining every
    // original output slot. This also keeps long zero-length runs linear-time.
    for(size_t i = begin; i < end; ++i) {
        if(points.empty() || points.back().x != x[i] || points.back().y != y[i])
            points.push_back({x[i], y[i]});
        indices.push_back(points.size() - 1);
    }
    if(points.size() < 2) return;
    const bool closed = points.size() > 2 && points.front().x == points.back().x &&
                        points.front().y == points.back().y;
    if(closed) {
        const size_t duplicate = points.size() - 1;
        for(auto& index : indices) if(index == duplicate) index = 0;
        points.pop_back();
    }
    const size_t count = points.size();
    std::vector<double> distance(count, 0);
    double area = 0;
    for(size_t i = 1; i < count; ++i)
        distance[i] = distance[i - 1] + std::hypot(points[i].x - points[i - 1].x,
                                                  points[i].y - points[i - 1].y);
    double length = distance.back();
    if(closed) {
        length += std::hypot(points.back().x - points.front().x, points.back().y - points.front().y);
        for(size_t i = 0; i < count; ++i) {
            const auto a = points[i], b = points[(i + 1) % count];
            // Translate before computing area to avoid cancellation far from 0.
            area += (a.x - points[0].x) * (b.y - points[0].y) -
                    (b.x - points[0].x) * (a.y - points[0].y);
        }
    }
    if(length == 0) return;
    std::vector<Point> normals(count);
    std::vector<double> targets(count), springs(count), motion(count);
    for(size_t i = 0; i < count; ++i) {
        const bool hasPrev = closed || i > 0, hasNext = closed || i + 1 < count;
        const Point incoming = hasPrev ? direction(points[(i + count - 1) % count], points[i]) : Point{0,0};
        const Point outgoing = hasNext ? direction(points[i], points[(i + 1) % count]) : Point{0,0};
        Point tangent = direction({0,0}, {incoming.x + outgoing.x, incoming.y + outgoing.y});
        // A reversing cusp has no unique normal; use its outgoing direction.
        if(tangent.x == 0 && tangent.y == 0) tangent = hasNext ? outgoing : incoming;
        const double t = distance[i] / length;
        targets[i] = (2 * sample(wave, t, closed) - 1) * sample(amount, t, closed, -1, 1, 0);
        springs[i] = sample(stiffness, t, closed, 0, 1000, 0);
        motion[i] = sample(motionInfluence, t, closed, 0, 1, 0);
        // Positive means outward for simple closed loops, left of travel for
        // open paths. Each hole is treated as its own independent closed shape.
        const double side = closed && area > 0 ? -1 : 1;
        normals[i] = {-tangent.y * side, tangent.x * side};
    }
    std::vector<size_t> first(count, end);
    for(size_t i = begin; i < end; ++i) {
        const size_t index = indices[i - begin];
        if(first[index] == end) first[index] = i;
        frame.normals[i] = normals[index];
        frame.targets[i] = targets[index];
        frame.stiffness[i] = springs[index];
        frame.motionInfluence[i] = motion[index];
        frame.representatives[i] = first[index];
        frame.closed[i] = closed;
    }
}

inline Frame prepare(const std::vector<float>& x, const std::vector<float>& y,
                     const std::vector<float>& wave, const std::vector<float>& amount,
                     const std::vector<float>& stiffness, const std::vector<float>& motionInfluence = {}) {
    if(x.size() != y.size()) return {};
    for(size_t i = 0; i < x.size(); ++i)
        if(!std::isfinite(x[i]) || !std::isfinite(y[i]) || ((x[i] == -1) != (y[i] == -1))) return {};
    Frame frame;
    frame.coordinates = {x, y};
    frame.normals.resize(x.size(), {0,0});
    frame.targets.resize(x.size(), 0);
    frame.stiffness.resize(x.size(), 0);
    frame.motionInfluence.resize(x.size(), 0);
    frame.representatives.resize(x.size(), std::numeric_limits<size_t>::max());
    frame.closed.resize(x.size(), false);
    size_t begin = 0;
    for(size_t i = 0; i <= x.size(); ++i) {
        if(i == x.size() || x[i] == -1) {
            if(i > begin) preparePath(x, y, begin, i, wave, amount, stiffness, motionInfluence, frame);
            begin = i + 1;
        }
    }
    return frame;
}

inline Result applyOffsets(const Frame& frame, const std::vector<double>& offsets) {
    Result result = frame.coordinates;
    for(size_t i = 0; i < result.x.size(); ++i) {
        result.x[i] = static_cast<float>(result.x[i] + frame.normals[i].x * offsets[i]);
        result.y[i] = static_cast<float>(result.y[i] + frame.normals[i].y * offsets[i]);
    }
    return result;
}

inline Result process(const std::vector<float>& x, const std::vector<float>& y,
                      const std::vector<float>& wave, const std::vector<float>& amount) {
    const auto frame = prepare(x, y, wave, amount, {});
    return applyOffsets(frame, frame.targets);
}

inline Result process(const std::vector<float>& x, const std::vector<float>& y,
                      const std::vector<float>& wave, float amount) {
    return process(x, y, wave, std::vector<float>{amount});
}

class Simulation {
public:
    void reset() {
        offsets.clear(); velocities.clear(); representatives.clear(); closed.clear();
        previousCoordinates = {}; previousSpring = false;
    }

    Result update(const std::vector<float>& x, const std::vector<float>& y,
                  const std::vector<float>& wave, const std::vector<float>& amount,
                  const std::vector<float>& stiffness, bool spring, double seconds,
                  const std::vector<float>& motionInfluence = {}) {
        const auto frame = prepare(x, y, wave, amount, stiffness, motionInfluence);
        const bool topologyChanged = representatives != frame.representatives || closed != frame.closed;
        if(topologyChanged) {
            offsets.assign(frame.targets.size(), 0);
            velocities.assign(frame.targets.size(), 0);
        }
        representatives = frame.representatives;
        closed = frame.closed;
        if(!spring) {
            offsets = frame.targets;
            velocities.assign(offsets.size(), 0);
        } else {
            // Exact damped-spring solution for a target held during this frame.
            // Unit mass and fixed damping ratio 0.65 give mild, decaying overshoot.
            // Limit long stalls; no unstable Euler integration or catch-up loop.
            const double dt = std::isfinite(seconds) ? std::max(0.0, std::min(.1, seconds)) : 0;
            // Rebase at startup/mode switches, topology changes and timing gaps.
            // Track input positions even when influence is zero: enabling it must
            // never replay previously ignored movement.
            const bool useMotion = previousSpring && !topologyChanged &&
                previousCoordinates.x.size() == x.size() &&
                std::isfinite(seconds) && seconds > 1e-6 && seconds <= .1;
            for(size_t i = 0; i < offsets.size(); ++i) {
                const size_t representative = representatives[i];
                if(representative == std::numeric_limits<size_t>::max()) {
                    offsets[i] = velocities[i] = 0;
                } else if(representative != i) {
                    offsets[i] = offsets[representative];
                    velocities[i] = velocities[representative];
                } else if(frame.stiffness[i] <= 0) {
                    // Zero stiffness holds the current normal offset, without drift.
                    velocities[i] = 0;
                } else if(dt > 0) {
                    const double k = frame.stiffness[i];
                    const double decay = .65 * std::sqrt(k);
                    const double frequency = std::sqrt(k) * std::sqrt(1 - .65 * .65);
                    const double e = offsets[i] - frame.targets[i], v = velocities[i];
                    const double c = std::cos(frequency * dt), s = std::sin(frequency * dt);
                    const double envelope = std::exp(-decay * dt);
                    offsets[i] = frame.targets[i] + envelope * (e*c + (v + decay*e)/frequency*s);
                    velocities[i] = envelope * (v*c - (decay*v + k*e)/frequency*s);
                    if(useMotion && frame.motionInfluence[i] > 0) {
                        const auto normal = frame.normals[i];
                        const double movement = frame.motionInfluence[i] *
                            ((double(x[i]) - previousCoordinates.x[i]) * normal.x +
                             (double(y[i]) - previousCoordinates.y[i]) * normal.y);
                        // Exact constant-rest-velocity forcing over this frame:
                        // offset' = velocity - influence * restVelocity.normal
                        // velocity' = -k*(offset-target) - 2*decay*velocity.
                        // The normal is held at its current direction. This gives
                        // lag while moving and a damped rebound after stopping.
                        const double damping = 2 * decay;
                        double h; // (1 - exp(-decay*dt)*(cos + decay/frequency*sin)) / k
                        if(std::sqrt(k) * dt < .001) {
                            // Avoid cancellation for very soft springs / tiny dt.
                            h = dt*dt * (.5 - damping*dt/6 +
                                (damping*damping-k)*dt*dt/24 +
                                (2*damping*k-damping*damping*damping)*dt*dt*dt/120);
                        } else {
                            h = (1 - envelope * (c + decay/frequency*s)) / k;
                        }
                        const double b = envelope * s / frequency;
                        offsets[i] -= movement * (b + damping*h) / dt;
                        velocities[i] += movement * k*h / dt;
                    }
                }
            }
        }
        previousCoordinates = frame.coordinates;
        previousSpring = spring;
        return applyOffsets(frame, offsets);
    }

private:
    Result previousCoordinates;
    bool previousSpring = false;
    std::vector<double> offsets, velocities;
    std::vector<size_t> representatives;
    std::vector<bool> closed;
};
} // namespace waveItCore
