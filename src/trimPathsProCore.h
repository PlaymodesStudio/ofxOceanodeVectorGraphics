#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

// Independent of openFrameworks so the geometry and vector contract can be tested.
//
// Built for a node that runs every frame on thousands of edges: parse() and
// trim() can write into caller-owned buffers, so a steady-state update makes
// no heap allocation. The value-returning overloads remain for tests.
namespace trimPathsProCore {
enum class Scope { Edges, Paths, AllPaths, Groups };
enum class Mapping { Auto, Scalar, Path, Edge, Group };
enum class Metric { EdgeCount, Distance };

struct Point { double x = 0, y = 0; };
struct Edge { Point a, b; int path = 0, local = 0; double length = 0; };
struct Geometry {
    std::vector<std::vector<Point>> paths;
    std::vector<Edge> edges;            // In path order, so each path's edges are contiguous.
    std::vector<int> pathEdgeBegin;     // paths.size()+1 entries; path p owns edges [begin[p], begin[p+1]).
    std::vector<char> pathClosed;       // First and last vertex coincide exactly.
    std::string error;
};

// Reuses the capacity of every buffer in g, including the per-path vertex vectors.
inline void parse(const std::vector<float>& x, const std::vector<float>& y, Geometry& g) {
    g.edges.clear(); g.pathEdgeBegin.clear(); g.pathClosed.clear(); g.error.clear();
    auto fail = [&](const char* message) {
        g.paths.clear(); g.edges.clear(); g.pathEdgeBegin.clear(); g.pathClosed.clear(); g.error = message;
    };
    if(x.size() != y.size()) { fail("In.X and In.Y must have equal sizes."); return; }
    // Previous paths are emptied, not freed: their vertex storage is reused.
    for(auto& path : g.paths) path.clear();
    size_t count = 0;
    auto current = [&]() -> std::vector<Point>& {
        if(count == g.paths.size()) g.paths.emplace_back();
        return g.paths[count];
    };
    auto flush = [&]() {
        if(count == g.paths.size() || g.paths[count].empty()) return;
        auto& path = g.paths[count];
        g.pathEdgeBegin.push_back(static_cast<int>(g.edges.size()));
        const int p = static_cast<int>(count);
        for(size_t i = 1; i < path.size(); ++i) {
            const double dx = path[i].x-path[i-1].x, dy = path[i].y-path[i-1].y;
            g.edges.push_back({path[i-1], path[i], p, static_cast<int>(i-1), std::sqrt(dx*dx+dy*dy)});
        }
        g.pathClosed.push_back(path.front().x == path.back().x && path.front().y == path.back().y);
        ++count;
    };
    for(size_t i = 0; i < x.size(); ++i) {
        if(!std::isfinite(x[i]) || !std::isfinite(y[i])) { fail("Input coordinates must be finite."); return; }
        if(x[i] == -1 || y[i] == -1) {
            if(x[i] != y[i]) { fail("Path separators (-1) must align in X and Y."); return; }
            flush();
        } else current().push_back({x[i], y[i]});
    }
    flush();
    g.paths.resize(count);
    g.pathEdgeBegin.push_back(static_cast<int>(g.edges.size()));
}

inline Geometry parse(const std::vector<float>& x, const std::vector<float>& y) {
    Geometry g;
    parse(x, y, g);
    return g;
}

struct Settings {
    Scope scope = Scope::Paths;
    Metric metric = Metric::Distance;
    Mapping startMapping = Mapping::Auto, endMapping = Mapping::Auto, originMapping = Mapping::Auto;
    std::vector<float> start{0}, end{1}, origin{0};
    std::vector<std::vector<int>> groups{{}};
    bool keepOrder = false, connected = true, endpointDots = false;
};

inline bool operator==(const Settings& a, const Settings& b) {
    return a.scope == b.scope && a.metric == b.metric && a.startMapping == b.startMapping &&
           a.endMapping == b.endMapping && a.originMapping == b.originMapping && a.keepOrder == b.keepOrder &&
           a.connected == b.connected && a.endpointDots == b.endpointDots && a.start == b.start &&
           a.end == b.end && a.origin == b.origin && a.groups == b.groups;
}
inline bool operator!=(const Settings& a, const Settings& b) { return !(a == b); }

struct Result {
    std::vector<float> x, y;
    // One slot per original edge (per group/edge occurrence in Groups scope).
    std::vector<float> fullX, fullY, wrappedX, wrappedY, completeness;
    std::vector<int> sourcePath, sourceEdge, sourceGroup;
    bool valid = true;
    std::string status;
};

// Exact endpoint deduplication within each source path/group occurrence, as an
// open-addressing set. Entries are stamped with a generation so that clearing
// between updates costs nothing; the table only ever grows.
struct DotSet {
    struct Key { int group = 0, path = 0; double x = 0, y = 0; };
    std::vector<Key> keys;
    std::vector<uint32_t> stamps;
    uint32_t generation = 0;
    size_t mask = 0;

