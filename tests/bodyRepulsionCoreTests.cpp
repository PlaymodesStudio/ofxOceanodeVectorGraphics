#include "../src/bodyRepulsionCore.h"
#include <cassert>
#include <iostream>

using bodyRepulsionCore::Simulation;
using bodyRepulsionCore::Settings;
using V = std::vector<float>;

int main() {
    Settings drift; drift.returnStrength = {0}; drift.damping = {0};
    Simulation s;
    s.update({.5f}, {.5f}, {.4f}, {.5f}, {.2f}, {1}, drift, 1.0/60);
    assert(s.x()[0] > .5f && s.y()[0] == .5f);
    const float pushed = s.x()[0];
    s.update({.5f}, {.5f}, {}, {}, {.2f}, {1}, drift, 1.0/60);
    const float traveled = s.x()[0] - pushed;
    assert(traveled > 0);
    const float previous = s.x()[0];
    s.update({.5f}, {.5f}, {}, {}, {.2f}, {1}, drift, 1.0/60);
    assert(std::abs(s.x()[0] - previous - traveled) < 1e-6f);

    // Drag removes momentum smoothly; the spring settles back to the moving rest shape.
    Settings drag = drift; drag.damping = {5};
    float lastTravel = traveled;
    for(int i = 0; i < 100; ++i) {
        const float before = s.x()[0];
        s.update({.5f}, {.5f}, {}, {}, {}, {1}, drag, 1.0/60);
        const float delta = s.x()[0] - before;
        assert(delta >= 0 && delta <= lastTravel + 1e-7f);
        lastTravel = delta;
    }
    Settings spring;
    for(int i = 0; i < 600; ++i) s.update({.5f}, {.5f}, {}, {}, {}, {1}, spring, 1.0/60);
    assert(std::abs(s.x()[0] - .5f) < 1e-5f);
    s.update({.8f}, {.5f}, {}, {}, {}, {1}, spring, 1.0/60);
    assert(s.x()[0] > .5f && s.x()[0] < .8f); // Moving Body input pulls, not teleports.

    // Fixed stepping gives the same static-input trajectory at different host frame rates.
    Simulation slow, fast, jitter;
    for(int i = 0; i < 90; ++i) slow.update({.5f}, {.5f}, {.4f}, {.4f}, {.3f}, {1}, spring, 1.0/30);
    for(int i = 0; i < 432; ++i) fast.update({.5f}, {.5f}, {.4f}, {.4f}, {.3f}, {1}, spring, 1.0/144);
    for(int i = 0; i < 120; ++i) {
        jitter.update({.5f}, {.5f}, {.4f}, {.4f}, {.3f}, {1}, spring, .007);
        jitter.update({.5f}, {.5f}, {.4f}, {.4f}, {.3f}, {1}, spring, .018);
    }
    assert(slow.x() == fast.x() && slow.y() == fast.y());
    assert(slow.x() == jitter.x() && slow.y() == jitter.y());

    // One value retains the same trajectory as repeating it for every Body point.
    Simulation singletonControls, expandedControls;
    Settings expandedSettings{{40,40,40},{8,8,8},{3,3,3}};
    const V sharedBodyX{.45f,-1,.5f,.7f}, sharedBodyY{.5f,-1,.5f,.6f};
    for(int i = 0; i < 180; ++i) {
        singletonControls.update(sharedBodyX, sharedBodyY, {.4f}, {.5f}, {.3f}, {1}, spring, 1.0/60);
        expandedControls.update(sharedBodyX, sharedBodyY, {.4f}, {.5f}, {.3f,.3f,.3f},
                                {1}, expandedSettings, 1.0/60);
        assert(singletonControls.x() == expandedControls.x());
        assert(singletonControls.y() == expandedControls.y());
    }
    Settings bodyWeights{{0,40},{0},{0}};
    Simulation perBody;
    perBody.update({.5f,-1,.5f,.5f}, {.5f,-1,.5f,.5f}, {.4f}, {.5f}, {.2f},
                   {1}, bodyWeights, 1.0/60);
    assert(perBody.x()[0] == .5f && perBody.x()[1] == -1);
    assert(perBody.x()[2] > .5f && perBody.x()[2] == perBody.x()[3]);

    Simulation independentDrag;
    independentDrag.update({.5f,.5f}, {.5f,.5f}, {.4f}, {.5f}, {.2f}, {1}, drift, 1.0/60);
    const float beforeDrag = independentDrag.x()[0];
    assert(independentDrag.x()[0] == independentDrag.x()[1]);
    Settings perDrag = drift; perDrag.damping = {0,50};
    independentDrag.update({.5f,.5f}, {.5f,.5f}, {}, {}, {.2f}, {1}, perDrag, 1.0/60);
    assert(independentDrag.x()[0] > independentDrag.x()[1] && independentDrag.x()[1] > beforeDrag);
    Simulation independentSpring;
    Settings perSpring{{0},{0,8},{0}};
    independentSpring.update({.5f,-1,.5f,.5f}, {.5f,-1,.5f,.5f}, {}, {}, {}, {1}, perSpring, 0);
    independentSpring.update({.8f,-1,.8f,.8f}, {.5f,-1,.5f,.5f}, {}, {}, {}, {1}, perSpring, 1.0/60);
    assert(independentSpring.x()[0] == .5f && independentSpring.x()[2] > .5f);
    assert(independentSpring.x()[2] == independentSpring.x()[3]);
    Settings invalidControls{{},{-1,std::numeric_limits<float>::quiet_NaN()},
                             {std::numeric_limits<float>::infinity(),-1}};
    Simulation invalidResponse;
    invalidResponse.update({.5f,.5f}, {.5f,.5f}, {.4f}, {.5f}, {.2f}, {1}, invalidControls, .1);
    assert(invalidResponse.x() == V({.5f,.5f}));

    Simulation mapping;
    const V bx{.5f,-1,.5f,.5f}, by{.5f,-1,.5f,.5f};
    mapping.update(bx, by, {.4f}, {.5f}, {0,.2f}, {1}, drift, .05);
    assert(mapping.x()[0] == .5f && mapping.x()[1] == -1 && mapping.y()[1] == -1);
    assert(mapping.x()[2] > .5f && mapping.x()[2] == mapping.x()[3]);
    mapping.update(bx, by, {.4f}, {.5f}, {.2f}, {1}, drift, .05, true);
    assert(mapping.x() == bx && mapping.y() == by);
    mapping.update(bx, by, {.4f}, {.5f}, {.2f}, {1}, drift, .05);
    assert(mapping.x()[0] == mapping.x()[2] && mapping.x()[2] == mapping.x()[3]);
    mapping.update({.9f}, {.8f}, {}, {}, {}, {1}, drift, 0);
    assert(mapping.x() == V{.9f} && mapping.y() == V{.8f}); // Count resets state.
    mapping.update({-1}, {-1}, {}, {}, {}, {1}, drift, .05);
    assert(mapping.x() == V{-1} && mapping.y() == V{-1}); // Separator-layout reset.
    mapping.update({.7f}, {.6f}, {}, {}, {}, {1}, drift, 0);
    assert(mapping.x() == V{.7f} && mapping.y() == V{.6f});

    // Outside radius, boundary, missing fear and symmetric opposing forces.
    for(const V& fear : {V{}, V{0}, V{-.1f}, V{.05f}, V{.25f}}) {
        Simulation untouched;
        untouched.update({.5f}, {.5f}, {.25f}, {.5f}, fear, {1}, drift, .1);
        assert(untouched.x() == V{.5f});
    }
    Simulation symmetry;
    symmetry.update({.5f}, {.5f}, {.25f,.75f}, {.5f,.5f}, {.5f}, {1}, drift, .1);
    assert(symmetry.x() == V{.5f} && symmetry.y() == V{.5f});
    Simulation nearer, farther;
    nearer.update({.5f}, {.5f}, {.45f}, {.5f}, {.2f}, {1}, drift, 1.0/240);
    farther.update({.5f}, {.5f}, {.35f}, {.5f}, {.2f}, {1}, drift, 1.0/240);
    assert(nearer.x()[0] > farther.x()[0] && farther.x()[0] > .5f);
    Simulation two;
    two.update({.5f}, {.5f}, {.4f,.4f}, {.5f,.5f}, {.2f}, {1}, drift, 1.0/240);
    Simulation one;
    one.update({.5f}, {.5f}, {.4f}, {.5f}, {.2f}, {1}, drift, 1.0/240);
    assert(two.x()[0] > one.x()[0]);

    // Each repeller's multiplier scales its acceleration independently.
    auto weighted = [&](const V& strength) {
        Simulation result;
        result.update({.5f}, {.5f}, {.4f}, {.5f}, {.2f}, strength, drift, 1.0/240);
        return result.x()[0];
    };
    const float full = weighted({1});
    const float half = weighted({.5f});
    const float doubleForce = weighted({2});
    assert(half > .5f && half < full && doubleForce > full);
    assert(std::abs((half-.5f) - .5f*(full-.5f)) < 1e-7f);
    assert(std::abs((doubleForce-.5f) - 2*(full-.5f)) < 1e-7f);
    for(const V& strength : {V{}, V{0}, V{-.1f},
                            V{std::numeric_limits<float>::quiet_NaN()},
                            V{std::numeric_limits<float>::infinity()}})
        assert(weighted(strength) == .5f);

    Simulation selective, broadcast, expanded, forceSeparators, skippedInvalid;
    selective.update({.5f}, {.5f}, {.4f,.6f}, {.5f,.5f}, {.2f}, {1,0}, drift, 1.0/240);
    assert(selective.x()[0] == full); // Second Force contributes nothing.
    broadcast.update({.5f}, {.5f}, {.4f,.4f,.4f}, {.5f,.5f,.5f}, {.2f}, {.5f}, drift, 1.0/240);
    expanded.update({.5f}, {.5f}, {.4f,.4f,.4f}, {.5f,.5f,.5f}, {.2f}, {.5f,.5f,.5f}, drift, 1.0/240);
    assert(broadcast.x() == expanded.x());
    Simulation shorter;
    shorter.update({.5f}, {.5f}, {.4f,.4f,.4f}, {.5f,.5f,.5f}, {.2f}, {0,.5f}, drift, 1.0/240);
    assert(shorter.x()[0] == full); // Last supplied strength repeats for the third Force.
    forceSeparators.update({.5f}, {.5f}, {.4f,-1,.4f}, {.5f,-1,.5f}, {.2f}, {0,1}, drift, 1.0/240);
    assert(forceSeparators.x()[0] == full);
    skippedInvalid.update({.5f}, {.5f}, {std::numeric_limits<float>::quiet_NaN(),.4f},
                          {.5f,.5f}, {.2f}, {0,1}, drift, 1.0/240);
    assert(skippedInvalid.x()[0] == full); // Filtering invalid points does not shift strengths.
    const float beforeDisabled = selective.x()[0];
    selective.update({.5f}, {.5f}, {.4f,.6f}, {.5f,.5f}, {.2f}, {0}, drift, 1.0/240);
    assert(selective.x()[0] > beforeDisabled); // Disabling force preserves momentum.

    Simulation overlap;
    overlap.update({.5f,.5f}, {.5f,.5f}, {.5f}, {.5f}, {.2f}, {1}, drift, .1);
    assert(overlap.x()[0] == overlap.x()[1] && overlap.y()[0] == overlap.y()[1]);
    assert(std::hypot(overlap.x()[0]-.5f, overlap.y()[0]-.5f) > 0);

    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    for(const V& invalid : {V{}, V{.5f,.6f}, V{nan}, V{inf}, V{-1}}) {
        s.update(invalid, {.5f}, {}, {}, {}, {1}, spring, .1);
        assert(s.x().empty() && s.y().empty());
    }
    s.update({.5f}, {.5f}, {}, {}, {}, {1}, spring, .01);
    assert(s.x() == V{.5f});
    // Malformed Force input must not suppress inertial updates or contaminate positions.
    s.update({.5f}, {.5f}, {.4f}, {.5f}, {.2f}, {1}, drift, .01);
    float before = s.x()[0];
    s.update({.5f}, {.5f}, {.4f}, {}, {.2f}, {1}, drift, .01);
    assert(s.x()[0] > before);
    Simulation invalidForces;
    invalidForces.update({.5f}, {.5f}, {nan,inf,-1}, {.5f,.5f,-1}, {.2f}, {1}, drift, .1);
    assert(invalidForces.x() == V{.5f});

    Simulation hitch, capped;
    hitch.update({.5f}, {.5f}, {.4f}, {.5f}, {.2f}, {1}, spring, 50);
    capped.update({.5f}, {.5f}, {.4f}, {.5f}, {.2f}, {1}, spring, .1);
    assert(hitch.x() == capped.x());
    const V saved = hitch.x();
    for(double dt : {0.0,-1.0,static_cast<double>(nan),static_cast<double>(inf)}) {
        hitch.update({.5f}, {.5f}, {.4f}, {.5f}, {.2f}, {1}, spring, dt);
        assert(hitch.x() == saved);
    }
    // Maximum controls remain bounded under prolonged motion and changing inputs.
    Settings extreme{{1000},{1000},{100}};
    Simulation stress;
    for(int i = 0; i < 600; ++i) {
        const float f = .5f + .25f * std::sin(i*.02f);
        stress.update({.5f,.6f}, {.5f,.6f}, {f,.5f}, {.5f,f}, {.4f}, {1}, extreme, .1);
        for(float x : stress.x()) assert(std::isfinite(x) && std::abs(x) < 2);
        for(float y : stress.y()) assert(std::isfinite(y) && std::abs(y) < 2);
    }
    std::cout << "PASS: repulsion, per-Force strength, per-Body controls, scalar broadcasting, momentum, "
                 "damping, moving springs, frame rates, fear mapping, "
                 "separators, resets, coincidence, invalid inputs and stress.\n";
}
