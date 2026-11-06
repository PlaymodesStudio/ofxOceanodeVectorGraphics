#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace borderBoundaryCore {

struct Controls {
    float border = 0.05f;
    float falloff = 0.05f;
    float curve = 1.f;
    float roundness = 0.f;
};

inline double bounded(float value, double fallback, double low, double high) {
    return std::isfinite(value) ? std::max(low, std::min(high, double(value))) : fallback;
}

// Separate output buffers must not alias either input. They retain capacity between calls.
// Mismatched inputs leave the last complete output pair untouched.
inline bool process(const std::vector<float>& x, const std::vector<float>& y,
                    const Controls& controls, std::vector<float>& outX, std::vector<float>& outY) {
    if(x.size() != y.size()) return false;
    const double half = 0.5 - bounded(controls.border, 0.05, 0, 0.5);
    const double feather = bounded(controls.falloff, 0.05, 0, 0.5);
    const double curve = bounded(controls.curve, 1, 0.1, 8);
    const double inverseCurve = 1.0 / curve;
    const double radius = half * bounded(controls.roundness, 0, 0, 1);
    const double corner = half - radius;
    // Every rounded box contains this disk, including its unaffected feather interior.
    const double safeRadius = std::max(0.0, half - feather);
    const double safeRadiusSquared = safeRadius * safeRadius;
    outX.resize(x.size());
    outY.resize(x.size());
    for(size_t i = 0; i < x.size(); ++i) {
        const float px = x[i], py = y[i];
        if(px == -1.f || py == -1.f || !std::isfinite(px) || !std::isfinite(py)) {
            // A malformed/nonfinite vertex becomes a path break, never a spurious segment.
            outX[i] = outY[i] = -1.f;
            continue;
        }
        if(half == 0) {
            outX[i] = outY[i] = 0.5f;
            continue;
        }
        // Double intermediates also handle finite float coordinates far outside the viewport.
        const double dx = double(px) - 0.5, dy = double(py) - 0.5;
        const double distanceSquared = dx * dx + dy * dy;
        if(distanceSquared <= safeRadiusSquared) {
            outX[i] = px; outY[i] = py;
            continue;
        }
        const double distance = std::sqrt(distanceSquared);
        const double ux = std::abs(dx) / distance, uy = std::abs(dy) / distance;
        const double major = std::max(ux, uy), minor = std::min(ux, uy);
        double boundary = half / major;
        if(radius > 0 && boundary * minor > corner) {
            // Ray/rounded-corner intersection. No trig, path traversal or neighbour search.
            const double difference = ux - uy;
            boundary = corner * (ux + uy) + std::sqrt(std::max(0.0,
                radius * radius - corner * corner * difference * difference));
        }
        const double width = std::min(feather, boundary);
        const double start = boundary - width;
        if(distance <= start) {
            outX[i] = px; outY[i] = py;
            continue;
        }
        double mapped = boundary;
        if(width > 0 && distance < boundary + width * inverseCurve) {
            // Integrate velocity attenuation 1 - pow(t, Curve) across the soft knee.
            // Value and slope match identity at its start and a hard cap at its end.
            const double t = (distance - start) / (width * (1.0 + inverseCurve));
            const double power = curve == 1.0 ? t * t
                               : curve == 2.0 ? t * t * t : std::pow(t, curve + 1.0);
            mapped = std::min(boundary, distance - width * inverseCurve * power);
        }
        const double scale = mapped / distance;
        outX[i] = static_cast<float>(0.5 + dx * scale);
        outY[i] = static_cast<float>(0.5 + dy * scale);
    }
    return true;
}

} // namespace borderBoundaryCore