    void begin(size_t expected) {
        size_t capacity = 64;
        while(capacity < expected*2) capacity <<= 1;
        if(capacity > keys.size() || generation == UINT32_MAX) {
            keys.assign(std::max(capacity, keys.size()), Key{});
            stamps.assign(keys.size(), 0);
            generation = 0;
        }
        mask = keys.size()-1;
        ++generation;
    }
    static uint64_t mix(uint64_t v) {
        v ^= v >> 33; v *= 0xff51afd7ed558ccdULL; v ^= v >> 33; v *= 0xc4ceb9fe1a85ec53ULL; v ^= v >> 33;
        return v;
    }
    bool insert(int group, int path, Point p) {
        uint64_t bx, by;
        std::memcpy(&bx, &p.x, sizeof bx); std::memcpy(&by, &p.y, sizeof by);
        const uint64_t ids = (uint64_t(uint32_t(group)) << 32) | uint32_t(path);
        size_t h = static_cast<size_t>(mix(bx ^ mix(by ^ mix(ids)))) & mask;
        while(stamps[h] == generation) {
            const Key& k = keys[h];
            if(k.group == group && k.path == path && k.x == p.x && k.y == p.y) return false;
            h = (h+1) & mask;
        }
        stamps[h] = generation;
        keys[h] = {group, path, p.x, p.y};
        return true;
    }
};

// Scratch buffers that keep their capacity between trim() calls.
struct Workspace {
    struct Unit { int begin, end, group; };  // Range into edgeOrder.
    std::vector<Unit> units;
    std::vector<int> edgeOrder;              // Edge index of every stable slot, in slot order.
    std::vector<int> seenGroup;              // Groups scope: last group that used each path, +1.
    std::vector<Point> dots;
    DotSet dotSet;
};

inline const char* mappingName(Mapping m) {
    switch(m) {
        case Mapping::Scalar: return "Scalar";
        case Mapping::Path: return "Per Path";
        case Mapping::Edge: return "Per Edge";
        case Mapping::Group: return "Per Group";
        default: return "Auto";
    }
}

inline bool mappingAllowed(Mapping m, Scope s) {
    if(m == Mapping::Scalar) return true;
    if(m == Mapping::Group) return s == Scope::Groups;
    if(m == Mapping::Path) return s == Scope::Edges || s == Scope::Paths;
    return m == Mapping::Edge && s == Scope::Edges;
}

inline size_t mappingSize(Mapping m, const Geometry& g, const Settings& s) {
    switch(m) {
        case Mapping::Path: return g.paths.size();
        case Mapping::Edge: return g.edges.size();
        case Mapping::Group: return s.groups.size();
        default: return 1;
    }
}

inline bool resolve(const std::vector<float>& v, Mapping requested,
                    const char* name, const Geometry& g, const Settings& s,
                    Mapping& resolved, std::string& error) {
    if(v.empty()) { error = std::string(name) + " is empty; supply a scalar or a mapped vector."; return false; }
    for(float value : v) if(!std::isfinite(value)) {
        error = std::string(name) + " contains a nonfinite value."; return false;
    }
    if(requested != Mapping::Auto) {
        if(!mappingAllowed(requested, s.scope)) {
            error = std::string(name) + ": mapping is unavailable in this scope."; return false;
        }
        const size_t expected = mappingSize(requested, g, s);
        if(v.size() != expected) {
            error = std::string(name) + ": " + mappingName(requested) + " expects " +
                    std::to_string(expected) + " values, got " + std::to_string(v.size()) + ".";
            return false;
        }
        resolved = requested;
        return true;
    }
    if(v.size() == 1) { resolved = Mapping::Scalar; return true; }
    int matches = 0;
    for(Mapping m : {Mapping::Path, Mapping::Edge, Mapping::Group}) {
        if(mappingAllowed(m, s.scope) && v.size() == mappingSize(m, g, s)) {
            resolved = m; ++matches;
        }
    }
    if(matches == 1) return true;
    error = std::string(name) + (matches ? ": ambiguous vector size; select an explicit mapping."
                                        : ": size does not match a supported mapping.");
    return false;
}

inline double valueAt(const std::vector<float>& v, Mapping m, const Edge& e, int edge, int group) {
    size_t index = 0;
    if(m == Mapping::Path) index = static_cast<size_t>(e.path);
    if(m == Mapping::Edge) index = static_cast<size_t>(edge);
    if(m == Mapping::Group) index = static_cast<size_t>(group);
    return std::max(0.0, std::min(1.0, static_cast<double>(v[index])));
}

// Exact at both ends: t == 0 gives a and t == 1 gives b bit for bit, so pieces
// of neighbouring edges share their vertex and endpoint dots deduplicate.
inline Point interpolate(const Edge& e, double t) {
    if(t <= 0) return e.a;
    if(t >= 1) return e.b;
    return {e.a.x + t*(e.b.x-e.a.x), e.a.y + t*(e.b.y-e.a.y)};
}

inline void trim(const Geometry& g, const Settings& s, Result& r, Workspace& w) {
    r.x.clear(); r.y.clear();
    r.fullX.clear(); r.fullY.clear(); r.wrappedX.clear(); r.wrappedY.clear(); r.completeness.clear();
    r.sourcePath.clear(); r.sourceEdge.clear(); r.sourceGroup.clear();
    r.valid = true; r.status.clear();
    auto fail = [&](const std::string& message) { r.valid = false; r.status = message; };
    if(!g.error.empty()) { fail(g.error); return; }
    if(static_cast<int>(s.scope) < 0 || static_cast<int>(s.scope) > 3 ||
       static_cast<int>(s.metric) < 0 || static_cast<int>(s.metric) > 1) {
        fail("Invalid scope or progress metric."); return;
    }
    // Units are ranges over edgeOrder, which lists the edge of every stable
    // slot in slot order: the identity outside Groups scope.
    const int edgeTotal = static_cast<int>(g.edges.size());
    w.units.clear(); w.edgeOrder.clear();
    size_t ignoredGroupPaths = 0;
    if(s.scope == Scope::Groups) {
        w.seenGroup.assign(g.paths.size(), 0);
        for(size_t group = 0; group < s.groups.size(); ++group) {
            const int begin = static_cast<int>(w.edgeOrder.size());
            for(int path : s.groups[group]) {
                if(path < 0 || static_cast<size_t>(path) >= g.paths.size()) { ++ignoredGroupPaths; continue; }
                if(w.seenGroup[path] == static_cast<int>(group)+1) continue;
                w.seenGroup[path] = static_cast<int>(group)+1;
                for(int e = g.pathEdgeBegin[path]; e < g.pathEdgeBegin[path+1]; ++e) w.edgeOrder.push_back(e);
            }
            w.units.push_back({begin, static_cast<int>(w.edgeOrder.size()), static_cast<int>(group)});
        }
    } else {
        w.edgeOrder.resize(edgeTotal);
        std::iota(w.edgeOrder.begin(), w.edgeOrder.end(), 0);
        if(s.scope == Scope::Edges) {
            for(int e = 0; e < edgeTotal; ++e) w.units.push_back({e, e+1, -1});
        } else if(s.scope == Scope::Paths) {
            for(size_t p = 0; p < g.paths.size(); ++p) w.units.push_back({g.pathEdgeBegin[p], g.pathEdgeBegin[p+1], -1});
        } else {
            w.units.push_back({0, edgeTotal, -1});
        }
    }
    // Initialize every stable slot before validation, including invisible edges.
    const size_t slots = w.edgeOrder.size();
    r.fullX.assign(slots*3, 0); r.fullY.assign(slots*3, 0);
    r.wrappedX.assign(slots*3, 0); r.wrappedY.assign(slots*3, 0);
    for(size_t i = 2; i < slots*3; i += 3) r.fullX[i] = r.fullY[i] = r.wrappedX[i] = r.wrappedY[i] = -1;
    r.completeness.assign(slots, 0);
    r.sourcePath.resize(slots); r.sourceEdge.resize(slots); r.sourceGroup.resize(slots);
    for(const auto& unit : w.units) for(int i = unit.begin; i < unit.end; ++i) {
        const int index = w.edgeOrder[i];
        r.sourcePath[i] = g.edges[index].path; r.sourceEdge[i] = index; r.sourceGroup[i] = unit.group;
    }
    Mapping sm, em, om;
    std::string error;
    if(!resolve(s.start, s.startMapping, "Start", g, s, sm, error) ||
       !resolve(s.end, s.endMapping, "End", g, s, em, error) ||
       !resolve(s.origin, s.originMapping, "Origin", g, s, om, error)) {
        fail(error); return;
    }
    r.status += "Start: "; r.status += mappingName(sm);
    r.status += "; End: "; r.status += mappingName(em);
    r.status += "; Origin: "; r.status += mappingName(om);
    if(ignoredGroupPaths) r.status += "; ignored " + std::to_string(ignoredGroupPaths) + " unavailable group paths";
    if(g.edges.empty()) r.status += "; no edges";
    else if(slots == 0) r.status += "; select paths in the group editor";

    const bool distance = s.metric == Metric::Distance;
    bool separatorCollision = false;
    auto vertex = [&](Point p) {
        const float x = static_cast<float>(p.x), y = static_cast<float>(p.y);
        separatorCollision = separatorCollision || x == -1 || y == -1;
        r.x.push_back(x); r.y.push_back(y);
    };
    bool open = false;
    auto close = [&]() { if(open) { r.x.push_back(-1); r.y.push_back(-1); open = false; } };
    w.dots.clear();
    if(s.endpointDots) w.dotSet.begin(slots*4);
    int previousEdge = -1, previousGroup = -2;
    double previousEnd = 0;
    for(const auto& unit : w.units) {
        if(unit.begin == unit.end) continue;
        double total = unit.end-unit.begin;
        if(distance) { total = 0; for(int i = unit.begin; i < unit.end; ++i) total += g.edges[w.edgeOrder[i]].length; }
        // Controls are uniform within each unit: an edge, a path, the full input, or a group.
        const int first = w.edgeOrder[unit.begin];
        const Edge& firstEdge = g.edges[first];
        double start = valueAt(s.start, sm, firstEdge, first, unit.group);
        double end = valueAt(s.end, em, firstEdge, first, unit.group);
        const double origin = valueAt(s.origin, om, firstEdge, first, unit.group);
        if(!s.keepOrder && start > end) std::swap(start, end);
        if(start >= end || total == 0) { close(); previousEdge = -1; continue; }
        // Shift the interval, not its length. Preserve the full [0,1] range as a full turn.
        // start < 1 and origin <= 1 here, so this is fmod(start+origin, 1) without the call.
        const double shifted = start + origin;
        const double shiftedStart = shifted >= 1 ? shifted-1 : shifted;
        const double shiftedEnd = shiftedStart + (end-start);
        const double intervals[2][2] = {{shiftedStart, std::min(1.0, shiftedEnd)}, {0, shiftedEnd-1}};
        const int intervalCount = shiftedEnd > 1 ? 2 : 1;
        for(int k = 0; k < intervalCount; ++k) {
            const double lo = intervals[k][0], hi = intervals[k][1];
            double cursor = 0;
            for(int i = unit.begin; i < unit.end; ++i) {
                const size_t slot = static_cast<size_t>(i);
                const int index = w.edgeOrder[i];
                const Edge& e = g.edges[index];
                const double weight = distance ? e.length : 1;
                double a = lo, b = hi;
                if(s.scope != Scope::Edges && weight > 0) {
                    a = std::max(0.0, (lo*total-cursor)/weight);
                    b = std::min(1.0, (hi*total-cursor)/weight);
                }
                cursor += weight;
                if(a >= b || e.length == 0 || weight == 0) continue;
                const Point p = interpolate(e, a), q = interpolate(e, b);
                // A seam can cut an edge twice. Keep its second piece in a parallel stable slot.
                auto& fixedX = r.completeness[slot] > 0 ? r.wrappedX : r.fullX;
                auto& fixedY = r.completeness[slot] > 0 ? r.wrappedY : r.fullY;
                fixedX[slot*3] = static_cast<float>(p.x); fixedX[slot*3+1] = static_cast<float>(q.x);
                fixedY[slot*3] = static_cast<float>(p.y); fixedY[slot*3+1] = static_cast<float>(q.y);
                r.completeness[slot] = static_cast<float>(std::min(1.0, r.completeness[slot] + b-a));
                const bool closedSeam = previousEdge >= 0 && e.local == 0 && g.pathClosed[e.path] &&
                    g.edges[previousEdge].local == static_cast<int>(g.paths[e.path].size())-2;
                const bool joins = s.connected && open && previousEdge >= 0 && previousGroup == unit.group &&
                    g.edges[previousEdge].path == e.path &&
                    (g.edges[previousEdge].local+1 == e.local || closedSeam) && previousEnd == 1 && a == 0;
                if(!joins) { close(); vertex(p); }
                vertex(q); open = true;
                previousEdge = index; previousGroup = unit.group; previousEnd = b;
                if(s.endpointDots) for(Point point : {p, q}) {
                    if(w.dotSet.insert(unit.group, e.path, point)) w.dots.push_back(point);
                }
            }
        }
        // Edge scope may reconnect adjacent fully visible edges; other units are independent.
        if(s.scope != Scope::Edges) { close(); previousEdge = -1; }
    }
    close();
    for(const auto& dot : w.dots) { vertex(dot); open = true; close(); }
    if(separatorCollision) {
        // The legacy XY format cannot represent a vertex with either coordinate exactly -1.
        r.x.clear(); r.y.clear();
        std::fill(r.completeness.begin(), r.completeness.end(), 0);
        for(size_t i = 0; i < slots; ++i) {
            r.fullX[i*3] = r.fullX[i*3+1] = r.fullY[i*3] = r.fullY[i*3+1] = 0;
            r.wrappedX[i*3] = r.wrappedX[i*3+1] = r.wrappedY[i*3] = r.wrappedY[i*3+1] = 0;
        }
        fail("A trimmed coordinate equals reserved separator -1; transform the input coordinates first.");
    }
}

inline Result trim(const Geometry& g, const Settings& s) {
    Result r;
    Workspace w;
    trim(g, s, r, w);
    return r;
}
} // namespace trimPathsProCore
