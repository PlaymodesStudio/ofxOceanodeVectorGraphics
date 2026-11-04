// Standalone checks for contour1DCore.h (no openFrameworks needed).
#include "../src/contour1DCore.h"
#include <cassert>
#include <cstdio>
#include <random>

using namespace contour1DCore;
using V = std::vector<float>;
static const double PI = 3.14159265358979323846;

struct Poly { V x, y; };

// Rotates by `angle` about (0.5, 0.5), translates by (dx, dy), starts the path
// `start` points later, closes it with a repeated point.
static Poly place(const std::vector<Vec2>& p, double angle, double dx = 0, double dy = 0, size_t start = 0) {
    Poly out;
    const double c = std::cos(angle), s = std::sin(angle);
    for(size_t k = 0; k <= p.size(); ++k) {
        const Vec2 q = p[(start + k) % p.size()];
        const double vx = q.x - .5, vy = q.y - .5;
        out.x.push_back(float(.5 + c * vx - s * vy + dx));
        out.y.push_back(float(.5 + s * vx + c * vy + dy));
    }
    return out;
}

// A body with half-width w(u) along a straight spine of length L, centred.
static std::vector<Vec2> body(double L, int n, double (*w)(double)) {
    std::vector<Vec2> p;
    for(int i = 0; i <= n; ++i) { const double u = double(i) / n; p.push_back({.5 - L / 2 + L * u, .5 - w(u)}); }
    for(int i = n - 1; i > 0; --i) { const double u = double(i) / n; p.push_back({.5 - L / 2 + L * u, .5 + w(u)}); }
    return p;
}
static double ellipseW(double u) { return .12 * std::sqrt(std::max(0.0, 1 - (2 * u - 1) * (2 * u - 1))); }
static double thinW(double u) { return .04 * std::sqrt(std::max(0.0, 1 - (2 * u - 1) * (2 * u - 1))); }
static double tadpoleW(double u) { return u < .3 ? .1 * std::sqrt(std::max(0.0, 1 - ((u - .15) / .15) * ((u - .15) / .15))) + 1e-3 : .03 * (1 - u) / .7 + 1e-3; }

static std::vector<Vec2> circle(double r, int n) {
    std::vector<Vec2> p;
    for(int i = 0; i < n; ++i) p.push_back({.5 + r * std::cos(2 * PI * i / n), .5 + r * std::sin(2 * PI * i / n)});
    return p;
}

// A banana: spine on a circle of radius R spanning `span` radians, half-width w.
static std::vector<Vec2> banana(double R, double span, double w, int n) {
    std::vector<Vec2> p;
    for(int i = 0; i <= n; ++i) { const double a = -span / 2 + span * i / n; p.push_back({.5 + (R - w) * std::cos(a), .5 + (R - w) * std::sin(a)}); }
    for(int i = n; i >= 0; --i) { const double a = -span / 2 + span * i / n; p.push_back({.5 + (R + w) * std::cos(a), .5 + (R + w) * std::sin(a)}); }
    return p;
}

static bool near(double a, double b, double e) { return std::abs(a - b) < e; }
static double maxDiff(const std::vector<double>& a, const std::vector<double>& b) {
    double m = 0;
    for(size_t i = 0; i < a.size(); ++i) m = std::max(m, std::abs(a[i] - b[i]));
    return m;
}
static std::vector<double> width(const Shape& s) {
    std::vector<double> w(s.left.size());
    for(size_t k = 0; k < w.size(); ++k) w[k] = s.left[k] + s.right[k];
    return w;
}

