#pragma once

// 1D Contour core: unfolds a closed body outline along its spine into a 1D
// profile (distance from the spine to the outline, head to tail). The result
// depends only on the body's shape, not on its position, rotation, bending,
// or on where the input path starts. No openFrameworks dependency.
//
// Per shape:
//   1. The outline is resampled with evenly spaced points.
//   2. Its two tips are the farthest pair of outline points (the body's
//      diameter). From frame to frame they are carried with the body
//      (translation, plus rotation of its long axis when elongated) and only
//      re-picked when a clearly longer diameter appears, so they never hop.
//   3. The outline is split at the tips into two sides; the spine is the
//      midline between them (sides paired by arc fraction), smoothed, then
//      measured by its own arc length. That length is the unfolded x axis.
//   4. Every outline point is projected onto the spine: position along it and
//      distance from it. Each side is rasterised into Size bins (farthest
//      point per bin), giving a Left and a Right profile; Width is their sum.
//   5. Head/tail: the wider end, the narrower end, the tip nearest a given
//      point, or the tip leading the body's motion, always with hysteresis
//      against last frame's choice so a near-symmetric body never swaps ends.

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace contour1DCore {

struct Vec2 { double x = 0, y = 0; };
constexpr double kPi = 3.14159265358979323846;

enum HeadMode { kWideEnd = 0, kNarrowEnd = 1, kPoint = 2, kMotion = 3 };

struct Controls {
    int resample = 512;              // outline points after even resampling
    int size = 256;                  // profile samples, head to tail
    int head = kWideEnd;             // HeadMode
    bool flip = false;               // swap head and tail after the rule
    std::vector<float> headX, headY; // Point mode: one point per shape (cycles)
    double spineSmooth = .1;         // box window on the spine, fraction of its length
    double smooth = 0;               // box window on the profiles, fraction of the length
    double hysteresis = .1;          // margin before tips are re-picked or ends swap
};

// What one shape carries to the next frame.
struct Track {
    Vec2 centre, head, tail, velocity;   // velocity: smoothed centre motion per update
    double angle = 0, aniso = 0, radius = 0;
};

struct Shape {
    Vec2 centre, head, tail;
    double area = 0, perimeter = 0, length = 0;   // length: of the spine, in input units
    std::vector<Vec2> spine;                      // head to tail
    std::vector<double> left, right;              // Size samples, head to tail
};

struct Result {
    bool valid = false;
    std::vector<Shape> shapes;    // in input order, degenerate paths skipped
    std::vector<Track> tracks;    // one per shape, pass back as hints next frame
};

namespace detail {

inline double dist(Vec2 a, Vec2 b) { return std::hypot(a.x - b.x, a.y - b.y); }

// Unique points of one path as a closed outline; a repeated closing point is dropped.
inline std::vector<Vec2> outline(const std::vector<float>& x, const std::vector<float>& y,
                                 size_t begin, size_t end) {
    std::vector<Vec2> p;
    for(size_t i = begin; i < end; ++i)
        if(p.empty() || p.back().x != x[i] || p.back().y != y[i]) p.push_back({x[i], y[i]});
    while(p.size() > 1 && p.front().x == p.back().x && p.front().y == p.back().y) p.pop_back();
    return p;
}

// Cumulative length of a polyline (closed: includes the segment back to the start).
inline std::vector<double> cumulative(const std::vector<Vec2>& p, bool closed) {
    const size_t n = p.size(), segments = closed ? n : n - 1;
    std::vector<double> cum(segments + 1, 0);
    for(size_t j = 0; j < segments; ++j) cum[j + 1] = cum[j] + dist(p[j], p[(j + 1) % n]);
    return cum;
}

// Point at distance t along a polyline with cumulative lengths cum.
inline Vec2 at(const std::vector<Vec2>& p, const std::vector<double>& cum, double t, size_t& j) {
    const size_t n = p.size(), segments = cum.size() - 1;
    while(j + 1 < segments && cum[j + 1] <= t) ++j;
    const double seg = cum[j + 1] - cum[j];
    const double f = seg > 0 ? std::min(1.0, std::max(0.0, (t - cum[j]) / seg)) : 0;
    const Vec2 a = p[j], b = p[(j + 1) % n];
    return {a.x + (b.x - a.x) * f, a.y + (b.y - a.y) * f};
}

// `count` evenly spaced points around a closed outline, starting at its first point.
inline std::vector<Vec2> resampleClosed(const std::vector<Vec2>& p, int count, double& perimeter) {
    const auto cum = cumulative(p, true);
    perimeter = cum.back();
    std::vector<Vec2> out(count);
    size_t j = 0;
    for(int k = 0; k < count; ++k) out[k] = at(p, cum, perimeter * k / count, j);
    return out;
}

// `count` evenly spaced points along an open polyline, both ends kept.
inline std::vector<Vec2> resampleOpen(const std::vector<Vec2>& p, int count, double& length) {
    if(p.size() < 2) { length = 0; return std::vector<Vec2>(count, p.empty() ? Vec2{} : p[0]); }
    const auto cum = cumulative(p, false);
    length = cum.back();
    std::vector<Vec2> out(count);
    size_t j = 0;
    for(int k = 0; k < count; ++k) out[k] = at(p, cum, length * k / (count - 1), j);
    out.front() = p.front(); out.back() = p.back();
    return out;
}

struct Pose { Vec2 centre; double angle = 0, aniso = 0, radius = 0, area = 0; };

// Area centroid, long-axis angle (mod pi), elongation (0 round - 1 line) and
// size of an evenly resampled outline.
inline Pose measure(const std::vector<Vec2>& p) {
    const size_t n = p.size();
    const Vec2 o = p[0];
    Pose s;
    double a2 = 0, cx = 0, cy = 0;
    for(size_t j = 0; j < n; ++j) {
        const Vec2 a{p[j].x - o.x, p[j].y - o.y}, b{p[(j + 1) % n].x - o.x, p[(j + 1) % n].y - o.y};
        const double cross = a.x * b.y - b.x * a.y;
        a2 += cross; cx += (a.x + b.x) * cross; cy += (a.y + b.y) * cross;
    }
    s.area = .5 * std::abs(a2);
    if(std::abs(a2) > 1e-12) {
        s.centre = {o.x + cx / (3 * a2), o.y + cy / (3 * a2)};
    } else {
        for(const auto& q : p) { s.centre.x += q.x / n; s.centre.y += q.y / n; }
    }
    double cxx = 0, cyy = 0, cxy = 0;
    for(const auto& q : p) {
        const double dx = q.x - s.centre.x, dy = q.y - s.centre.y;
        cxx += dx * dx / n; cyy += dy * dy / n; cxy += dx * dy / n;
    }
    const double spread = std::hypot(.5 * (cxx - cyy), cxy), trace = cxx + cyy;
    s.angle = .5 * std::atan2(2 * cxy, cxx - cyy);
    s.aniso = trace > 0 ? 2 * spread / trace : 0;
    s.radius = std::sqrt(std::max(0.0, trace));
    return s;
}

// Rotation can only be measured on elongated shapes: fade it out when round.
inline double rotationWeight(double aniso) {
    return std::min(1.0, std::max(0.0, (aniso - .05) / .1));
}

// Indices of the convex hull (Andrew's monotone chain).
inline std::vector<size_t> convexHull(const std::vector<Vec2>& p) {
    std::vector<size_t> order(p.size());
    for(size_t i = 0; i < p.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        return p[a].x < p[b].x || (p[a].x == p[b].x && p[a].y < p[b].y);
    });
    auto cross = [&](size_t o, size_t a, size_t b) {
        return (p[a].x - p[o].x) * (p[b].y - p[o].y) - (p[a].y - p[o].y) * (p[b].x - p[o].x);
    };
    std::vector<size_t> hull(2 * p.size());
    size_t k = 0;
    for(size_t i = 0; i < order.size(); ++i) {
        while(k >= 2 && cross(hull[k - 2], hull[k - 1], order[i]) <= 0) --k;
        hull[k++] = order[i];
    }
    for(size_t i = order.size() - 1, t = k + 1; i-- > 0;) {
        while(k >= t && cross(hull[k - 2], hull[k - 1], order[i]) <= 0) --k;
        hull[k++] = order[i];
    }
    hull.resize(k > 1 ? k - 1 : k);
    return hull;
}

// The farthest pair of outline points always lies on the convex hull.
inline std::pair<size_t, size_t> farthestPair(const std::vector<Vec2>& p) {
    const auto hull = convexHull(p);
    std::pair<size_t, size_t> best{0, 0};
    double far = -1;
    for(size_t a = 0; a < hull.size(); ++a)
        for(size_t b = a + 1; b < hull.size(); ++b) {
            const size_t i = hull[a], j = hull[b];
            const double dx = p[i].x - p[j].x, dy = p[i].y - p[j].y, d = dx * dx + dy * dy;
            if(d > far) { far = d; best = {std::min(i, j), std::max(i, j)}; }
        }
    return best;
}

// Moves index a along the outline to the local maximum of its distance from
// p[b], so a carried tip re-centres on the true tip of its lobe every frame
// without ever jumping to another lobe.
inline void climb(const std::vector<Vec2>& p, size_t& a, size_t b) {
    const size_t n = p.size();
    for(size_t it = 0; it < n; ++it) {
        const size_t l = (a + n - 1) % n, r = (a + 1) % n;
        const double da = dist(p[a], p[b]), dl = dist(p[l], p[b]), dr = dist(p[r], p[b]);
        if(dl > da && dl >= dr) a = l;
        else if(dr > da) a = r;
        else break;
    }
}

inline size_t nearestIndex(const std::vector<Vec2>& p, Vec2 q) {
    size_t best = 0;
    double near = std::numeric_limits<double>::infinity();
    for(size_t i = 0; i < p.size(); ++i) {
        const double d = dist(p[i], q);
        if(d < near) { near = d; best = i; }
    }
    return best;
}

// Outline points from index `from` to index `to`, walking forward or backward.
inline std::vector<Vec2> side(const std::vector<Vec2>& p, size_t from, size_t to, bool forward) {
    const size_t n = p.size();
    std::vector<Vec2> s;
    for(size_t i = from;; i = forward ? (i + 1) % n : (i + n - 1) % n) {
        s.push_back(p[i]);
        if(i == to) break;
    }
    return s;
}

// Box filter over a window of `fraction` of the polyline, ends pinned.
inline void smoothOpen(std::vector<Vec2>& p, double fraction) {
    const int n = int(p.size());
    const int r = int(std::round(std::min(1.0, std::max(0.0, fraction)) * n * .5));
    if(r < 1 || n < 3) return;
    std::vector<double> px(n + 1, 0), py(n + 1, 0);
    for(int i = 0; i < n; ++i) { px[i + 1] = px[i] + p[i].x; py[i + 1] = py[i] + p[i].y; }
    for(int i = 1; i + 1 < n; ++i) {
        const int w = std::min(r, std::min(i, n - 1 - i));
        const double count = 2 * w + 1;
        p[i] = {(px[i + w + 1] - px[i - w]) / count, (py[i + w + 1] - py[i - w]) / count};
    }
}

inline void smoothProfile(std::vector<double>& v, double fraction) {
    const int n = int(v.size());
    const int r = int(std::round(std::min(1.0, std::max(0.0, fraction)) * n * .5));
    if(r < 1 || n < 2) return;
    std::vector<double> prefix(n + 1, 0);
    for(int i = 0; i < n; ++i) prefix[i + 1] = prefix[i] + v[i];
    for(int i = 0; i < n; ++i) {
        const int lo = std::max(0, i - r), hi = std::min(n - 1, i + r);
        v[i] = (prefix[hi + 1] - prefix[lo]) / (hi - lo + 1);
    }
}

// Projection of q onto the spine: arc fraction s (0 head - 1 tail), distance
// and the signed side (positive = left of travel on screen, y down).
struct Projection { double s = 0, distance = 0, signedSide = 0; };

inline Projection project(const std::vector<Vec2>& spine, const std::vector<double>& cum, Vec2 q) {
    Projection best;
    double near = std::numeric_limits<double>::infinity();
    const double length = cum.back();
    for(size_t j = 0; j + 1 < spine.size(); ++j) {
        const Vec2 a = spine[j], b = spine[j + 1];
        const double ex = b.x - a.x, ey = b.y - a.y, l2 = ex * ex + ey * ey;
        const double t = l2 > 0 ? std::min(1.0, std::max(0.0, ((q.x - a.x) * ex + (q.y - a.y) * ey) / l2)) : 0;
        const Vec2 foot{a.x + ex * t, a.y + ey * t};
        const double d = dist(q, foot);
        if(d < near) {
            near = d;
            best.distance = d;
            best.s = length > 0 ? (cum[j] + (cum[j + 1] - cum[j]) * t) / length : 0;
            const double l = std::sqrt(l2);
            best.signedSide = l > 0 ? ((q.x - foot.x) * ey - (q.y - foot.y) * ex) / l : 0;
        }
    }
    return best;
}

// Draws a side's (s, distance) polyline into `bins` keeping the farthest value
// per bin, then fills bins no segment touched from their neighbours.
inline void rasterise(const std::vector<Projection>& side, std::vector<double>& bins) {
    const int n = int(bins.size());
    std::fill(bins.begin(), bins.end(), -1.0);
    for(size_t i = 0; i + 1 < side.size(); ++i) {
        const double s0 = side[i].s, s1 = side[i + 1].s, d0 = side[i].distance, d1 = side[i + 1].distance;
        const double lo = std::min(s0, s1), hi = std::max(s0, s1);
        const int k0 = std::max(0, int(std::ceil(lo * (n - 1) - 1e-9)));
        const int k1 = std::min(n - 1, int(std::floor(hi * (n - 1) + 1e-9)));
        for(int k = k0; k <= k1; ++k) {
            const double pos = double(k) / (n - 1);
            const double f = s1 != s0 ? std::min(1.0, std::max(0.0, (pos - s0) / (s1 - s0))) : 0;
            bins[k] = std::max(bins[k], d0 + (d1 - d0) * f);
        }
    }
    int last = -1;
    for(int k = 0; k < n; ++k) {
        if(bins[k] >= 0) {
            for(int g = last + 1; g < k; ++g)   // interpolate the gap
                bins[g] = last < 0 ? bins[k] : bins[last] + (bins[k] - bins[last]) * (g - last) / double(k - last);
            last = k;
        }
    }
    if(last < 0) std::fill(bins.begin(), bins.end(), 0.0);
    else for(int g = last + 1; g < n; ++g) bins[g] = bins[last];
}

inline void flip(Shape& s) {
    std::swap(s.head, s.tail);
    std::reverse(s.spine.begin(), s.spine.end());
    std::reverse(s.left.begin(), s.left.end());
    std::reverse(s.right.begin(), s.right.end());
    std::swap(s.left, s.right);   // walking the other way, left becomes right
}

// Builds spine and profiles for an outline whose tips are at indices h (head)
// and t (tail).
inline Shape build(const std::vector<Vec2>& p, size_t h, size_t t, const Controls& c, int size) {
    Shape s;
    s.head = p[h]; s.tail = p[t];
    const int spinePoints = std::max(16, std::min(128, int(p.size() / 2)));
    double la = 0, lb = 0;
    const auto a = resampleOpen(side(p, h, t, true), spinePoints, la);
    const auto b = resampleOpen(side(p, h, t, false), spinePoints, lb);
    std::vector<Vec2> mid(spinePoints);
    for(int k = 0; k < spinePoints; ++k) mid[k] = {.5 * (a[k].x + b[k].x), .5 * (a[k].y + b[k].y)};
    smoothOpen(mid, c.spineSmooth);
    s.spine = resampleOpen(mid, spinePoints, s.length);   // the x axis is spine length
    const auto cum = cumulative(s.spine, false);

    // Project both sides (each includes both tips) and rasterise them.
    const auto forward = side(p, h, t, true), backward = side(p, h, t, false);
    std::vector<Projection> pa, pb;
    double signA = 0, signB = 0;
    for(const auto& q : forward) { pa.push_back(project(s.spine, cum, q)); signA += pa.back().signedSide; }
    for(const auto& q : backward) { pb.push_back(project(s.spine, cum, q)); signB += pb.back().signedSide; }
    s.left.assign(size, 0); s.right.assign(size, 0);
    const bool forwardIsLeft = signA >= signB;
    rasterise(forwardIsLeft ? pa : pb, s.left);
    rasterise(forwardIsLeft ? pb : pa, s.right);
    smoothProfile(s.left, c.smooth);
    smoothProfile(s.right, c.smooth);
    return s;
}

// Width integrated over the first and the last 30% of the spine.
inline void endMasses(const Shape& s, double& head, double& tail) {
    const size_t n = s.left.size();
    head = tail = 0;
    for(size_t k = 0; k < n; ++k) {
        const double w = s.left[k] + s.right[k];
        if(k * 10 < n * 3) head += w;
        if(k * 10 > n * 7) tail += w;
    }
}

inline Vec2 carry(const Track& h, const Pose& pose, double turn) {
    const double c = std::cos(turn), si = std::sin(turn);
    const double vx = h.head.x - h.centre.x, vy = h.head.y - h.centre.y;
    return {pose.centre.x + c * vx - si * vy, pose.centre.y + si * vx + c * vy};
}

} // namespace detail

