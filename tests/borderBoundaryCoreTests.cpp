#include "../src/borderBoundaryCore.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <limits>
#include <random>
#include <string>

using namespace borderBoundaryCore;
static bool close(float a, float b, float tolerance = 2e-6f) { return std::abs(a-b) < tolerance; }

// Independently evaluate rounded-box signed distance to verify containment.
static double signedDistance(float x, float y, const Controls& c) {
    const double half = 0.5 - c.border, radius = half * c.roundness;
    const double qx = std::abs(x - 0.5) - (half - radius);
    const double qy = std::abs(y - 0.5) - (half - radius);
    return std::hypot(std::max(qx, 0.0), std::max(qy, 0.0))
        + std::min(std::max(qx, qy), 0.0) - radius;
}

static void regressions() {
    std::vector<float> ox, oy;
    Controls c{0.1f, 0.f, 1.f, 0.f};
    assert(process({0.5f, 0.f, 1.f, -1.f, 2.f}, {.5f, .5f, .5f, -1.f, 2.f}, c, ox, oy));
    assert(close(ox[0], .5f) && close(ox[1], .1f) && close(ox[2], .9f));
    assert(ox[3] == -1 && oy[3] == -1 && close(ox[4], .9f) && close(oy[4], .9f));
    const auto oldX = ox, oldY = oy;
    assert(!process({0, 1}, {0}, c, ox, oy) && ox == oldX && oy == oldY);
    assert(process({}, {}, c, ox, oy) && ox.empty() && oy.empty());

    c.roundness = 1;
    process({1, .5f}, {1, 1}, c, ox, oy);
    assert(close(ox[0], .5f + .4f / std::sqrt(2.f)) && close(oy[1], .9f));
    c.roundness = .5f;
    process({1}, {1}, c, ox, oy);
    assert(close(ox[0], .7f + .2f / std::sqrt(2.f)) && ox == oy);

    // At the side: knee starts at .8, default Curve gives .875 at the .9 border,
    // and becomes a hard cap at 1.0. No discontinuity or inversion through the ramp.
    c = {.1f, .1f, 1.f, 0.f};
    process({.2f, .5f, .8f, .9f, 1.f, 2.f}, {.5f, .5f, .5f, .5f, .5f, .5f}, c, ox, oy);
    assert(close(ox[0], .2f) && ox[1] == .5f && close(ox[2], .8f));
    assert(close(ox[3], .875f) && close(ox[4], .9f) && close(ox[5], .9f));
    for(float power : {.1f, .5f, 1.f, 2.f, 8.f}) {
        c.curve = power;
        float previous = 0.5f;
        for(int i = 0; i <= 2000; ++i) {
            const float input = .5f + i * .001f;
            process({input}, {.5f}, c, ox, oy);
            assert(ox[0] >= previous - 1e-7f && ox[0] <= input + 1e-7f && ox[0] <= .900001f);
            previous = ox[0];
        }
    }

    // Finite extreme coordinates, invalid vertices and malformed separators.
    const float maximum = std::numeric_limits<float>::max();
    const float nan = std::numeric_limits<float>::quiet_NaN();
    process({maximum, -maximum, nan, .5f, -1.f},
            {maximum, maximum, .5f, std::numeric_limits<float>::infinity(), .5f}, c, ox, oy);
    assert(close(ox[0], .9f) && close(oy[0], .9f) && close(ox[1], .1f));
    for(int i = 2; i < 5; ++i) assert(ox[i] == -1.f && oy[i] == -1.f);
    c.border = .5f;
    process({0, .5f, 1, -1}, {0, .5f, 1, -1}, c, ox, oy);
    assert(ox == std::vector<float>({.5f, .5f, .5f, -1}) && ox == oy);

    // Random geometry: containment, radial direction, reflection symmetry and fixed topology.
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> coordinate(-2.f, 3.f);
    std::vector<float> x(3000), y(3000);
    for(size_t i = 0; i < x.size(); ++i) { x[i] = coordinate(rng); y[i] = coordinate(rng); }
    for(float border : {0.f, .05f, .49f}) for(float rounded : {0.f, .01f, .5f, 1.f})
    for(float feather : {0.f, .05f, .5f}) for(float power : {.1f, 1.f, 2.f, 3.7f, 8.f}) {
        c = {border, feather, power, rounded};
        process(x, y, c, ox, oy);
        assert(ox.size() == x.size() && oy.size() == y.size());
        for(size_t i = 0; i < x.size(); ++i) {
            assert(std::isfinite(ox[i]) && std::isfinite(oy[i]));
            assert(signedDistance(ox[i], oy[i], c) < 1e-6);
            assert(std::abs((ox[i]-.5)*(y[i]-.5) - (oy[i]-.5)*(x[i]-.5)) < 3e-7);
        }
        auto* capacityX = ox.data(); auto* capacityY = oy.data();
        process(x, y, c, ox, oy);
        assert(capacityX == ox.data() && capacityY == oy.data());
    }
    c = {.05f, .1f, 2.7f, .6f};
    process({.02f, .98f, .02f, .98f, .02f}, {.2f, .2f, .8f, .8f, .2f}, c, ox, oy);
    assert(close(ox[0], 1-ox[1]) && close(oy[0], 1-oy[2]));
    assert(ox[0] == ox[4] && oy[0] == oy[4]); // closure survives
    // Invalid controls are safely bounded/fallbacked too.
    process(x, y, {nan, -1.f, 0.f, 100.f}, ox, oy);
    for(size_t i = 0; i < x.size(); ++i) assert(signedDistance(ox[i], oy[i], {.05f, 0, .1f, 1}) < 1e-6);
    std::cout << "Border Boundary core: geometry, ramp monotonicity, containment, separators, invalid inputs and buffer reuse passed.\n";
}

static void benchmark() {
    volatile float checksum = 0;
    for(size_t count : {500u, 3000u}) for(float curve : {1.f, 2.7f}) {
        std::vector<float> x(count), y(count), ox, oy;
        std::mt19937 rng(19);
        std::uniform_real_distribution<float> coordinate(-.2f, 1.2f);
        for(size_t i = 0; i < count; ++i) { x[i] = coordinate(rng); y[i] = coordinate(rng); }
        Controls c{.05f, .15f, curve, .6f};
        process(x, y, c, ox, oy);
        constexpr int iterations = 5000;
        const auto start = std::chrono::steady_clock::now();
        for(int i = 0; i < iterations; ++i) {
            process(x, y, c, ox, oy);
            checksum += ox[i % count];
        }
        const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now()-start).count() / iterations;
        std::cout << count << " vertices, curve " << curve << ": " << us << " us/pass (core only)\n";
    }
    (void)checksum;
}

int main(int argc, char** argv) {
    if(argc > 1 && std::string(argv[1]) == "--benchmark") benchmark();
    else regressions();
}