int main() {
    Controls c;
    c.size = 64;
    // 1. Ellipse: profile is the half-width, symmetric, and the same at any
    //    rotation, position and input start point.
    {
        const auto e = body(.6, 100, ellipseW);
        const Poly ref = place(e, 0);
        const Result r0 = analyse(ref.x, ref.y, c);
        assert(r0.valid && r0.shapes.size() == 1);
        const Shape& s = r0.shapes[0];
        assert(s.left.size() == 64 && s.right.size() == 64);
        assert(near(s.length, .6, .01));
        assert(near(s.left[32], .12, .01) && near(s.right[32], .12, .01));
        assert(s.left[0] < .03 && s.left[63] < .03);          // tips
        for(int k = 0; k < 64; ++k) assert(near(s.left[k], s.right[k], .005));
        for(int k = 0; k < 32; ++k) assert(near(s.left[k], s.left[63 - k], .01));
        for(double angle : {.3, 1.1, PI / 2, 2.5, -2.0}) {
            for(size_t start : {size_t(0), size_t(37), size_t(150)}) {
                const Poly p = place(e, angle, .1, -.2, start);
                const Result r = analyse(p.x, p.y, c);
                assert(r.valid && r.shapes.size() == 1);
                assert(maxDiff(width(r.shapes[0]), width(s)) < .01);
                assert(near(r.shapes[0].length, .6, .01));
                assert(near(r.shapes[0].centre.x, .6, 1e-3) && near(r.shapes[0].centre.y, .3, 1e-3));
            }
        }
    }
    // 2. Tadpole: Wide End puts the round head first whatever the rotation;
    //    the profile thins toward the tail. Narrow End and Flip reverse it.
    {
        const auto t = body(.6, 200, tadpoleW);
        std::vector<double> reference;
        for(double angle : {0.0, .7, PI, -1.3}) {
            const Poly p = place(t, angle, 0, 0, 55);
            const Result r = analyse(p.x, p.y, c);
            assert(r.shapes.size() == 1);
            const Shape& s = r.shapes[0];
            // Head tip is the rotated image of the left tip (.2, .5).
            const double hx = .5 + std::cos(angle) * -.3, hy = .5 + std::sin(angle) * -.3;
            assert(near(s.head.x, hx, .01) && near(s.head.y, hy, .01));
            assert(near(s.tail.x, 1 - hx, .01) && near(s.tail.y, 1 - hy, .01));
            const auto w = width(s);
            assert(w[10] > .15 && w[50] < .05 && w[50] > w[60]);
            if(reference.empty()) reference = w; else assert(maxDiff(w, reference) < .01);
        }
        Controls narrow = c; narrow.head = kNarrowEnd;
        const Poly p = place(t, .4);
        const Result rn = analyse(p.x, p.y, narrow);
        assert(near(rn.shapes[0].head.x, .5 + std::cos(.4) * .3, .01));
        Controls flipped = c; flipped.flip = true;
        const Result rf = analyse(p.x, p.y, flipped);
        assert(near(rf.shapes[0].head.x, rn.shapes[0].head.x, 1e-9));
        const auto wf = width(rf.shapes[0]), wr = width(analyse(p.x, p.y, c).shapes[0]);
        for(int k = 0; k < 64; ++k) assert(near(wf[k], wr[63 - k], 1e-9));
        // Flip does not disturb the carried orientation: the track keeps the rule's head.
        assert(near(rf.tracks[0].head.x, rn.shapes[0].tail.x, 1e-9));
    }
    // 3. Left/Right: a bump on one side shows on the matching profile, more
    //    the stiffer the spine (the midline leans toward the bump), and swaps
    //    sides when the body is mirrored. Width is the same either way.
    {
        auto bumpy = body(.6, 100, ellipseW);
        for(auto& q : bumpy) if(q.y < .5 && std::abs(q.x - .5) < .06) q.y -= .05;   // bump at the top of the screen (y down)
        Controls point = c; point.head = kPoint; point.headX = {.2f}; point.headY = {.5f};
        const Poly p = place(bumpy, 0);
        const Shape soft = analyse(p.x, p.y, point).shapes[0];
        assert(near(soft.head.x, .2, .01));
        assert(near(soft.left[32], soft.right[32], .005) && near(soft.left[32] + soft.right[32], .29, .01));
        point.spineSmooth = .5;
        const Shape s = analyse(p.x, p.y, point).shapes[0];
        // Travelling +X on screen, the top (y down) is the left.
        assert(s.left[32] > s.right[32] + .02);
        assert(near(s.left[32] + s.right[32], .29, .01));
        Poly m = p; for(auto& v : m.y) v = 1 - v;   // mirror vertically
        const Shape sm = analyse(m.x, m.y, point).shapes[0];
        assert(sm.right[32] > sm.left[32] + .02);
        assert(maxDiff(sm.left, s.right) < .01 && maxDiff(sm.right, s.left) < .01);
    }
    // 4. Hysteresis: a symmetric body with outline noise never swaps ends, and
    //    the tips follow the body as it drifts and turns.
    {
        const auto e = body(.6, 100, ellipseW);
        std::mt19937 gen(7);
        std::normal_distribution<double> noise(0, .002);
        std::vector<Track> tracks;
        Vec2 firstHead;
        for(int frame = 0; frame < 120; ++frame) {
            auto jittered = e;
            for(auto& q : jittered) { q.x += noise(gen); q.y += noise(gen); }
            const double angle = .01 * frame, dx = .001 * frame;
            const Poly p = place(jittered, angle, dx, 0, frame * 3);
            const Result r = analyse(p.x, p.y, c, &tracks);
            tracks = r.tracks;
            const Shape& s = r.shapes[0];
            const double hx = .5 + std::cos(angle) * -.3 + dx, hy = .5 + std::sin(angle) * -.3;
            if(frame == 0) firstHead = s.head;
            // Whichever end was picked first stays the head, within noise.
            const bool sameEnd = near(s.head.x, hx, .02) && near(s.head.y, hy, .02);
            const bool otherEnd = near(s.head.x, 1 - hx + 2 * dx, .02) && near(s.head.y, 1 - hy, .02);
            assert(sameEnd || otherEnd);
            static bool pickedFirst = sameEnd;
            assert(sameEnd == pickedFirst);
        }
    }
    // 5. A round blob keeps its tips while it moves: the carried pair is
    //    never beaten by a clearly longer diameter.
    {
        const auto disc = circle(.2, 200);
        std::vector<Track> tracks;
        Vec2 head0;
        for(int frame = 0; frame < 60; ++frame) {
            const Poly p = place(disc, 0, .002 * frame, .001 * frame, frame * 7);
            const Result r = analyse(p.x, p.y, c, &tracks);
            tracks = r.tracks;
            const Vec2 h = r.shapes[0].head;
            if(frame == 0) head0 = h;
            else assert(near(h.x - .002 * frame, head0.x, .01) && near(h.y - .001 * frame, head0.y, .01));
            assert(near(r.shapes[0].length, .4, .02));
        }
    }
    // 6. Motion: the tip leading the movement becomes the head.
    {
        const auto e = body(.6, 100, ellipseW);
        Controls motion = c; motion.head = kMotion;
        std::vector<Track> tracks;
        for(int frame = 0; frame < 30; ++frame) {
            const Poly p = place(e, 0, .005 * frame, 0);
            tracks = analyse(p.x, p.y, motion, &tracks).tracks;
        }
        assert(tracks[0].head.x > tracks[0].tail.x);   // moving +X: right tip leads
        for(int frame = 0; frame < 60; ++frame) {       // turn round: the other tip leads
            const Poly p = place(e, 0, .145 - .005 * frame, 0);
            tracks = analyse(p.x, p.y, motion, &tracks).tracks;
        }
        assert(tracks[0].head.x < tracks[0].tail.x);
    }
    // 7. Banana: the spine follows the bend, so the length is about the arc
    //    length (plus the diagonals from the flat caps' outer corners, which
    //    are the tips) and the profile is the constant half-width.
    {
        const auto b = banana(.3, 2.0, .05, 100);
        const Poly p = place(b, .9, 0, 0, 20);
        const Shape s = analyse(p.x, p.y, c).shapes[0];
        assert(s.length > .6 && s.length < .68);
        assert(near(detail::dist(s.head, {.5, .5}), .35, .005) && near(detail::dist(s.tail, {.5, .5}), .35, .005));
        for(int k = 12; k < 52; ++k) assert(near(s.left[k], .05, .01) && near(s.right[k], .05, .01));
        assert(s.spine.front().x == s.head.x && s.spine.back().x == s.tail.x);
    }
    // 8. Several shapes, separators, degenerate paths, Point mode per shape,
    //    bad input.
    {
        const Poly a = place(body(.6, 100, ellipseW), 0, 0, -.3);
        const Poly b = place(body(.2, 50, thinW), PI / 2, 0, .3);   // vertical, tips at y .7 and .9
        V x = a.x, y = a.y;
        x.push_back(-1); y.push_back(-1);
        x.insert(x.end(), {.1f, .1f}); y.insert(y.end(), {.1f, .1f});   // one unique point: skipped
        x.push_back(-1); y.push_back(-1);
        x.insert(x.end(), b.x.begin(), b.x.end()); y.insert(y.end(), b.y.begin(), b.y.end());
        Controls point = c; point.head = kPoint; point.headX = {.9f, .5f}; point.headY = {.2f, .9f};
        const Result r = analyse(x, y, point);
        assert(r.valid && r.shapes.size() == 2 && r.tracks.size() == 2);
        assert(near(r.shapes[0].head.x, .8, .01) && r.shapes[0].area > r.shapes[1].area);
        assert(near(r.shapes[1].head.y, .9, .01) && near(r.shapes[1].head.x, .5, .01));
        assert(near(r.shapes[1].length, .2, .01));
        V bad = x; bad.pop_back();
        assert(!analyse(bad, y, c).valid);
        V nan = x; nan[3] = std::nanf("");
        assert(!analyse(nan, y, c).valid);
        assert(analyse({}, {}, c).valid && analyse({}, {}, c).shapes.empty());
    }
    // 9. Smoothing keeps the ends and the size; tiny Size and Resample are clamped.
    {
        const Poly p = place(body(.6, 100, tadpoleW), 0);
        Controls smooth = c; smooth.smooth = .2; smooth.spineSmooth = .5;
        const Shape s = analyse(p.x, p.y, smooth).shapes[0];
        assert(s.left.size() == 64 && near(s.head.x, .2, .01));
        Controls tiny = c; tiny.size = 1; tiny.resample = 1;
        const Shape t = analyse(p.x, p.y, tiny).shapes[0];
        assert(t.left.size() == 2 && t.spine.size() >= 2);
    }
    std::puts("contour1DCore tests passed");
    return 0;
}