// hints: last frame's tracks (any order; shapes are matched by position).
inline Result analyse(const std::vector<float>& x, const std::vector<float>& y, const Controls& c,
                      const std::vector<Track>* hints = nullptr) {
    Result result;
    if(x.size() != y.size()) return result;
    for(size_t i = 0; i < x.size(); ++i)
        if(!std::isfinite(x[i]) || !std::isfinite(y[i]) || ((x[i] == -1) != (y[i] == -1))) return result;
    result.valid = true;
    const int resample = std::max(16, std::min(c.resample, 1 << 16));
    const int size = std::max(2, std::min(c.size, 1 << 16));
    const double hyst = std::min(1.0, std::max(0.0, c.hysteresis));
    std::vector<bool> used(hints ? hints->size() : 0, false);

    std::vector<std::pair<size_t, size_t>> paths;
    for(size_t i = 0, begin = 0; i <= x.size(); ++i) {
        if(i == x.size() || x[i] == -1) {
            if(i > begin) paths.push_back({begin, i});
            begin = i + 1;
        }
    }
    for(const auto& path : paths) {
        const auto raw = detail::outline(x, y, path.first, path.second);
        if(raw.size() < 3) continue;
        double perimeter = 0;
        const auto p = detail::resampleClosed(raw, resample, perimeter);
        if(!(perimeter > 0)) continue;
        const detail::Pose pose = detail::measure(p);

        // Last frame's track of this body, if any: nearest centre within its size.
        const Track* hint = nullptr;
        size_t hintIndex = 0;
        if(hints) {
            double best = std::numeric_limits<double>::infinity();
            for(size_t h = 0; h < hints->size(); ++h) {
                const auto& t = (*hints)[h];
                const double d = detail::dist(t.centre, pose.centre);
                if(!used[h] && d < best && d < std::max(pose.radius, t.radius)) { best = d; hint = &t; hintIndex = h; }
            }
        }
        Track track;
        track.centre = pose.centre; track.angle = pose.angle; track.aniso = pose.aniso; track.radius = pose.radius;
        double turn = 0;
        if(hint) {
            used[hintIndex] = true;
            turn = pose.angle - hint->angle;
            turn -= kPi * std::round(turn / kPi);   // the axis has no direction
            track.angle = hint->angle + turn;
            const Vec2 step{pose.centre.x - hint->centre.x, pose.centre.y - hint->centre.y};
            track.velocity = {hint->velocity.x + (step.x - hint->velocity.x) * .3,
                              hint->velocity.y + (step.y - hint->velocity.y) * .3};
        }

        // Tips: carried from last frame while their chord stays close to the
        // true diameter, otherwise the farthest pair (oriented like last frame).
        const auto fresh = detail::farthestPair(p);
        const double diameter = detail::dist(p[fresh.first], p[fresh.second]);
        if(!(diameter > 0)) continue;
        size_t h = fresh.first, t = fresh.second;
        bool oriented = false;
        if(hint) {
            const double rotation = turn * detail::rotationWeight(std::min(pose.aniso, hint->aniso));
            Track swapped = *hint; std::swap(swapped.head, swapped.tail);
            const Vec2 headGuess = detail::carry(*hint, pose, rotation);
            const Vec2 tailGuess = detail::carry(swapped, pose, rotation);
            size_t th = detail::nearestIndex(p, headGuess), tt = detail::nearestIndex(p, tailGuess);
            for(int round = 0; round < 2 && th != tt; ++round) { detail::climb(p, th, tt); detail::climb(p, tt, th); }
            if(th != tt && detail::dist(p[th], p[tt]) >= (1 - hyst) * diameter) {
                h = th; t = tt;
            } else if(detail::dist(p[fresh.second], headGuess) < detail::dist(p[fresh.first], headGuess)) {
                std::swap(h, t);
            }
            oriented = true;
        }

        Shape shape = detail::build(p, h, t, c, size);
        shape.centre = pose.centre; shape.area = pose.area; shape.perimeter = perimeter;

        // Head rule, with hysteresis against the carried orientation.
        const double margin = oriented ? hyst : 0;
        bool swap = false;
        switch(c.head) {
            case kWideEnd: case kNarrowEnd: {
                double headMass, tailMass;
                detail::endMasses(shape, headMass, tailMass);
                const double total = headMass + tailMass;
                if(total > 0) {
                    const double share = headMass / total;   // .5 = symmetric
                    swap = c.head == kWideEnd ? share < .5 - .5 * margin : share > .5 + .5 * margin;
                }
                break;
            }
            case kPoint: {
                const size_t index = result.shapes.size();
                const Vec2 target{c.headX.empty() ? .5 : c.headX[index % c.headX.size()],
                                  c.headY.empty() ? .5 : c.headY[index % c.headY.size()]};
                if(std::isfinite(target.x) && std::isfinite(target.y))
                    swap = detail::dist(shape.tail, target) + margin * pose.radius < detail::dist(shape.head, target);
                break;
            }
            case kMotion: {
                const Vec2 v = track.velocity;
                const double speed = std::hypot(v.x, v.y);
                if(speed > 1e-9) {
                    const double headLead = ((shape.head.x - pose.centre.x) * v.x + (shape.head.y - pose.centre.y) * v.y) / speed;
                    const double tailLead = ((shape.tail.x - pose.centre.x) * v.x + (shape.tail.y - pose.centre.y) * v.y) / speed;
                    swap = tailLead > headLead + margin * pose.radius;
                }
                break;
            }
            default: break;
        }
        if(swap) detail::flip(shape);
        track.head = shape.head; track.tail = shape.tail;   // the rule's orientation, before Flip
        if(c.flip) detail::flip(shape);
        result.shapes.push_back(std::move(shape));
        result.tracks.push_back(track);
    }
    return result;
}

} // namespace contour1DCore
