#include "../src/twoBodyRepulsionCore.h"
#include "../src/bodyRepulsionCore.h"
#include <cassert>
#include <cmath>
#include <iostream>

using twoBodyRepulsionCore::Simulation;
using twoBodyRepulsionCore::BodyControls;
using twoBodyRepulsionCore::Border;
using V = std::vector<float>;

static BodyControls drifting() { BodyControls c; c.returnStrength = {0}; c.damping = {0}; return c; }
static BodyControls passive() { BodyControls c = drifting(); c.repulsion = {0}; return c; }
static bool near(float a, float b, float tolerance = 1e-6f) { return std::abs(a - b) <= tolerance; }

int main() {
    const Border noBorder;
    const BodyControls spring;

    // With Radius 0 and a passive B, A follows Body Repulsion with B's points as Force points.
    {
        Simulation two; bodyRepulsionCore::Simulation one; bodyRepulsionCore::Settings settings;
        const V ax{.5f,-1,.55f,.6f}, ay{.5f,-1,.52f,.5f}, bx{.4f,.45f}, by{.5f,.52f};
        BodyControls b = passive(); b.strength = {1,.5f};
        for(int i = 0; i < 180; ++i) {
            // Body Repulsion reads Force points raw; a free-drifting B stays put, so wobble A's rest.
            const V restX{ax[0] + .02f * std::sin(i * .1f), -1, ax[2], ax[3]};
            two.update(restX, ay, spring, bx, by, b, noBorder, 1.0/60);
            one.update(restX, ay, bx, by, {.15f}, {1,.5f}, settings, 1.0/60);
            for(size_t k = 0; k < ax.size(); ++k) {
                assert(near(two.aX()[k], one.x()[k]) && near(two.aY()[k], one.y()[k]));
            }
        }
        assert(two.aX()[2] != ax[2]); // The comparison exercised real motion.
        assert(two.bX() == bx && two.bY() == by); // A passive Body rests.
    }

    // Both Bodies react; equal controls give mirrored motion and matching strength scaling.
    {
        Simulation s; const BodyControls d = drifting();
        s.update({.45f}, {.5f}, d, {.55f}, {.5f}, d, noBorder, 1.0/60);
        assert(s.aX()[0] < .45f && s.bX()[0] > .55f);
        assert(near(.45f - s.aX()[0], s.bX()[0] - .55f) && s.aY()[0] == .5f && s.bY()[0] == .5f);
        Simulation weighted; BodyControls weak = d; weak.strength = {.5f};
        weighted.update({.45f}, {.5f}, weak, {.55f}, {.5f}, d, noBorder, 1.0/60);
        assert(near(weighted.aX()[0], s.aX()[0])); // B's strength 1 still pushes A fully.
        assert(near(weighted.bX()[0] - .55f, .5f * (s.bX()[0] - .55f))); // A's half strength halves B's push.
        Simulation silent; BodyControls none = d; none.strength = {0};
        silent.update({.45f}, {.5f}, none, {.55f}, {.5f}, d, noBorder, 1.0/60);
        assert(silent.bX()[0] == .55f && silent.aX()[0] < .45f);
    }

    // Radius moves the fear boundary to the disk surfaces.
    {
        const BodyControls d = drifting();
        Simulation apart; apart.update({.3f}, {.5f}, d, {.6f}, {.5f}, d, noBorder, .1);
        assert(apart.aX()[0] == .3f && apart.bX()[0] == .6f); // 0.3 apart, fear 0.15: untouched.
        BodyControls disk = d; disk.radius = {.1f};
        Simulation one; one.update({.3f}, {.5f}, disk, {.6f}, {.5f}, d, noBorder, .1);
        assert(one.aX()[0] == .3f && one.bX()[0] == .6f); // Surface gap 0.2 is outside fear 0.15.
        Simulation both; both.update({.3f}, {.5f}, disk, {.6f}, {.5f}, disk, noBorder, .1);
        assert(both.aX()[0] < .3f && both.bX()[0] > .6f); // Surface gap 0.1 is inside fear 0.15.
        // Inside the overlap the push is constant, as at exact contact.
        Simulation contact, overlapped;
        contact.update({.4f}, {.5f}, disk, {.6f}, {.5f}, disk, noBorder, 1.0/240);
        overlapped.update({.45f}, {.5f}, disk, {.55f}, {.5f}, disk, noBorder, 1.0/240);
        assert(near(.4f - contact.aX()[0], .45f - overlapped.aX()[0]));
        assert(contact.aX()[0] < .4f);
        // Fear zero disables the response even inside a disk overlap.
        BodyControls brave = disk; brave.fear = {0};
        Simulation fearless; fearless.update({.45f}, {.5f}, brave, {.55f}, {.5f}, disk, noBorder, .1);
        assert(fearless.aX()[0] == .45f && fearless.bX()[0] > .55f);
    }

    // Per-point controls map to Body points, excluding separators, with broadcast of one value.
    {
        const BodyControls d = drifting();
        BodyControls perPoint = d; perPoint.repulsion = {0,40}; perPoint.radius = {0,.05f};
        Simulation s;
        s.update({.5f,-1,.5f,.5f}, {.5f,-1,.5f,.5f}, perPoint, {.4f}, {.5f}, passive(), noBorder, 1.0/60);
        assert(s.aX()[0] == .5f && s.aX()[1] == -1 && s.aY()[1] == -1);
        assert(s.aX()[2] > .5f && s.aX()[2] == s.aX()[3]);
        Simulation singleton, expanded;
        BodyControls many = d; many.fear = {.15f,.15f,.15f}; many.repulsion = {40,40,40};
        for(int i = 0; i < 120; ++i) {
            singleton.update({.45f,-1,.5f,.55f}, {.5f,-1,.5f,.5f}, d, {.6f}, {.5f}, d, noBorder, 1.0/60);
            expanded.update({.45f,-1,.5f,.55f}, {.5f,-1,.5f,.5f}, many, {.6f}, {.5f}, d, noBorder, 1.0/60);
            assert(singleton.aX() == expanded.aX() && singleton.bX() == expanded.bX());
        }
        BodyControls invalid = d;
        invalid.fear = {}; invalid.repulsion = {-1, std::numeric_limits<float>::quiet_NaN()};
        invalid.radius = {std::numeric_limits<float>::infinity()};
        Simulation ignored; ignored.update({.45f}, {.5f}, invalid, {.55f}, {.5f}, passive(), noBorder, .1);
        assert(ignored.aX() == V{.45f});
    }

    // Border: hard wall at the margin, soft push over the feather, inactive at zero.
    {
        const BodyControls d = drifting();
        Simulation off; off.update({.02f}, {.98f}, d, {.5f}, {.5f}, passive(), noBorder, .1);
        assert(off.aX() == V{.02f} && off.aY() == V{.98f});
        Border margin; margin.margin = .1;
        Simulation walled; walled.update({.02f}, {.98f}, d, {.5f}, {.5f}, passive(), margin, .1);
        assert(near(walled.aX()[0], .1f) && near(walled.aY()[0], .9f));
        // Radius only shapes the Body interaction; the wall constrains the points themselves.
        BodyControls disk = d; disk.radius = {.3f};
        Simulation diskWall; diskWall.update({.02f}, {.98f}, disk, {.5f}, {.5f}, passive(), margin, .1);
        assert(near(diskWall.aX()[0], .1f) && near(diskWall.aY()[0], .9f));
        Border huge; huge.margin = .6;
        Simulation centred; centred.update({.02f}, {.98f}, d, {.5f}, {.5f}, passive(), huge, .1);
        assert(near(centred.aX()[0], .5f) && near(centred.aY()[0], .5f));
        Border feather; feather.margin = .1; feather.feather = .1;
        Simulation soft; soft.update({.15f}, {.5f}, d, {.5f}, {.5f}, passive(), feather, 1.0/60);
        assert(soft.aX()[0] > .15f && soft.aY()[0] == .5f); // Pushed inward before the margin.
        Simulation deep; deep.update({.25f}, {.5f}, d, {.5f}, {.5f}, passive(), feather, 1.0/60);
        assert(deep.aX()[0] == .25f); // Beyond margin plus feather nothing happens.
        BodyControls stiff = d; stiff.repulsion = {0};
        Simulation unpushed; unpushed.update({.15f}, {.5f}, stiff, {.5f}, {.5f}, passive(), feather, 1.0/60);
        assert(unpushed.aX()[0] == .15f);
        Simulation clamped; clamped.update({.05f}, {.5f}, stiff, {.5f}, {.5f}, passive(), feather, 1.0/60);
        assert(near(clamped.aX()[0], .1f)); // The wall holds regardless of Repulsion Strength.
        Border featherOnly; featherOnly.feather = .1;
        Simulation edge; edge.update({-.2f}, {.5f}, d, {.5f}, {.5f}, passive(), featherOnly, 1.0/60);
        assert(edge.aX()[0] >= 0.f && edge.aX()[0] < .01f); // Clamped to the edge, then nudged inward.
        // The spring pulling outward cannot pass the wall; momentum into the wall is dropped.
        Simulation held; BodyControls strong; strong.returnStrength = {1000}; strong.damping = {0};
        for(int i = 0; i < 120; ++i) held.update({.0f}, {.5f}, strong, {.5f}, {.5f}, passive(), margin, 1.0/60);
        assert(near(held.aX()[0], .1f));
        // Nonfinite or negative border values act as zero.
        Border invalid; invalid.margin = -1; invalid.feather = std::numeric_limits<double>::quiet_NaN();
        Simulation unaffected; unaffected.update({.02f}, {.98f}, d, {.5f}, {.5f}, passive(), invalid, .1);
        assert(unaffected.aX() == V{.02f});
    }

    // Each Body keeps its own identity; a count change in one leaves the other's momentum.
    {
        const BodyControls d = drifting();
        Simulation s;
        s.update({.45f}, {.5f}, d, {.55f}, {.5f}, d, noBorder, 1.0/60);
        s.update({.45f}, {.5f}, d, {.9f}, {.9f}, d, noBorder, 1.0/60);
        const float before = s.aX()[0];
        s.update({.45f}, {.5f}, d, {.9f,.95f}, {.9f,.95f}, d, noBorder, 1.0/60);
        assert(s.aX()[0] < before); // A continues drifting.
        assert(s.bX() == V({.9f,.95f})); // B restarted at rest.
        s.update({.45f}, {.5f}, d, {.9f,.95f}, {.9f,.95f}, d, noBorder, 1.0/60, true);
        assert(s.aX() == V{.45f} && s.bX() == V({.9f,.95f}));
        // Invalid A clears A only; B keeps simulating.
        s.update({.45f}, {.5f}, d, {.55f}, {.5f}, d, noBorder, 1.0/60);
        const float nan = std::numeric_limits<float>::quiet_NaN();
        for(const V& invalid : {V{}, V{.5f,.6f}, V{nan}, V{-1}}) {
            s.update(invalid, {.5f}, d, {.55f}, {.5f}, d, noBorder, 1.0/60);
            assert(s.aX().empty() && s.aY().empty() && s.bX().size() == 1);
        }
        const float bBefore = s.bX()[0];
        s.update({}, {}, d, {.55f}, {.5f}, d, noBorder, 1.0/60);
        assert(s.bX()[0] > bBefore); // B's momentum from the earlier push continues.
        s.update({.45f}, {.5f}, d, {.55f}, {.5f}, d, noBorder, 0);
        assert(s.aX() == V{.45f});
    }

    // Fixed stepping gives the same trajectory at different host frame rates.
    {
        Simulation slow, fast, jitter;
        for(int i = 0; i < 90; ++i) slow.update({.45f}, {.5f}, spring, {.55f}, {.52f}, spring, noBorder, 1.0/30);
        for(int i = 0; i < 432; ++i) fast.update({.45f}, {.5f}, spring, {.55f}, {.52f}, spring, noBorder, 1.0/144);
        for(int i = 0; i < 120; ++i) {
            jitter.update({.45f}, {.5f}, spring, {.55f}, {.52f}, spring, noBorder, .007);
            jitter.update({.45f}, {.5f}, spring, {.55f}, {.52f}, spring, noBorder, .018);
        }
        assert(slow.aX() == fast.aX() && slow.aY() == fast.aY() && slow.bX() == fast.bX() && slow.bY() == fast.bY());
        assert(slow.aX() == jitter.aX() && slow.bX() == jitter.bX());
        Simulation hitch, capped;
        hitch.update({.45f}, {.5f}, spring, {.55f}, {.5f}, spring, noBorder, 50);
        capped.update({.45f}, {.5f}, spring, {.55f}, {.5f}, spring, noBorder, .1);
        assert(hitch.aX() == capped.aX() && hitch.bX() == capped.bX());
        const V saved = hitch.aX();
        for(double dt : {0.0, -1.0, static_cast<double>(std::numeric_limits<float>::quiet_NaN())}) {
            hitch.update({.45f}, {.5f}, spring, {.55f}, {.5f}, spring, noBorder, dt);
            assert(hitch.aX() == saved);
        }
    }

    // Broad phase: pairs just inside reach interact, pairs just outside do not, in every direction.
    {
        const BodyControls d = drifting();
        BodyControls disk = d; disk.radius = {.05f}; // reach = 0.05 + 0.05 + 0.15 = 0.25
        for(float sign : {-1.f, 1.f}) {
            Simulation inside, outside, insideY, outsideY;
            inside.update({.5f}, {.5f}, disk, {.5f + sign*.24f}, {.5f}, disk, noBorder, 1.0/240);
            outside.update({.5f}, {.5f}, disk, {.5f + sign*.26f}, {.5f}, disk, noBorder, 1.0/240);
            insideY.update({.5f}, {.5f}, disk, {.5f}, {.5f + sign*.24f}, disk, noBorder, 1.0/240);
            outsideY.update({.5f}, {.5f}, disk, {.5f}, {.5f + sign*.24f}, disk, noBorder, 0);
            assert(inside.aX()[0] != .5f && inside.bX()[0] != .5f + sign*.24f);
            assert(outside.aX()[0] == .5f && outside.bX()[0] == .5f + sign*.26f);
            assert(insideY.aY()[0] != .5f);
            assert(outsideY.aY()[0] == .5f);
        }
        // Mixed radii: only the responding side's fear counts toward reach.
        BodyControls brave = passive(); brave.radius = {.3f};
        Simulation wide; wide.update({.5f}, {.5f}, d, {.9f}, {.5f}, brave, noBorder, 1.0/240);
        assert(wide.aX()[0] < .5f); // Surface gap 0.1 inside A's fear 0.15.
        Simulation wider; wider.update({.5f}, {.5f}, d, {.96f}, {.5f}, brave, noBorder, 1.0/240);
        assert(wider.aX()[0] == .5f);
    }

    // Coincident points part in opposite directions; Y-only offsets push along Y.
    {
        const BodyControls d = drifting();
        Simulation same; same.update({.5f}, {.5f}, d, {.5f}, {.5f}, d, noBorder, .1);
        const float dx = same.aX()[0] - .5f, dy = same.aY()[0] - .5f;
        assert(std::hypot(dx, dy) > 0);
        assert(near(same.bX()[0] - .5f, -dx) && near(same.bY()[0] - .5f, -dy));
        Simulation vertical; vertical.update({.5f}, {.45f}, d, {.5f}, {.55f}, d, noBorder, 1.0/60);
        assert(vertical.aX()[0] == .5f && vertical.aY()[0] < .45f && vertical.bY()[0] > .55f);
    }

    // Maximum controls remain bounded under prolonged motion with the border active.
    {
        BodyControls extreme; extreme.repulsion = {1000}; extreme.returnStrength = {1000};
        extreme.damping = {100}; extreme.radius = {.2f}; extreme.fear = {.5f};
        Border border; border.margin = .05; border.feather = .1;
        Simulation stress;
        for(int i = 0; i < 600; ++i) {
            const float f = .5f + .45f * std::sin(i * .02f);
            stress.update({.5f,.6f}, {f,.6f}, extreme, {f,.5f}, {.5f,f}, extreme, border, .1);
            for(const V* out : {&stress.aX(), &stress.aY(), &stress.bX(), &stress.bY()})
                for(float v : *out) assert(std::isfinite(v) && v >= .05f - 1e-6f && v <= .95f + 1e-6f);
        }
    }

    std::cout << "PASS: Body Repulsion equivalence, mutual pushes, strength, radius, per-point controls, "
                 "border wall and feather, independent identity, frame rates, broad phase, "
                 "coincidence and stress.\n";
}
