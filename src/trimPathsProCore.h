#pragma once

#include <algorithm>
#include <cmath>
#include <set>
#include <string>
#include <utility>
#include <vector>

// Independent of openFrameworks so the geometry and vector contract can be tested.
namespace trimPathsProCore {
enum class Scope { Edges, Paths, AllPaths, Groups };
enum class Mapping { Auto, Scalar, Path, Edge, Group };
enum class Metric { EdgeCount, Distance };

struct Point { double x = 0, y = 0; };
struct Edge { Point a, b; int path = 0, local = 0; double length = 0; };
struct Geometry {
    std::vector<std::vector<Point>> paths;
    std::vector<Edge> edges;
    std::vector<std::vector<int>> pathEdges;
    std::string error;
};

inline Geometry parse(const std::vector<float>& x, const std::vector<float>& y) {
    Geometry g;
    if(x.size() != y.size()) { g.error = "In.X and In.Y must have equal sizes."; return g; }
    std::vector<Point> path;
    auto flush = [&]() {
        if(path.empty()) return;
        const int p = static_cast<int>(g.paths.size());
        g.pathEdges.emplace_back();
        for(size_t i = 1; i < path.size(); ++i) {
            g.pathEdges.back().push_back(static_cast<int>(g.edges.size()));
            g.edges.push_back({path[i-1], path[i], p, static_cast<int>(i-1),
                              std::hypot(path[i].x-path[i-1].x, path[i].y-path[i-1].y)});
        }
        g.paths.push_back(std::move(path));
        path.clear();
    };
    for(size_t i = 0; i < x.size(); ++i) {
        if(!std::isfinite(x[i]) || !std::isfinite(y[i])) {
            g = Geometry{}; g.error = "Input coordinates must be finite."; return g;
        }
        if(x[i] == -1 || y[i] == -1) {
            if(x[i] != y[i]) {
                g = Geometry{}; g.error = "Path separators (-1) must align in X and Y."; return g;
            }
            flush();
        } else path.push_back({x[i], y[i]});
    }
    flush();
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

struct Result {
    std::vector<float> x, y;
    // One slot per original edge (per group/edge occurrence in Groups scope).
    std::vector<float> fullX, fullY, wrappedX, wrappedY, completeness;
    std::vector<int> sourcePath, sourceEdge, sourceGroup;
    bool valid = true;
    std::string status;
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

struct Unit { std::vector<int> edges; int group = -1; };
inline Point interpolate(const Edge& e, double t) {
    return {e.a.x + t*(e.b.x-e.a.x), e.a.y + t*(e.b.y-e.a.y)};
}

inline Result trim(const Geometry& g, const Settings& s) {
    Result r;
    auto fail = [&](const std::string& message) { r.valid = false; r.status = message; };
    if(!g.error.empty()) { fail(g.error); return r; }
    if(static_cast<int>(s.scope) < 0 || static_cast<int>(s.scope) > 3 ||
       static_cast<int>(s.metric) < 0 || static_cast<int>(s.metric) > 1) {
        fail("Invalid scope or progress metric."); return r;
    }
    std::vector<Unit> units;
    size_t ignoredGroupPaths = 0;
    if(s.scope == Scope::Edges) {
        for(size_t e = 0; e < g.edges.size(); ++e) units.push_back({{static_cast<int>(e)}, -1});
    } else if(s.scope == Scope::Paths) {
        for(const auto& edges : g.pathEdges) units.push_back({edges, -1});
    } else if(s.scope == Scope::AllPaths) {
        units.push_back({{}, -1});
        for(size_t e = 0; e < g.edges.size(); ++e) units.back().edges.push_back(static_cast<int>(e));
    } else {
        for(size_t group = 0; group < s.groups.size(); ++group) {
            units.push_back({{}, static_cast<int>(group)});
            std::set<int> seen;
            for(int path : s.groups[group]) {
                if(path < 0 || static_cast<size_t>(path) >= g.paths.size()) { ++ignoredGroupPaths; continue; }
                if(!seen.insert(path).second) continue;
                const auto& edges = g.pathEdges[path];
                units.back().edges.insert(units.back().edges.end(), edges.begin(), edges.end());
            }
        }
    }
    // Initialize every stable slot before validation, including invisible edges.
    for(const auto& unit : units) for(int index : unit.edges) {
        r.fullX.insert(r.fullX.end(), {0, 0, -1});
        r.fullY.insert(r.fullY.end(), {0, 0, -1});
        r.wrappedX.insert(r.wrappedX.end(), {0, 0, -1});
        r.wrappedY.insert(r.wrappedY.end(), {0, 0, -1});
        r.completeness.push_back(0);
        r.sourcePath.push_back(g.edges[index].path);
        r.sourceEdge.push_back(index);
        r.sourceGroup.push_back(unit.group);
    }
    Mapping sm, em, om;
    std::string error;
    if(!resolve(s.start, s.startMapping, "Start", g, s, sm, error) ||
       !resolve(s.end, s.endMapping, "End", g, s, em, error) ||
       !resolve(s.origin, s.originMapping, "Origin", g, s, om, error)) {
        fail(error); return r;
    }
    r.status = std::string("Start: ") + mappingName(sm) + "; End: " + mappingName(em) + "; Origin: " + mappingName(om);
    if(ignoredGroupPaths) r.status += "; ignored " + std::to_string(ignoredGroupPaths) + " unavailable group paths";
    if(g.edges.empty()) r.status += "; no edges";
    else if(r.completeness.empty()) r.status += "; select paths in the group editor";

    bool separatorCollision = false;
    auto vertex = [&](Point p) {
        const float x = static_cast<float>(p.x), y = static_cast<float>(p.y);
        separatorCollision = separatorCollision || x == -1 || y == -1;
        r.x.push_back(x); r.y.push_back(y);
    };
    bool open = false;
    auto close = [&]() { if(open) { r.x.push_back(-1); r.y.push_back(-1); open = false; } };
    std::vector<Point> dots;
    // Exact endpoint deduplication within each source path/group occurrence.
    std::set<std::pair<std::pair<int,int>, std::pair<double,double>>> dotKeys;
    int previousEdge = -1, previousGroup = -2;
    double previousEnd = 0;
    size_t unitSlot = 0;
    for(const auto& unit : units) {
        if(unit.edges.empty()) continue;
        double total = 0;
        for(int index : unit.edges) total += s.metric == Metric::Distance ? g.edges[index].length : 1;
        // Controls are uniform within each unit: an edge, a path, the full input, or a group.
        const int first = unit.edges.front();
        const Edge& firstEdge = g.edges[first];
        double start = valueAt(s.start, sm, firstEdge, first, unit.group);
        double end = valueAt(s.end, em, firstEdge, first, unit.group);
        const double origin = valueAt(s.origin, om, firstEdge, first, unit.group);
        if(!s.keepOrder && start > end) std::swap(start, end);
        if(start >= end || total == 0) {
            close(); previousEdge = -1; unitSlot += unit.edges.size(); continue;
        }
        // Shift the interval, not its length. Preserve the full [0,1] range as a full turn.
        const double shiftedStart = std::fmod(start + origin, 1.0);
        const double shiftedEnd = shiftedStart + (end-start);
        std::vector<std::pair<double,double>> intervals{{shiftedStart, std::min(1.0, shiftedEnd)}};
        if(shiftedEnd > 1) intervals.push_back({0, shiftedEnd-1});
        for(const auto& interval : intervals) {
            double cursor = 0;
            for(size_t offset = 0; offset < unit.edges.size(); ++offset) {
                const size_t slot = unitSlot + offset;
                const int index = unit.edges[offset];
                const Edge& e = g.edges[index];
                const double weight = s.metric == Metric::Distance ? e.length : 1;
                double a = interval.first, b = interval.second;
                if(s.scope != Scope::Edges && weight > 0) {
                    a = std::max(0.0, (interval.first*total-cursor)/weight);
                    b = std::min(1.0, (interval.second*total-cursor)/weight);
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
                const auto& path = g.paths[e.path];
                const bool closedSeam = previousEdge >= 0 && e.local == 0 &&
                    g.edges[previousEdge].local == static_cast<int>(path.size())-2 &&
                    path.front().x == path.back().x && path.front().y == path.back().y;
                const bool joins = s.connected && open && previousEdge >= 0 && previousGroup == unit.group &&
                    g.edges[previousEdge].path == e.path &&
                    (g.edges[previousEdge].local+1 == e.local || closedSeam) && previousEnd == 1 && a == 0;
                if(!joins) { close(); vertex(p); }
                vertex(q); open = true;
                previousEdge = index; previousGroup = unit.group; previousEnd = b;
                if(s.endpointDots) for(Point point : {p, q}) {
                    if(dotKeys.insert({{unit.group, e.path}, {point.x, point.y}}).second) dots.push_back(point);
                }
            }
        }
        unitSlot += unit.edges.size();
        // Edge scope may reconnect adjacent fully visible edges; other units are independent.
        if(s.scope != Scope::Edges) { close(); previousEdge = -1; }
    }
    close();
    for(const auto& dot : dots) { vertex(dot); open = true; close(); }
    if(separatorCollision) {
        // The legacy XY format cannot represent a vertex with either coordinate exactly -1.
        r.x.clear(); r.y.clear();
        std::fill(r.completeness.begin(), r.completeness.end(), 0);
        for(size_t i = 0; i < r.completeness.size(); ++i) {
            r.fullX[i*3] = r.fullX[i*3+1] = r.fullY[i*3] = r.fullY[i*3+1] = 0;
            r.wrappedX[i*3] = r.wrappedX[i*3+1] = r.wrappedY[i*3] = r.wrappedY[i*3+1] = 0;
        }
        fail("A trimmed coordinate equals reserved separator -1; transform the input coordinates first.");
    }
    return r;
}
} // namespace trimPathsProCore
