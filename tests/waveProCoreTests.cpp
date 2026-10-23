// Standalone checks for waveProCore.h (no openFrameworks needed).
#include "../src/waveProCore.h"
#include <cassert>
#include <cstdio>

using namespace waveProCore;
using V = std::vector<float>;
static const double PI = 3.14159265358979323846;

static void circle(V& x, V& y, int n, double r = .25, bool ccw = true) {
    x.clear(); y.clear();
    for(int i = 0; i <= n; ++i) {   // repeated endpoint closes the loop
        const double a = 2 * PI * (i % n) / n * (ccw ? 1 : -1);
        x.push_back(float(.5 + r * std::cos(a)));
        y.push_back(float(.5 + r * std::sin(a)));
    }
}
static double radius(float x, float y) { return std::hypot(x - .5, y - .5); }
static V sine(int n, double cycles) {
    V w(n);
    for(int i = 0; i < n; ++i) w[i] = float(.5 + .5 * std::sin(2 * PI * cycles * i / n));
    return w;
}
static bool near(double a, double b, double e = 1e-4) { return std::abs(a - b) < e; }

int main() {
    V x, y;
    // 1. A 1000-sample, 3-cycle sine resampled onto 100 vertices: outward, any winding.
    for(bool ccw : {true, false}) {
        circle(x, y, 100, .25, ccw);
        Controls c; c.wave = sine(1000, 3); c.amount = {.05f};
        const auto r = direct(x, y, c);
        assert(r.x.size() == x.size());
        for(int i = 0; i < 100; ++i)
            // Stable anchor runs one fixed direction, so a reversed circle reads the wave backwards.
            assert(near(radius(r.x[i], r.y[i]), .25 + .05 * std::sin((ccw ? 1 : -1) * 2 * PI * 3 * i / 100), 2e-4));
        assert(r.x.back() == r.x.front() && r.y.back() == r.y.front());
    }
    // 2. Open path: endpoints read the first/last wave sample; positive is left of travel.
    {
        V ox{0, .5f, 1}, oy{0, 0, 0};
        Controls c; c.wave = {0, 1}; c.amount = {.1f};
        auto r = direct(ox, oy, c);
        assert(near(r.y[0], -.1) && near(r.y[1], 0) && near(r.y[2], .1));
        c.waveRepeat = {2};               // tiled: 0,1 | 0,1
        r = direct(ox, oy, c);
        assert(near(r.y[0], -.1) && near(r.y[2], .1));
    }
    // 3. Separators, per-shape Wave Offset, and per-perimeter Amount.
    {
        V a, b, ax, ay; circle(a, b, 64); circle(ax, ay, 64, .1);
        x = a; y = b; x.push_back(-1); y.push_back(-1);
        x.insert(x.end(), ax.begin(), ax.end()); y.insert(y.end(), ay.begin(), ay.end());
        Controls c; c.wave = sine(256, 1); c.amount = {.02f}; c.waveOffset = {0, .25f};
        const auto r = direct(x, y, c);
        assert(r.x[65] == -1 && r.y[65] == -1);
        assert(near(radius(r.x[0], r.y[0]), .25, 1e-5));          // sin(0)
        assert(near(radius(r.x[66], r.y[66]), .1 + .02, 1e-4));   // shifted a quarter: sin(pi/2)
    }
    // 4. Normal Smooth removes the spike at a square corner.
    {
        V sx{0, 1, 1, 0, 0}, sy{0, 0, 1, 1, 0};
        Controls c; c.wave = {1}; c.amount = {.1f};
        auto hard = direct(sx, sy, c);
        c.smooth = {.5f};
        auto soft = direct(sx, sy, c);
        assert(near(hard.x[1], 1 + .1 / std::sqrt(2)));          // corner bisector
        assert(near(soft.x[1], hard.x[1]));                       // symmetric corner stays put
        V lx{0, .5f, 1, 1}, ly{0, 0, 0, .5f};                     // open L: corner bends, ends smoothed
        c.smooth = {0}; hard = direct(lx, ly, c);
        c.smooth = {1}; soft = direct(lx, ly, c);
        assert(!near(soft.x[2], hard.x[2]) && !near(soft.y[3], hard.y[3]));
    }
    // 4b. Stable anchor: rotating the start point or reversing the path changes nothing.
    {
        V bx, by; circle(bx, by, 90);
        for(int i = 0; i < 90; ++i) { const double k = 1 + .3 * std::sin(3 * 2 * PI * i / 90);  // lumpy blob
            bx[i] = float(.5 + (bx[i] - .5) * k); by[i] = float(.5 + (by[i] - .5) * k); }
        bx[90] = bx[0]; by[90] = by[0];
        Controls c; c.wave = sine(1024, 16); c.amount = {.02f};
        const auto ref = direct(bx, by, c);
        auto lookup = [&](const Result& r, float px, float py) {
            for(size_t i = 0; i < bx.size(); ++i) if(bx[i] == px && by[i] == py) return Vec2{r.x[i], r.y[i]};
            assert(false); return Vec2{};
        };
        for(int shift : {1, 17, 45}) for(bool reverse : {false, true}) {
            V rx, ry;
            for(int i = 0; i < 90; ++i) {
                const int k = reverse ? (shift - i + 900) % 90 : (shift + i) % 90;
                rx.push_back(bx[k]); ry.push_back(by[k]);
            }
            rx.push_back(rx[0]); ry.push_back(ry[0]);
            const auto r = direct(rx, ry, c);
            for(size_t i = 0; i < rx.size(); ++i) {
                const Vec2 e = lookup(ref, rx[i], ry[i]);
                assert(near(r.x[i], e.x, 1e-5) && near(r.y[i], e.y, 1e-5));
            }
        }
        c.anchor = kPathStart;                     // Path Start does depend on the start point
        V rx(bx.begin() + 17, bx.end() - 1), ry(by.begin() + 17, by.end() - 1);
        rx.insert(rx.end(), bx.begin(), bx.begin() + 18); ry.insert(ry.end(), by.begin(), by.begin() + 18);
        const auto r = direct(rx, ry, c);
        const Vec2 e = lookup(direct(bx, by, c), rx[0], ry[0]);
        assert(!near(r.x[0], e.x, 1e-4) || !near(r.y[0], e.y, 1e-4));
    }
    // 4c. Resample: steady count, anchored start, closure kept, wave independent of input density.
    {
        V cx, cy; circle(cx, cy, 37);
        Controls c; c.wave = sine(1024, 16); c.amount = {.02f}; c.resample = 256;
        const auto a = direct(cx, cy, c);
        assert(a.x.size() == 257 && a.x.front() == a.x.back() && a.y.front() == a.y.back());
        circle(cx, cy, 53, .25, false);                      // other density, other winding
        const auto b = direct(cx, cy, c);
        assert(b.x.size() == 257);
        for(size_t i = 0; i < a.x.size(); ++i)                // same samples, up to chord error
            assert(near(a.x[i], b.x[i], 3e-3) && near(a.y[i], b.y[i], 3e-3));
        V ox{0, .3f, 1}, oy{0, 0, 0}; c.resample = 11;        // open: endpoints kept
        const auto o = direct(ox, oy, c);
        assert(o.x.size() == 11 && o.x.front() == 0 && o.x.back() == 1);
    }
    // 4d. Tracked anchor follows a rotating C shape; Stable hops when the
    //     opening faces +X and the centroid ray leaves through the gap.
    {
        auto cShape = [](double turn, V& ox, V& oy) {
            ox.clear(); oy.clear();
            const int arc = 120; const double open = 1.2;   // radians missing
            for(int i = 0; i <= arc; ++i) {                 // outer edge
                const double a = turn + open / 2 + (2 * PI - open) * i / arc;
                ox.push_back(float(.5 + .3 * std::cos(a))); oy.push_back(float(.5 + .3 * std::sin(a)));
            }
            for(int i = arc; i >= 0; --i) {                 // inner edge, back
                const double a = turn + open / 2 + (2 * PI - open) * i / arc;
                ox.push_back(float(.5 + .2 * std::cos(a))); oy.push_back(float(.5 + .2 * std::sin(a)));
            }
            ox.push_back(ox[0]); oy.push_back(oy[0]);
        };
        for(int mode : {kStable, kTracked}) {
            Controls c; c.amount = {0}; c.resample = 256; c.anchor = mode;
            Simulation sim; double worst = 0, px = 0, py = 0;
            for(int f = 0; f < 600; ++f) {
                V ox, oy; cShape(2 * PI * f / 600, ox, oy);
                const auto r = sim.update(ox, oy, c, false, 1.0 / 60);
                if(f) worst = std::max(worst, std::hypot(r.x[0] - px, r.y[0] - py));
                px = r.x[0]; py = r.y[0];
            }
            // One frame turns the C by 0.6 degrees: about 0.003 at radius 0.3.
            if(mode == kTracked) assert(worst < .01);
            else assert(worst > .1);
        }
    }
    // 4e. Distribute By Length: whole cycles shared in proportion to perimeter.
    {
        auto ring = [](double cx, double cy, double r, int n, V& ox, V& oy) {
            for(int i = 0; i <= n; ++i) {
                const double a = 2 * PI * (i % n) / n;
                ox.push_back(float(cx + r * std::cos(a))); oy.push_back(float(cy + r * std::sin(a)));
            }
        };
        auto twoBlobs = [&](double big, double small, bool smallFirst, V& ox, V& oy) {
            ox.clear(); oy.clear();
            V bx, by, sx, sy;
            ring(.3, .5, big, 400, bx, by); ring(.75, .5, small, 400, sx, sy);
            V& fx = smallFirst ? sx : bx; V& fy = smallFirst ? sy : by;
            V& lx = smallFirst ? bx : sx; V& ly = smallFirst ? by : sy;
            ox = fx; oy = fy; ox.push_back(-1); oy.push_back(-1);
            ox.insert(ox.end(), lx.begin(), lx.end()); oy.insert(oy.end(), ly.begin(), ly.end());
        };
        // Cycles of one shape = sign changes of its radial displacement / 2.
        auto cyclesOf = [](const Result& r, const V& ox, const V& oy, size_t begin, size_t end, double cx, double cy) {
            int changes = 0, last = 0;
            for(size_t i = begin; i < end; ++i) {
                const double d = std::hypot(r.x[i] - cx, r.y[i] - cy) - std::hypot(ox[i] - cx, oy[i] - cy);
                const int sign = d > 1e-6 ? 1 : (d < -1e-6 ? -1 : 0);
                if(sign && last && sign != last) ++changes;
                if(sign) last = sign;
            }
            return changes / 2.0;
        };
        V bx, by; twoBlobs(.2, .1, false, bx, by);
        Controls c; c.wave = sine(1200, 12); c.amount = {.01f}; c.waveCycles = 12;
        auto r = direct(bx, by, c);                                   // Per Shape: 12 + 12
        assert(near(cyclesOf(r, bx, by, 0, 401, .3, .5), 12, .51) && near(cyclesOf(r, bx, by, 402, 803, .75, .5), 12, .51));
        c.distribute = kByLength; r = direct(bx, by, c);             // perimeters 2:1 -> 8 + 4
        assert(near(cyclesOf(r, bx, by, 0, 401, .3, .5), 8, .51) && near(cyclesOf(r, bx, by, 402, 803, .75, .5), 4, .51));
        c.waveCycles = 1; r = direct(bx, by, c);                     // too few: the small blob gets none
        for(size_t i = 402; i < 803; ++i) assert(near(r.x[i], bx[i], 1e-6) && near(r.y[i], by[i], 1e-6));
        // Hysteresis: the small blob breathes across the 3/4 cycle boundary without flickering,
        // keeps its share when the input lists it first, and changes once it has clearly grown.
        c.waveCycles = 9; c.wave = sine(900, 9);
        Simulation sim; int flips = 0, lastShare = -1;
        auto smallShare = [&]() {
            for(const auto& st : sim.cycleShares()) if(st.centre.x > .5) return st.cycles;
            return -1;
        };
        for(int f = 0; f < 240; ++f) {
            const double small = .1273 - .01 * std::cos(f * .2);       // ideal share 3.33 .. 3.68, starts low
            V ox, oy; twoBlobs(.2, small, f >= 120, ox, oy);           // order swaps halfway
            sim.update(ox, oy, c, false, 1.0 / 60);
            int total = 0; for(const auto& st : sim.cycleShares()) total += st.cycles;
            assert(total == 9);
            if(lastShare >= 0 && smallShare() != lastShare) ++flips;
            lastShare = smallShare();
        }
        assert(flips == 0 && lastShare == 3);
        V ox, oy; twoBlobs(.2, .16, false, ox, oy);                    // ideal share 4.0
        sim.update(ox, oy, c, false, 1.0 / 60);
        assert(smallShare() == 4);
    }
    // 5. Invalid input clears, mismatched sizes clear.
    assert(direct({0, 1}, {0}, {}).x.empty());
    assert(direct({0, -1}, {0, 0}, {}).x.empty());

    // 6. Spring settles on the wave target; Damping 1 never overshoots.
    {
        circle(x, y, 64);
        Controls c; c.wave = {1}; c.amount = {.05f}; c.stiffness = {100}; c.damping = {1};
        Simulation s; double peak = 0, last = 0;
        Controls flat = c; flat.wave = {.5f};
        s.update(x, y, flat, true, 1.0 / 60);      // membrane starts on a flat wave...
        for(int f = 0; f < 600; ++f) {
            const auto r = s.update(x, y, c, true, 1.0 / 60);   // ...then the wave steps up
            last = radius(r.x[0], r.y[0]); peak = std::max(peak, last);
            if(f == 0) assert(last < .26);
        }
        assert(near(last, .3, 1e-5) && peak <= .3 + 1e-6);
    }
    // 7. Damping 0 rings without numerical loss; larger damping decays faster.
    {
        circle(x, y, 32);
        Controls c; c.wave = {.5f}; c.amount = {0}; c.stiffness = {50}; c.damping = {0};
        Simulation s; s.update(x, y, c, false, 1.0 / 60);
        Controls kick = c; kick.wave = {1}; kick.amount = {.05f};
        s.update(x, y, kick, false, 1.0 / 60);   // start displaced at rest
        double late = 0;
        for(int f = 0; f < 1200; ++f) {
            const auto r = s.update(x, y, c, true, 1.0 / 60);
            if(f > 1000) late = std::max(late, std::abs(radius(r.x[0], r.y[0]) - .25));
        }
        assert(late > .049);
    }
    // 8. Frame-rate independence: 60 Hz and 240 Hz agree after one second.
    {
        circle(x, y, 64);
        Controls c; c.wave = sine(64, 2); c.amount = {.04f}; c.stiffness = {60}; c.damping = {.3f};
        c.propagation = {1};
        Simulation a, b; Result ra, rb;
        Controls flat = c; flat.wave = {.5f};
        a.update(x, y, flat, true, 1.0 / 60); b.update(x, y, flat, true, 1.0 / 240);
        for(int f = 0; f < 60; ++f) ra = a.update(x, y, c, true, 1.0 / 60);
        for(int f = 0; f < 240; ++f) rb = b.update(x, y, c, true, 1.0 / 240);
        for(size_t i = 0; i < ra.x.size(); ++i)
            assert(near(radius(ra.x[i], ra.y[i]), radius(rb.x[i], rb.y[i]), 2e-3));
    }
    // 9. Propagation: a disturbance travels along the contour; without it, it stays.
    {
        const int n = 400; circle(x, y, n);
        V bump(n);
        for(int i = 0; i < n; ++i) {
            const double d = std::min(i, n - i) / double(n);
            bump[i] = float(.5 + .5 * std::exp(-(d / .03) * (d / .03)));
        }
        Controls c; c.wave = bump; c.amount = {.05f}; c.stiffness = {0}; c.damping = {0};
        for(double speed : {0.0, 1.0}) {
            Simulation s; s.update(x, y, c, false, 1.0 / 60);       // start shaped like the bump
            Controls run = c; run.wave = {.5f}; run.propagation = {float(speed)};  // ...then released
            Result r;
            for(int f = 0; f < 15; ++f) r = s.update(x, y, run, true, 1.0 / 60); // 0.25 s
            const double at = radius(r.x[100], r.y[100]) - .25;    // quarter perimeter away
            if(speed == 0) assert(near(at, 0, 1e-9));
            else assert(at > .005);
        }
    }
    // 9a. Damping also calms fine travelling ripples: with propagation on, a
    //     16-cycle disturbance decays at least as fast as the spring alone would let it.
    {
        circle(x, y, 512);
        Controls c; c.wave = {.5f}; c.amount = {.03f}; c.stiffness = {40}; c.damping = {.5f};
        c.propagation = {2};
        Simulation s; s.update(x, y, c, true, 1.0 / 60);
        Controls kick = c; kick.wave = sine(1024, 16);
        s.update(x, y, kick, false, 1.0 / 60);      // displaced by the fine wave, then released
        Result r;
        for(int f = 0; f < 60; ++f) r = s.update(x, y, c, true, 1.0 / 60);
        double worst = 0;
        for(size_t i = 0; i < r.x.size(); ++i) worst = std::max(worst, std::abs(radius(r.x[i], r.y[i]) - .25));
        assert(worst < .03 * .05);                  // under 5% left after one second
    }
    // 9b. Propagation does not flatten the wave: the outline still settles on it.
    {
        circle(x, y, 512);
        Controls c; c.wave = sine(1024, 16); c.amount = {.03f}; c.stiffness = {40}; c.damping = {1};
        c.propagation = {2};
        Simulation s; Result r;
        for(int f = 0; f < 600; ++f) r = s.update(x, y, c, true, 1.0 / 60);
        const auto target = direct(x, y, c);
        for(size_t i = 0; i < r.x.size(); ++i)
            assert(near(r.x[i], target.x[i], 1e-4) && near(r.y[i], target.y[i], 1e-4));
    }
    // 10. Stability with extreme values and long frames.
    {
        circle(x, y, 200);
        Controls c; c.wave = sine(7, 3); c.amount = {1}; c.stiffness = {1000}; c.damping = {0};
        c.propagation = {20}; c.motion = {1}; c.smooth = {.3f};
        Simulation s;
        for(int f = 0; f < 200; ++f) {
            const auto r = s.update(x, y, c, true, f % 7 ? 1.0 / 30 : 5.0);
            for(float v : r.x) assert(std::isfinite(v));
        }
    }
    // 11. Motion Influence reacts to acceleration: the front lags while the body
    //     speeds up, the outline relaxes at constant speed, and overshoots on stopping.
    {
        Controls c; c.amount = {0}; c.stiffness = {40}; c.damping = {.5f}; c.motion = {1};
        Simulation s; double pos = 0, vel = 0, duringStart = 0, cruising = 0, stopping = 0;
        auto front = [&](double shift) {
            circle(x, y, 64);
            for(auto& v : x) v += float(shift);
            const auto r = s.update(x, y, c, true, 1.0 / 60);
            return r.x[0] - x[0];                              // offset of the +X point
        };
        for(int f = 0; f < 600; ++f) {
            const double accel = f < 30 ? 1 : (f >= 400 && f < 430 ? -1 : 0);   // 0.5 s pushes
            vel += accel / 60; pos += vel / 60;
            const double o = front(pos);
            if(f == 25) duringStart = o;
            if(f == 399) cruising = o;
            if(f == 425) stopping = o;
        }
        assert(duringStart < -.005);              // pressed in while accelerating
        assert(std::abs(cruising) < 1e-4);        // constant speed: no deformation
        assert(stopping > .005);                  // bulges forward when braking
        Controls off = c; off.motion = {0};       // no influence: no deformation at all
        Simulation q; circle(x, y, 64); q.update(x, y, off, true, 1.0 / 60);
        for(auto& v : x) v += .1f;
        const auto r = q.update(x, y, off, true, 1.0 / 60);
        assert(r.x == x);
    }
    // 11a. Plastic membrane (Stiffness 0) under strong motion stays within half the body size.
    {
        Controls c; c.amount = {0}; c.stiffness = {0}; c.damping = {0}; c.motion = {1};
        Simulation s; double worst = 0;
        for(int f = 0; f < 1200; ++f) {
            const double t = f / 60.0;
            circle(x, y, 64, .1);
            for(auto& v : x) v += float(.3 * std::sin(3 * t));
            const auto r = s.update(x, y, c, true, 1.0 / 60);
            for(size_t i = 0; i < r.x.size(); ++i) worst = std::max(worst, (double)std::hypot(r.x[i] - x[i], r.y[i] - y[i]));
        }
        assert(worst <= .05 + 1e-6 && worst > .01);    // radius .1: stops at .05
    }
    // 11b. Motion Influence ignores per-point outline noise: shaking points
    //      without moving the body leaves the membrane at rest.
    {
        Controls c; c.amount = {0}; c.stiffness = {40}; c.damping = {.3f}; c.motion = {1};
        Simulation s; double worst = 0;
        for(int f = 0; f < 300; ++f) {
            circle(x, y, 128);
            for(size_t i = 0; i + 1 < x.size(); ++i) {   // tangential jitter, alternating
                const double a = 2 * PI * i / 128, j = 1e-4 * ((f + i) % 2 ? 1 : -1);
                x[i] += float(-std::sin(a) * j); y[i] += float(std::cos(a) * j);
            }
            x.back() = x[0]; y.back() = y[0];
            const auto r = s.update(x, y, c, true, 1.0 / 60);
            for(size_t i = 0; i < r.x.size(); ++i) worst = std::max(worst, (double)std::hypot(r.x[i] - x[i], r.y[i] - y[i]));
        }
        assert(worst < 1e-5);
    }
    // 12. Topology change resets state; duplicates share a position.
    {
        V dx{0, .5f, .5f, 1}, dy{0, 0, 0, 0};
        Controls c; c.wave = {1}; c.amount = {.1f};
        const auto r = direct(dx, dy, c);
        assert(r.x[1] == r.x[2] && r.y[1] == r.y[2]);
    }
    std::puts("Wave Pro core: resampling, winding, open/closed, per-shape offset/repeat, smoothing, "
              "cycle distribution, spring, damping, frame-rate independence, propagation, stability, motion passed.");
}
