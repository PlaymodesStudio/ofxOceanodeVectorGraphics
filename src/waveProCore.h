#pragma once

// Wave Pro core: displaces XY contours along their normals with a waveform,
// optionally through a damped elastic membrane (springs + tension along the
// contour) so ripples travel around each shape. No openFrameworks dependency.

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace waveProCore {

struct Vec2 { double x = 0, y = 0; };
constexpr double kPi = 3.14159265358979323846;

// Where closed shapes start reading the wave. Path Start trusts the input order.
// Stable and Tracked always run clockwise on screen (y down). Stable starts
// where a +X ray from the centroid leaves the outline; Tracked starts there
// only when a shape first appears, then follows the body from frame to frame.
enum AnchorMode { kPathStart = 0, kStable = 1, kTracked = 2 };

// A shape's anchor point and pose, carried between frames by Simulation.
struct AnchorState { Vec2 point, centre; double angle = 0, aniso = 0, radius = 0; };

// How Wave In is shared between separate shapes.
enum DistributeMode { kPerShape = 0, kByLength = 1 };

// A shape's share of the wave cycles, carried between frames by Simulation.
struct CycleState { Vec2 centre; double radius = 0; int cycles = 0; };

struct Controls {
    std::vector<float> wave, waveOffset, waveRepeat;     // offset/repeat: one value per shape
    std::vector<float> amount, smooth;                   // mapped along each perimeter
    std::vector<float> stiffness, damping, propagation, motion;
    int anchor = kStable;     // AnchorMode
    // By Length: Wave In holds waveCycles cycles, shared out in whole cycles
    // between shapes in proportion to their perimeters, so all shapes together
    // read the buffer once with a similar wavelength.
    int distribute = kPerShape;
    int waveCycles = 1;
    // Points per shape after even arc-length resampling (0 = keep input points).
    // Fixed sample positions, starting at the anchor, stop the wave shimmering
    // when the input's points slide along the outline (e.g. MetaballsV3), and
    // keep the point count steady so Spring state survives.
    int resample = 0;
};

// Linear interpolation of a control vector at normalized position t.
// Closed shapes read the vector as periodic (last sample blends into first).
inline double sampleAlong(const std::vector<float>& v, double t, bool closed,
                          double lo, double hi, double fallback) {
    if(v.empty()) return fallback;
    auto at = [&](size_t i) {
        const double s = v[i];
        return std::isfinite(s) ? std::min(hi, std::max(lo, s)) : fallback;
    };
    if(v.size() == 1) return at(0);
    const size_t n = v.size();
    const double p = std::min(1.0, std::max(0.0, t)) * (closed ? n : n - 1);
    size_t a = static_cast<size_t>(p);
    const double f = p - a;
    if(closed) { a %= n; return at(a) + (at((a + 1) % n) - at(a)) * f; }
    a = std::min(a, n - 1);
    return at(a) + (at(std::min(a + 1, n - 1)) - at(a)) * f;
}

// Per-shape controls cycle when there are more shapes than values.
inline double samplePerShape(const std::vector<float>& v, size_t shape,
                             double lo, double hi, double fallback) {
    if(v.empty()) return fallback;
    const double s = v[shape % v.size()];
    return std::isfinite(s) ? std::min(hi, std::max(lo, s)) : fallback;
}

// One separated path, with adjacent duplicates collapsed into unique nodes.
struct Shape {
    bool closed = false;
    std::vector<size_t> slots;          // original index -> local node, see Frame::slotNode
    std::vector<Vec2> pos;              // unique node positions
    std::vector<double> gap;            // normalized length node j -> j+1 (wraps when closed)
    std::vector<Vec2> normal;
    std::vector<double> target, stiffness, damping, speed, motion;
    double area = 0;                    // shoelace (twice the signed area), closed shapes
    double perimeter = 0;               // in input coordinates
};

struct Frame {
    bool valid = false;
    std::vector<float> x, y;
    std::vector<Shape> shapes;
    // For every input slot: owning shape and node, or npos for separators/degenerate paths.
    static constexpr size_t npos = std::numeric_limits<size_t>::max();
    std::vector<size_t> slotShape, slotNode;
    std::vector<AnchorState> anchors;   // one per anchored closed shape, any order
    std::vector<CycleState> cycles;     // By Length: one per shape that received a share
};

namespace detail {

inline Vec2 unit(double x, double y) {
    const double l = std::hypot(x, y);
    return l > 0 ? Vec2{x / l, y / l} : Vec2{0, 0};
}

// Box-filter normals over an arc-length window (fraction of perimeter per node).
inline void smoothNormals(Shape& s, const std::vector<double>& u, const std::vector<double>& width) {
    const size_t n = s.pos.size();
    bool any = false;
    for(double w : width) any |= w > 0;
    if(!any || n < 3) return;
    // Node weight = half of its adjacent gaps (its share of the contour).
    std::vector<double> w(n);
    for(size_t i = 0; i < n; ++i) {
        const double left = (s.closed || i > 0) ? s.gap[(i + n - 1) % n] : 0;
        const double right = (s.closed || i + 1 < n) ? s.gap[i] : 0;
        w[i] = .5 * (left + right);
    }
    // Closed shapes are unrolled three times so windows can wrap.
    const size_t copies = s.closed ? 3 : 1;
    const size_t m = n * copies;
    std::vector<double> pos(m), px(m + 1, 0), py(m + 1, 0);
    for(size_t k = 0; k < m; ++k) {
        const size_t i = k % n;
        pos[k] = u[i] + (s.closed ? double(k / n) - 1.0 : 0.0);
        px[k + 1] = px[k] + s.normal[i].x * w[i];
        py[k + 1] = py[k] + s.normal[i].y * w[i];
    }
    std::vector<Vec2> out(n);
    for(size_t i = 0; i < n; ++i) {
        const double half = .5 * std::min(1.0, width[i]);
        const size_t centre = s.closed ? i + n : i;
        if(half <= 0) { out[i] = s.normal[i]; continue; }
        const size_t lo = std::lower_bound(pos.begin(), pos.end(), pos[centre] - half) - pos.begin();
        const size_t hi = std::upper_bound(pos.begin(), pos.end(), pos[centre] + half) - pos.begin();
        const Vec2 sum{px[hi] - px[lo], py[hi] - py[lo]};
        const Vec2 n2 = unit(sum.x, sum.y);
        out[i] = (n2.x == 0 && n2.y == 0) ? s.normal[i] : n2;
    }
    s.normal = out;
}

// Arc position (0-1) where a +X ray from the area centroid leaves the outline.
// Depends only on the geometry, so it ignores where the input path starts.
inline Vec2 centroid(const Shape& s, double area) {
    const size_t n = s.pos.size();
    const Vec2 o = s.pos[0];
    Vec2 centre{0, 0};
    if(std::abs(area) > 0) {
        for(size_t j = 0; j < n; ++j) {
            const Vec2 a{s.pos[j].x - o.x, s.pos[j].y - o.y};
            const Vec2 b{s.pos[(j + 1) % n].x - o.x, s.pos[(j + 1) % n].y - o.y};
            const double cross = a.x * b.y - b.x * a.y;
            centre.x += (a.x + b.x) * cross;
            centre.y += (a.y + b.y) * cross;
        }
        return {o.x + centre.x / (3 * area), o.y + centre.y / (3 * area)};
    }
    for(const auto& p : s.pos) { centre.x += p.x / n; centre.y += p.y / n; }
    return centre;
}

inline double stableStart(const Shape& s, const std::vector<double>& u, double area) {
    const size_t n = s.pos.size();
    const Vec2 centre = centroid(s, area);
    double best = -std::numeric_limits<double>::infinity(), start = 0;
    for(size_t j = 0; j < n; ++j) {
        const Vec2 a = s.pos[j], b = s.pos[(j + 1) % n];
        const double da = a.y - centre.y, db = b.y - centre.y;
        if(da == db || (da > 0 && db > 0) || (da < 0 && db < 0)) continue;
        const double f = da / (da - db);
        const double x = a.x + (b.x - a.x) * f;
        if(x > centre.x && x > best) { best = x; start = u[j] + s.gap[j] * f; }
    }
    if(best > -std::numeric_limits<double>::infinity()) return start;
    for(size_t j = 0; j < n; ++j)        // centroid outside the shape: rightmost vertex
        if(s.pos[j].x > best) { best = s.pos[j].x; start = u[j]; }
    return start;
}

// Collapses duplicates, detects closure and measures one path. Fills pos,
// closed, gap (normalized), arc positions u, shoelace area and slot mapping.
inline bool parseShape(const std::vector<float>& x, const std::vector<float>& y,
                       size_t begin, size_t end, Shape& s, std::vector<size_t>& local,
                       std::vector<double>& u, double& area) {
    for(size_t i = begin; i < end; ++i) {
        if(s.pos.empty() || s.pos.back().x != x[i] || s.pos.back().y != y[i])
            s.pos.push_back({x[i], y[i]});
        local.push_back(s.pos.size() - 1);
    }
    if(s.pos.size() < 2) return false;
    if(s.pos.size() > 3 && s.pos.front().x == s.pos.back().x && s.pos.front().y == s.pos.back().y) {
        s.closed = true;
        const size_t dup = s.pos.size() - 1;
        for(auto& l : local) if(l == dup) l = 0;
        s.pos.pop_back();
    }
    const size_t n = s.pos.size();
    const size_t segments = s.closed ? n : n - 1;
    s.gap.assign(n, 0);
    double perimeter = 0;
    for(size_t j = 0; j < segments; ++j) {
        const Vec2 a = s.pos[j], b = s.pos[(j + 1) % n];
        s.gap[j] = std::hypot(b.x - a.x, b.y - a.y);
        perimeter += s.gap[j];
    }
    if(!(perimeter > 0) || !std::isfinite(perimeter)) return false;
    s.perimeter = perimeter;
    u.assign(n, 0);
    for(size_t j = 0; j < segments; ++j) s.gap[j] /= perimeter;
    for(size_t j = 1; j < n; ++j) u[j] = u[j - 1] + s.gap[j - 1];
    area = 0;
    if(s.closed) for(size_t j = 0; j < n; ++j) {
        const Vec2 a = s.pos[j], b = s.pos[(j + 1) % n], o = s.pos[0];
        area += (a.x - o.x) * (b.y - o.y) - (b.x - o.x) * (a.y - o.y);
    }
    return true;
}

// Point at normalized arc position t on a parsed shape.
inline Vec2 pointAt(const Shape& s, const std::vector<double>& u, double t) {
    const size_t n = s.pos.size();
    size_t j = std::upper_bound(u.begin(), u.end(), t) - u.begin();
    j = j == 0 ? 0 : j - 1;
    if(!s.closed && j + 1 >= n) return s.pos[n - 1];
    const double f = s.gap[j] > 0 ? std::min(1.0, std::max(0.0, (t - u[j]) / s.gap[j])) : 0;
    const Vec2 a = s.pos[j], b = s.pos[(j + 1) % n];
    return {a.x + (b.x - a.x) * f, a.y + (b.y - a.y) * f};
}

// Arc position of the outline point nearest to p.
inline double nearestArc(const Shape& s, const std::vector<double>& u, Vec2 p) {
    const size_t n = s.pos.size();
    double best = std::numeric_limits<double>::infinity(), arc = 0;
    for(size_t j = 0; j < n; ++j) {
        const Vec2 a = s.pos[j], b = s.pos[(j + 1) % n];
        const double ex = b.x - a.x, ey = b.y - a.y, l = ex * ex + ey * ey;
        const double t = l > 0 ? std::min(1.0, std::max(0.0, ((p.x - a.x) * ex + (p.y - a.y) * ey) / l)) : 0;
        const double d = std::hypot(p.x - a.x - t * ex, p.y - a.y - t * ey);
        if(d < best) { best = d; arc = u[j] + s.gap[j] * t; }
    }
    return arc - std::floor(arc);
}

// Centre, long-axis angle (mod pi), elongation (0 round - 1 line) and size.
// Averages over the whole outline, so it is far steadier than any one point.
inline AnchorState measurePose(const Shape& s, double area) {
    const size_t n = s.pos.size();
    AnchorState a;
    a.centre = centroid(s, area);
    double cxx = 0, cyy = 0, cxy = 0, total = 0;
    for(size_t j = 0; j < n; ++j) {   // second moments of the outline, length weighted
        const double w = .5 * (s.gap[j] + s.gap[(j + n - 1) % n]);
        const double dx = s.pos[j].x - a.centre.x, dy = s.pos[j].y - a.centre.y;
        cxx += w * dx * dx; cyy += w * dy * dy; cxy += w * dx * dy; total += w;
    }
    if(total > 0) { cxx /= total; cyy /= total; cxy /= total; }
    const double spread = std::hypot(.5 * (cxx - cyy), cxy), trace = cxx + cyy;
    a.angle = .5 * std::atan2(2 * cxy, cxx - cyy);
    a.aniso = trace > 0 ? 2 * spread / trace : 0;
    a.radius = std::sqrt(std::max(0.0, trace));
    return a;
}

// Rotation can only be measured on elongated shapes: fade it out when round.
inline double rotationWeight(double aniso) {
    return std::min(1.0, std::max(0.0, (aniso - .05) / .1));
}

// Start arc position for a closed shape, recording its anchor for next frame.
// Tracked: last frame's anchor of the nearest matching shape is moved with
// the body (centroid translation, plus principal-axis rotation when the
// shape is elongated enough for the axis to be meaningful), then snapped to
// the nearest outline point. It moves continuously with the body, never
// re-choosing a location from scratch, so bends and turns cannot make it hop.
inline double chooseStart(const Shape& s, const std::vector<double>& u, double area, int mode,
                          const std::vector<AnchorState>* hints, std::vector<AnchorState>& out) {
    AnchorState a = measurePose(s, area);

    const AnchorState* match = nullptr;
    if(mode == kTracked && hints) {
        double best = std::numeric_limits<double>::infinity();
        for(const auto& h : *hints) {
            const double d = std::hypot(h.centre.x - a.centre.x, h.centre.y - a.centre.y);
            if(d < best && d < std::max(a.radius, h.radius)) { best = d; match = &h; }
        }
    }
    double start;
    if(match) {
        // The axis has no direction: take the turn closest to last frame's.
        double turn = a.angle - match->angle;
        turn -= kPi * std::round(turn / kPi);
        a.angle = match->angle + turn;
        // Fade rotation out for near-round shapes, whose axis is just noise.
        const double weight = rotationWeight(std::min(a.aniso, match->aniso));
        const double r = turn * weight, c = std::cos(r), si = std::sin(r);
        const double vx = match->point.x - match->centre.x, vy = match->point.y - match->centre.y;
        start = nearestArc(s, u, {a.centre.x + c * vx - si * vy, a.centre.y + si * vx + c * vy});
    } else {
        start = stableStart(s, u, area);
    }
    a.point = pointAt(s, u, start);
    out.push_back(a);
    return start;
}

// Rebuilds every path with `count` evenly spaced points. Closed shapes start at
// the anchor and run with positive area when stable (so no re-anchoring is
// needed afterwards) and keep their repeated closing point.
inline void resampleAll(std::vector<float>& x, std::vector<float>& y, int count, int mode,
                        const std::vector<AnchorState>* hints, std::vector<AnchorState>& anchors) {
    std::vector<float> ox, oy;
    size_t begin = 0;
    for(size_t i = 0; i <= x.size(); ++i) {
        if(i < x.size() && x[i] != -1) continue;
        if(i > begin) {
            Shape s; std::vector<size_t> local; std::vector<double> u; double area;
            if(!ox.empty()) { ox.push_back(-1); oy.push_back(-1); }
            if(!parseShape(x, y, begin, i, s, local, u, area)) {
                ox.insert(ox.end(), x.begin() + begin, x.begin() + i);   // degenerate: pass through
                oy.insert(oy.end(), y.begin() + begin, y.begin() + i);
            } else {
                const bool anchored = s.closed && mode != kPathStart;
                const double start = anchored ? chooseStart(s, u, area, mode, hints, anchors) : 0;
                const double dir = anchored && area < 0 ? -1 : 1;
                const int n = std::max(s.closed ? 3 : 2, count);
                for(int k = 0; k < n; ++k) {
                    double t = s.closed ? start + dir * double(k) / n : double(k) / (n - 1);
                    if(s.closed) t -= std::floor(t);
                    const Vec2 p = pointAt(s, u, t);
                    ox.push_back(float(p.x)); oy.push_back(float(p.y));
                }
                if(s.closed) { ox.push_back(ox[ox.size() - n]); oy.push_back(oy[oy.size() - n]); }
            }
        }
        begin = i + 1;
    }
    x.swap(ox); y.swap(oy);
}

inline void buildShape(const std::vector<float>& x, const std::vector<float>& y,
                       size_t begin, size_t end, size_t shapeIndex, const Controls& c,
                       bool preAnchored, const std::vector<AnchorState>* hints,
                       double windowStart, double windowSpan, Frame& frame) {
    Shape s;
    std::vector<size_t> local;
    std::vector<double> u;
    double area;
    if(!parseShape(x, y, begin, end, s, local, u, area)) return;
    const size_t n = s.pos.size();
    s.area = area;

    // Outward for closed shapes regardless of winding; left of travel when open.
    const double side = s.closed && area > 0 ? -1 : 1;
    s.normal.resize(n);
    for(size_t j = 0; j < n; ++j) {
        const bool hasPrev = s.closed || j > 0, hasNext = s.closed || j + 1 < n;
        const Vec2 p = s.pos[(j + n - 1) % n], q = s.pos[j], r = s.pos[(j + 1) % n];
        const Vec2 in = hasPrev ? unit(q.x - p.x, q.y - p.y) : Vec2{};
        const Vec2 out = hasNext ? unit(r.x - q.x, r.y - q.y) : Vec2{};
        Vec2 t = unit(in.x + out.x, in.y + out.y);
        if(t.x == 0 && t.y == 0) t = hasNext ? out : in;   // reversing cusp
        s.normal[j] = {-t.y * side, t.x * side};
    }

    // Position along the shape used for every control vector.
    std::vector<double> along = u;
    if(s.closed && c.anchor != kPathStart && !preAnchored) {
        const double start = chooseStart(s, u, area, c.anchor, hints, frame.anchors);
        const double dir = area > 0 ? 1 : -1;
        for(size_t j = 0; j < n; ++j) {
            const double t = dir * (u[j] - start);
            along[j] = t - std::floor(t);
        }
    }

    const double offset = samplePerShape(c.waveOffset, shapeIndex, -1e6, 1e6, 0);
    const double repeat = samplePerShape(c.waveRepeat, shapeIndex, 0, 1000, 1);
    std::vector<double> width(n);
    s.target.resize(n); s.stiffness.resize(n); s.damping.resize(n);
    s.speed.resize(n); s.motion.resize(n);
    for(size_t j = 0; j < n; ++j) {
        const double t = along[j];
        double w = t * repeat + offset;
        double wrapped = w - std::floor(w);
        if(!s.closed && wrapped == 0 && w > 0) wrapped = 1;   // open end reads last sample
        // By Length reads only this shape's window of whole cycles, as a loop.
        const bool shared = c.distribute == kByLength;
        const double read = shared ? windowStart + wrapped * windowSpan : wrapped;
        const double wave = sampleAlong(c.wave, read, shared || s.closed, 0, 1, .5);
        s.target[j] = (2 * wave - 1) * sampleAlong(c.amount, t, s.closed, -1, 1, 0);
        width[j] = sampleAlong(c.smooth, t, s.closed, 0, 1, 0);
        s.stiffness[j] = sampleAlong(c.stiffness, t, s.closed, 0, 1000, 0);
        s.damping[j] = sampleAlong(c.damping, t, s.closed, 0, 10, 0);
        s.speed[j] = sampleAlong(c.propagation, t, s.closed, 0, 100, 0);
        s.motion[j] = sampleAlong(c.motion, t, s.closed, 0, 1, 0);
    }
    smoothNormals(s, u, width);

    const size_t index = frame.shapes.size();
    for(size_t i = begin; i < end; ++i) {
        frame.slotShape[i] = index;
        frame.slotNode[i] = local[i - begin];
    }
    frame.shapes.push_back(std::move(s));
}

// Whole cycles per shape in proportion to length; at least one each when
// there are enough. Largest-deficit greedy, so the total is always exact.
inline std::vector<int> shareCycles(const std::vector<double>& length, int total) {
    const size_t count = length.size();
    std::vector<int> share(count, 0);
    double sum = 0;
    for(double l : length) sum += l;
    if(count == 0 || total <= 0 || !(sum > 0)) return share;
    std::vector<double> ideal(count);
    for(size_t i = 0; i < count; ++i) ideal[i] = total * length[i] / sum;
    int left = total;
    if(size_t(total) >= count) { std::fill(share.begin(), share.end(), 1); left -= int(count); }
    for(; left > 0; --left) {
        size_t best = 0;
        for(size_t i = 1; i < count; ++i)
            if(ideal[i] - share[i] > ideal[best] - share[best]) best = i;
        ++share[best];
    }
    return share;
}

// Keeps last frame's shares while they stay close to fair, so a blob that
// slowly grows does not flicker between 8 and 9 cycles at the halfway point:
// a shape changes count only once its fair share is 3/4 of a cycle away.
inline std::vector<int> stickyCycles(const std::vector<double>& length, int total,
                                     const std::vector<int>& previous) {
    const auto fresh = shareCycles(length, total);
    if(previous.size() != fresh.size()) return fresh;
    int previousTotal = 0;
    for(int p : previous) previousTotal += p;
    if(previousTotal != total) return fresh;
    double sum = 0;
    for(double l : length) sum += l;
    for(size_t i = 0; i < fresh.size(); ++i) {
        const double ideal = total * length[i] / sum;
        if(std::abs(ideal - previous[i]) > std::abs(ideal - fresh[i]) + .5) return fresh;
    }
    return previous;
}

// Solves a (possibly cyclic) tridiagonal system in place: a = sub, b = diag, c = super.
// Rows are strictly diagonally dominant here, so no pivoting is needed.
inline void solveTridiagonal(std::vector<double> a, std::vector<double> b, std::vector<double> c,
                             std::vector<double>& d) {
    const size_t n = d.size();
    for(size_t i = 1; i < n; ++i) {
        const double m = a[i] / b[i - 1];
        b[i] -= m * c[i - 1];
        d[i] -= m * d[i - 1];
    }
    d[n - 1] /= b[n - 1];
    for(size_t i = n - 1; i-- > 0;) d[i] = (d[i] - c[i] * d[i + 1]) / b[i];
}

inline void solveCyclic(const std::vector<double>& a, const std::vector<double>& b,
                        const std::vector<double>& c, std::vector<double>& d) {
    // Sherman-Morrison: corners are a[0] (row 0, col n-1) and c[n-1] (row n-1, col 0).
    const size_t n = d.size();
    const double alpha = c[n - 1], beta = a[0], gamma = -b[0];
    std::vector<double> bb = b;
    bb[0] = b[0] - gamma;
    bb[n - 1] = b[n - 1] - alpha * beta / gamma;
    std::vector<double> aa = a, cc = c;
    aa[0] = 0; cc[n - 1] = 0;
    std::vector<double> z(n, 0);
    z[0] = gamma; z[n - 1] = alpha;
    solveTridiagonal(aa, bb, cc, d);
    solveTridiagonal(aa, bb, cc, z);
    const double fact = (d[0] + beta * d[n - 1] / gamma) / (1 + z[0] + beta * z[n - 1] / gamma);
    for(size_t i = 0; i < n; ++i) d[i] -= fact * z[i];
}

} // namespace detail

// hints: last frame's anchors, used by the Tracked anchor mode.
// cycleHints: last frame's cycle shares, used by By Length.
inline Frame prepare(const std::vector<float>& x, const std::vector<float>& y, const Controls& c,
                     const std::vector<AnchorState>* hints = nullptr,
                     const std::vector<CycleState>* cycleHints = nullptr) {
    Frame frame;
    if(x.size() != y.size()) return frame;
    for(size_t i = 0; i < x.size(); ++i)
        if(!std::isfinite(x[i]) || !std::isfinite(y[i]) || ((x[i] == -1) != (y[i] == -1))) return frame;
    frame.valid = true;
    frame.x = x; frame.y = y;
    if(c.resample > 0)
        detail::resampleAll(frame.x, frame.y, std::min(c.resample, 1 << 16), c.anchor, hints, frame.anchors);
    const auto& px = frame.x;
    const auto& py = frame.y;
    frame.slotShape.assign(px.size(), Frame::npos);
    frame.slotNode.assign(px.size(), Frame::npos);
    std::vector<std::pair<size_t, size_t>> paths;
    for(size_t i = 0, begin = 0; i <= px.size(); ++i) {
        if(i == px.size() || px[i] == -1) {
            if(i > begin) paths.push_back({begin, i});
            begin = i + 1;
        }
    }
    // By Length: measure every path, then give each a window of whole cycles.
    std::vector<double> windowStart(paths.size(), 0), windowSpan(paths.size(), 1);
    if(c.distribute == kByLength) {
        const int total = std::max(1, std::min(c.waveCycles, 1 << 16));
        std::vector<size_t> measured;
        std::vector<double> length;
        std::vector<CycleState> states;
        for(size_t p = 0; p < paths.size(); ++p) {
            Shape s; std::vector<size_t> local; std::vector<double> u; double area;
            if(!detail::parseShape(px, py, paths[p].first, paths[p].second, s, local, u, area)) continue;
            const AnchorState pose = detail::measurePose(s, area);
            measured.push_back(p);
            length.push_back(s.perimeter);
            states.push_back({pose.centre, pose.radius, 0});
        }
        // Match last frame's shapes by position, one to one, so shares follow
        // the blobs even if the input lists them in another order.
        std::vector<int> previous;
        if(cycleHints && cycleHints->size() == states.size()) {
            std::vector<bool> used(cycleHints->size(), false);
            for(const auto& st : states) {
                size_t best = Frame::npos; double bestDistance = 0;
                for(size_t h = 0; h < cycleHints->size(); ++h) {
                    const auto& hint = (*cycleHints)[h];
                    const double d = std::hypot(hint.centre.x - st.centre.x, hint.centre.y - st.centre.y);
                    if(!used[h] && d < std::max(st.radius, hint.radius) && (best == Frame::npos || d < bestDistance)) {
                        best = h; bestDistance = d;
                    }
                }
                if(best == Frame::npos) { previous.clear(); break; }
                used[best] = true;
                previous.push_back((*cycleHints)[best].cycles);
            }
        }
        const auto share = detail::stickyCycles(length, total, previous);
        int done = 0;
        for(size_t k = 0; k < measured.size(); ++k) {
            windowStart[measured[k]] = double(done) / total;
            windowSpan[measured[k]] = double(share[k]) / total;
            done += share[k];
            states[k].cycles = share[k];
        }
        frame.cycles = states;
    }
    for(size_t p = 0; p < paths.size(); ++p)
        detail::buildShape(px, py, paths[p].first, paths[p].second, p, c, c.resample > 0, hints,
                           windowStart[p], windowSpan[p], frame);
    return frame;
}

struct Result { std::vector<float> x, y; };

inline Result compose(const Frame& f, const std::vector<std::vector<double>>& offsets) {
    if(!f.valid) return {};
    Result r{f.x, f.y};
    for(size_t i = 0; i < r.x.size(); ++i) {
        const size_t s = f.slotShape[i];
        if(s == Frame::npos) continue;
        const size_t j = f.slotNode[i];
        const Vec2 n = f.shapes[s].normal[j];
        r.x[i] = float(r.x[i] + n.x * offsets[s][j]);
        r.y[i] = float(r.y[i] + n.y * offsets[s][j]);
    }
    return r;
}

// Stateless displacement straight to the wave target.
inline Result direct(const std::vector<float>& x, const std::vector<float>& y, const Controls& c) {
    const Frame f = prepare(x, y, c);
    std::vector<std::vector<double>> offsets;
    for(const auto& s : f.shapes) offsets.push_back(s.target);
    return compose(f, offsets);
}

// Membrane model per node j (unit mass, normalized arc-length u):
//   o'' = -k (o - target) - d o' + c^2 d2(o - target)/du2 + eta d2(o')/du2 + F
//   d = 2 * damping * sqrt(k + 1),  eta = 2 * max(damping, 0.05) * c / kappa
// eta damps ripples travelling along the outline in proportion to their
// fineness (wavenumber): Damping applies as-is to ripples of 4 cycles per
// perimeter and more strongly to finer ones, which would otherwise ring
// faster than the frame rate can show. Its small floor keeps outline noise
// from feeding never-ending sub-frame shimmer when Damping is 0.
// The deviation from the wave is limited to half the body's size, so soft or
// plastic settings with strong Motion Influence cannot fly apart.
// Tension acts on the deviation from the wave, so the outline rests exactly
// on the wave and only disturbances travel along it. F is the inertial force
// of the body's own acceleration (Motion Influence), see bodyForces().
// Integrated with Newmark average acceleration (trapezoidal): unconditionally
// stable, no artificial amplitude loss, so Damping 0 rings indefinitely.
class Simulation {
public:
    void reset() {
        offset.clear(); velocity.clear(); signature.clear(); anchors.clear(); bodies.clear(); cycles.clear();
    }

    // By Length: last frame's cycle share per shape.
    const std::vector<CycleState>& cycleShares() const { return cycles; }

    Result update(const std::vector<float>& x, const std::vector<float>& y, const Controls& c,
                  bool spring, double seconds) {
        const Frame f = prepare(x, y, c, &anchors, &cycles);
        if(!f.valid) { reset(); return {}; }
        anchors = f.anchors;
        cycles = f.cycles;
        std::vector<size_t> sig = f.slotNode;
        for(const auto& s : f.shapes) sig.push_back(s.closed ? 1 : 0);
        sig.insert(sig.end(), f.slotShape.begin(), f.slotShape.end());
        const bool topologyChanged = sig != signature;
        if(topologyChanged) {
            signature = sig;
            offset.assign(f.shapes.size(), {});
            velocity.assign(f.shapes.size(), {});
            bodies.assign(f.shapes.size(), {});
            // Start on the wave: a membrane starting flat would be hit by the
            // whole wave at once and ring for no visible reason.
            for(size_t s = 0; s < f.shapes.size(); ++s) {
                offset[s] = f.shapes[s].target;
                velocity[s].assign(f.shapes[s].pos.size(), 0);
            }
        }
        const bool timed = std::isfinite(seconds) && seconds > 1e-6 && seconds <= .1;
        const double dt = std::isfinite(seconds) ? std::min(.1, seconds) : 0;
        for(size_t s = 0; s < f.shapes.size(); ++s) {
            std::vector<double> force(f.shapes[s].pos.size(), 0);
            const double limit = .5 * detail::measurePose(f.shapes[s], f.shapes[s].area).radius;
            // Body motion is tracked in every mode so enabling Spring or
            // Motion Influence never replays stale movement.
            bodyForces(f.shapes[s], bodies[s], spring && timed ? dt : 0, force);
            if(!spring) {
                offset[s] = f.shapes[s].target;
                std::fill(velocity[s].begin(), velocity[s].end(), 0);
            } else if(dt > 0) {
                const int substeps = substepsFor(f.shapes[s], dt);
                for(int i = 0; i < substeps; ++i)
                    step(f.shapes[s], force, limit, offset[s], velocity[s], dt / substeps);
            }
        }
        return compose(f, offset);
    }

private:
    // Smoothed rigid motion of one shape: centre velocity and spin.
    struct Body {
        bool known = false, moving = false;
        Vec2 centre, velocity;
        double angle = 0, spin = 0;
    };
    std::vector<std::vector<double>> offset, velocity;
    std::vector<size_t> signature;
    std::vector<AnchorState> anchors;
    std::vector<CycleState> cycles;
    std::vector<Body> bodies;

    // Inertial force along each normal from the shape's rigid-body
    // acceleration (centre acceleration, angular acceleration and the
    // centrifugal term of its spin). Measured on the whole shape, so outline
    // noise is not injected per point, and constant-speed travel causes no
    // deformation: only starting, stopping, turning and spinning do.
    static void bodyForces(const Shape& s, Body& b, double dt, std::vector<double>& force) {
        const AnchorState pose = detail::measurePose(s, s.area);
        if(!b.known || dt <= 0) {
            b = {}; b.known = true; b.centre = pose.centre; b.angle = pose.angle;
            return;
        }
        double turn = pose.angle - b.angle;
        turn -= kPi * std::round(turn / kPi);                 // the axis has no direction
        const double weight = detail::rotationWeight(pose.aniso);
        // Low-pass the velocities (about 60 ms) so outline noise, which the
        // acceleration would amplify by 1/dt^2, stays below the real motion.
        const double blend = 1 - std::exp(-dt / .06);
        const Vec2 rawVelocity{(pose.centre.x - b.centre.x) / dt, (pose.centre.y - b.centre.y) / dt};
        const Vec2 velocity{b.velocity.x + (rawVelocity.x - b.velocity.x) * blend,
                            b.velocity.y + (rawVelocity.y - b.velocity.y) * blend};
        const double spin = b.spin + (weight * turn / dt - b.spin) * blend;
        const Vec2 accel{(velocity.x - b.velocity.x) / dt, (velocity.y - b.velocity.y) / dt};
        const double spinAccel = (spin - b.spin) / dt;
        const bool ready = b.moving;   // needs one velocity sample before accelerating
        b.centre = pose.centre; b.angle += turn; b.velocity = velocity; b.spin = spin; b.moving = true;
        if(!ready) return;
        for(size_t j = 0; j < s.pos.size(); ++j) {
            if(s.motion[j] <= 0) continue;
            const double rx = s.pos[j].x - pose.centre.x, ry = s.pos[j].y - pose.centre.y;
            const double ax = accel.x - spinAccel * ry - spin * spin * rx;
            const double ay = accel.y + spinAccel * rx - spin * spin * ry;
            force[j] = -s.motion[j] * (ax * s.normal[j].x + ay * s.normal[j].y);
        }
    }

    // The solver is stable at any step; substeps only keep fast springs and
    // fast ripples accurate (less phase lag) at low frame rates.
    static int substepsFor(const Shape& s, double dt) {
        double spring = 0, speed = 0, minGap = 1;
        for(size_t j = 0; j < s.pos.size(); ++j) {
            spring = std::max(spring, std::sqrt(s.stiffness[j]));
            speed = std::max(speed, s.speed[j]);
        }
        for(double g : s.gap) if(g > 0) minGap = std::min(minGap, g);
        // Aim for <= 0.5 rad of spring phase and a Courant number <= 2 per substep.
        const double needed = std::max(spring * dt / .5, speed * dt / std::max(minGap, 1e-3) / 2);
        return int(std::min(16.0, std::max(1.0, std::ceil(needed))));
    }

    static void step(const Shape& s, const std::vector<double>& force, double limit,
                     std::vector<double>& o, std::vector<double>& v, double dt) {
        const size_t n = s.pos.size();
        constexpr double kappa = 2 * kPi * 4;   // reference ripple: 4 cycles per perimeter
        std::vector<double> lower(n, 0), upper(n, 0), d(n), k(n), c2(n), eta(n);
        for(size_t j = 0; j < n; ++j) {
            k[j] = s.stiffness[j];
            d[j] = 2 * s.damping[j] * std::sqrt(k[j] + 1);
            c2[j] = s.speed[j] * s.speed[j];
            eta[j] = 2 * std::max(.05, s.damping[j]) * s.speed[j] / kappa;
            // Non-uniform second difference; open ends are free (zero slope).
            const bool hasPrev = s.closed || j > 0, hasNext = s.closed || j + 1 < n;
            const double hl = hasPrev ? s.gap[(j + n - 1) % n] : 0;
            const double hr = hasNext ? s.gap[j] : 0;
            if(hasPrev && hasNext) {
                lower[j] = 2 / ((hl + hr) * hl);
                upper[j] = 2 / ((hl + hr) * hr);
            } else if(hasNext) {
                upper[j] = 2 / (hr * hr);
            } else if(hasPrev) {
                lower[j] = 2 / (hl * hl);
            }
        }
        auto laplacian = [&](const std::vector<double>& q, size_t j) {
            const double l = lower[j] ? q[(j + n - 1) % n] - q[j] : 0;
            const double r = upper[j] ? q[(j + 1) % n] - q[j] : 0;
            return lower[j] * l + upper[j] * r;
        };
        std::vector<double> acc(n), predicted(n), w(n), sub(n), diag(n), sup(n), rhs(n);
        const double h = 4 / (dt * dt);
        for(size_t j = 0; j < n; ++j) {
            acc[j] = -k[j] * (o[j] - s.target[j]) - d[j] * v[j] + force[j] +
                     c2[j] * (laplacian(o, j) - laplacian(s.target, j)) + eta[j] * laplacian(v, j);
            predicted[j] = o[j] + dt * v[j] + dt * dt / 4 * acc[j];
        }
        // Newmark gives v_next = w + (2/dt) o_next, with w known now.
        for(size_t j = 0; j < n; ++j) w[j] = v[j] + dt / 2 * acc[j] - 2 / dt * predicted[j];
        for(size_t j = 0; j < n; ++j) {
            const double alpha = (1 + d[j] * dt / 2) * h;
            const double coupling = c2[j] + 2 * eta[j] / dt;
            sub[j] = -coupling * lower[j];
            sup[j] = -coupling * upper[j];
            diag[j] = alpha + k[j] + coupling * (lower[j] + upper[j]);
            rhs[j] = alpha * predicted[j] + k[j] * s.target[j] - d[j] * v[j] - d[j] * dt / 2 * acc[j]
                     - c2[j] * laplacian(s.target, j) + eta[j] * laplacian(w, j) + force[j];
        }
        std::vector<double> next = rhs;
        if(s.closed && n >= 3) detail::solveCyclic(sub, diag, sup, next);
        else detail::solveTridiagonal(sub, diag, sup, next);
        for(size_t j = 0; j < n; ++j) {
            if(!std::isfinite(next[j])) { o.assign(n, 0); v.assign(n, 0); return; }
            const double accNext = h * (next[j] - predicted[j]);
            v[j] += dt / 2 * (acc[j] + accNext);
            o[j] = next[j];
            const double deviation = o[j] - s.target[j];
            if(std::abs(deviation) > limit) {   // hit the stop: no further outward motion
                o[j] = s.target[j] + std::copysign(limit, deviation);
                if(v[j] * deviation > 0) v[j] = 0;
            }
        }
    }
};

} // namespace waveProCore
