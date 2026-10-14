#include "../src/trimPathsProCore.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <random>

using namespace trimPathsProCore;
static int checks = 0;
static void checkAt(bool ok, int line) {
    if(!ok) { std::cerr << "Check failed at line " << line << '\n'; std::abort(); }
    ++checks;
}
#define check(...) checkAt((__VA_ARGS__), __LINE__)
static bool near(float a, float b) { return std::abs(a-b) < 1e-5f; }
static void invariant(const Result& r) {
    check(r.x.size() == r.y.size());
    for(size_t i = 0; i < r.x.size(); ++i) {
        check(std::isfinite(r.x[i]) && std::isfinite(r.y[i]));
        check((r.x[i] == -1) == (r.y[i] == -1));
    }
    check(r.fullX.size() == r.completeness.size()*3 && r.fullY.size() == r.fullX.size());
    check(r.wrappedX.size() == r.fullX.size() && r.wrappedY.size() == r.fullY.size());
    check(r.sourcePath.size() == r.completeness.size() && r.sourceEdge.size() == r.sourcePath.size() &&
          r.sourceGroup.size() == r.sourcePath.size());
    for(size_t i = 0; i < r.completeness.size(); ++i) {
        check(std::isfinite(r.completeness[i]) && r.completeness[i] >= 0 && r.completeness[i] <= 1);
        check(r.fullX[i*3+2] == -1 && r.fullY[i*3+2] == -1);
        check(r.wrappedX[i*3+2] == -1 && r.wrappedY[i*3+2] == -1);
        for(int j = 0; j < 2; ++j) {
            check(std::isfinite(r.fullX[i*3+j]) && std::isfinite(r.fullY[i*3+j]));
            check(std::isfinite(r.wrappedX[i*3+j]) && std::isfinite(r.wrappedY[i*3+j]));
        }
    }
}

int main() {
    const auto g = parse({0,.25f,1,-1,0,1}, {0,0,0,-1,1,1});
    check(g.paths.size() == 2 && g.edges.size() == 3);
    Settings s;
    s.end = {.5f,.75f};
    auto r = trim(g, s);
    check(r.valid && r.x == std::vector<float>({0,.25f,.5f,-1,0,.75f,-1}));
    check(r.sourcePath == std::vector<int>({0,0,1}));
    check(r.sourceEdge == std::vector<int>({0,1,2}));
    check(near(r.completeness[1], 1.0f/3)); invariant(r);

    // Same inputs, fundamentally different local-edge interpretation.
    s.scope = Scope::Edges;
    r = trim(g,s);
    check(r.valid && r.x == std::vector<float>({0,.125f,-1,.25f,.625f,-1,0,.75f,-1}));
    s.start = {.2f,.4f}; s.end = {.5f,.6f,.8f};
    r = trim(g,s);
    check(r.valid && near(r.fullX[0], .05f) && near(r.fullX[3], .4f) && near(r.fullX[6], .4f));

    s = Settings{}; s.scope = Scope::AllPaths; s.end = {.5f}; s.metric = Metric::EdgeCount;
    r = trim(g,s);
    check(r.valid && near(r.x[r.x.size()-2], .625f));
    s.metric = Metric::Distance; r = trim(g,s);
    check(r.x == std::vector<float>({0,.25f,1,-1}));
    s.end = {.5f,.9f}; check(!trim(g,s).valid); // Never silently ignore additional values.

    // Scope paths uses its own progress interval for each differently sized path.
    s = Settings{}; s.metric = Metric::EdgeCount; s.end = {.5f};
    r = trim(g,s); check(r.x == std::vector<float>({0,.25f,-1,0,.5f,-1}));
    s.start = {.75f}; s.end = {.25f};
    r = trim(g,s); check(r.valid && !r.x.empty());
    s.keepOrder = true; r = trim(g,s); check(r.x.empty()); invariant(r);
    for(Scope scope : {Scope::Edges, Scope::Paths, Scope::AllPaths, Scope::Groups}) {
        s = Settings{}; s.scope = scope; s.groups = {{0,1}}; s.start = {.5f}; s.end = {.5f};
        r = trim(g,s); check(r.valid && r.x.empty()); invariant(r);
        s.start = {2}; s.end = {3}; r = trim(g,s); check(r.x.empty()); invariant(r);
        s.start = {-2}; s.end = {3}; r = trim(g,s); check(r.valid && !r.x.empty()); invariant(r);
    }

    s = Settings{}; s.start = {}; r = trim(g,s); check(!r.valid && r.x.empty()); invariant(r);
    s.start = {std::numeric_limits<float>::quiet_NaN()}; check(!trim(g,s).valid);
    s.start = {0}; s.end = {std::numeric_limits<float>::infinity()}; check(!trim(g,s).valid);
    s.end = {1}; s.startMapping = Mapping::Edge; check(!trim(g,s).valid);
    s.startMapping = Mapping::Auto; s.end = {.1f,.2f,.3f,.4f};
    r = trim(g,s); check(!r.valid); invariant(r);

    // A singleton plus a two-edge path: path count == edge count is genuinely ambiguous.
    const auto ambiguous = parse({0,-1,0,.5f,1}, {0,-1,1,1,1});
    s = Settings{}; s.scope = Scope::Edges; s.end = {.2f,.8f};
    check(!trim(ambiguous,s).valid);
    s.endMapping = Mapping::Path; r = trim(ambiguous,s); check(r.valid && near(r.fullX[1], .4f));
    s.endMapping = Mapping::Edge; r = trim(ambiguous,s); check(r.valid && near(r.fullX[1], .1f));

    // Overlapping groups must emit separate fragments, with stable occurrence IDs.
    const auto line = parse({0,1}, {0,0});
    s = Settings{}; s.scope = Scope::Groups; s.groups = {{0},{0}};
    s.start = {0,.75f}; s.end = {.25f,1};
    r = trim(line,s);
    check(r.x == std::vector<float>({0,.25f,-1,.75f,1,-1}));
    check(r.sourceEdge == std::vector<int>({0,0}) && r.sourceGroup == std::vector<int>({0,1})); invariant(r);
    s.start = {0}; s.end = {.5f}; r = trim(line,s);
    check(r.x == std::vector<float>({0,.5f,-1,0,.5f,-1}));
    s.groups = {{0,0,-1,400},{}}; r = trim(line,s);
    check(r.valid && r.completeness.size() == 1 && r.status.find("unavailable") != std::string::npos);
    s.groups = {{},{}}; r = trim(line,s); check(r.valid && r.x.empty() && r.fullX.empty());

    s = Settings{}; s.endpointDots = true;
    r = trim(g,s); check(std::count_if(r.x.begin(), r.x.end(), [](float x) { return x != -1; }) == 10); invariant(r); // Five line vertices + five unique endpoints.
    s.endpointDots = false; s.connected = false;
    r = trim(g,s); check(r.x == std::vector<float>({0,.25f,-1,.25f,1,-1,0,1,-1}));
    s.scope = Scope::Edges; s.connected = true;
    r = trim(g,s); check(r.x == std::vector<float>({0,.25f,1,-1,0,1,-1}));

    check(!parse({0,1},{0}).error.empty());
    check(!parse({0,-1,1},{0,0,1}).error.empty());
    check(!parse({0,std::numeric_limits<float>::infinity()},{0,1}).error.empty());
    check(parse({-1,-1,0,1,-1,-1},{-1,-1,0,0,-1,-1}).paths.size() == 1);
    s = Settings{};
    r = trim(parse({},{}),s); check(r.valid && r.x.empty());
    r = trim(parse({.5f},{.5f}),s); check(r.valid && r.x.empty());
    r = trim(parse({0,0,0},{0,0,0}),s); check(r.valid && r.x.empty()); invariant(r);
    r = trim(parse({0,0,1},{0,0,0}),s); check(r.valid && r.x == std::vector<float>({0,1,-1})); invariant(r);
    r = trim(parse({0,1e-8f},{0,0}),s); check(r.x.size() == 3); // Tiny geometry is retained.
    s.end = {.5f};
    r = trim(parse({-2,0},{0,0}),s); check(!r.valid && r.x.empty()); invariant(r);
    const auto uneven = parse({0,.1f,1,-1,0,.2f,.5f,1}, {0,0,0,-1,1,1,1,1});
    s.end = {.5f,.75f}; r = trim(uneven,s); check(r.valid); invariant(r);

    // Origin moves the complete interval, preserving length across the seam.
    s = Settings{}; s.end = {.25f}; s.origin = {.875f};
    r = trim(line,s);
    check(r.valid && r.x == std::vector<float>({.875f,1,-1,0,.125f,-1}));
    check(r.fullX == std::vector<float>({.875f,1,-1}));
    check(r.wrappedX == std::vector<float>({0,.125f,-1}));
    check(r.completeness == std::vector<float>({.25f})); invariant(r);
    s.endpointDots = true; r = trim(line,s);
    check(r.x == std::vector<float>({.875f,1,-1,0,.125f,-1,.875f,-1,1,-1,0,-1,.125f,-1}));
    s.endpointDots = false; s.origin = {1};
    check(trim(line,s).x == std::vector<float>({0,.25f,-1}));
    s.origin = {0}; check(trim(line,s).x == std::vector<float>({0,.25f,-1}));
    s.origin = {.5f}; s.start = {.25f}; s.end = {.75f};
    r = trim(line,s); check(r.x == std::vector<float>({.75f,1,-1,0,.25f,-1}));
    s.start = {.75f}; s.end = {.25f};
    check(trim(line,s).x == r.x); // Keep Order is applied before moving the interval.
    s.keepOrder = true; r = trim(line,s); check(r.valid && r.x.empty()); invariant(r);
    s.start = {.25f}; s.end = {.25f}; r = trim(line,s); check(r.x.empty());

    // Independent per-path, per-edge and per-group origins use the same scope rules.
    s = Settings{}; s.end = {.5f}; s.origin = {.25f,.75f};
    r = trim(g,s);
    check(r.valid && r.x == std::vector<float>({.25f,.75f,-1,.75f,1,-1,0,.25f,-1})); invariant(r);
    s.originMapping = Mapping::Path; check(trim(g,s).valid);
    s.originMapping = Mapping::Scalar; check(!trim(g,s).valid);
    s = Settings{}; s.scope = Scope::Edges; s.end = {.25f}; s.origin = {0,.5f,.875f};
    r = trim(g,s);
    check(r.valid && near(r.fullX[3], .625f) && near(r.fullX[4], .8125f));
    check(near(r.wrappedX[7], .125f)); invariant(r);
    s.origin = {.25f,.75f}; s.originMapping = Mapping::Path;
    check(trim(g,s).valid); // Per-path origins can drive local edge trimming too.
    s.origin = {}; r = trim(g,s); check(!r.valid); invariant(r);
    s.origin = {std::numeric_limits<float>::quiet_NaN()}; check(!trim(g,s).valid);
    s.origin = {0,0,0,0}; check(!trim(g,s).valid);
    s.origin = {.25f,.75f}; s.originMapping = Mapping::Auto;
    check(!trim(ambiguous,s).valid);
    s.originMapping = Mapping::Edge; check(trim(ambiguous,s).valid);
    s = Settings{}; s.scope = Scope::Groups; s.groups = {{0},{0}};
    s.end = {.25f}; s.origin = {.5f,.875f};
    r = trim(line,s);
    check(r.x == std::vector<float>({.5f,.75f,-1,.875f,1,-1,0,.125f,-1}));
    check(r.sourceGroup == std::vector<int>({0,1}));
    check(r.completeness == std::vector<float>({.25f,.25f})); invariant(r);
    s = Settings{}; s.scope = Scope::AllPaths; s.end = {.5f}; s.origin = {.75f};
    r = trim(g,s);
    check(r.x == std::vector<float>({.5f,1,-1,0,.25f,.5f,-1})); invariant(r);
    s.metric = Metric::EdgeCount; r = trim(g,s);
    check(r.x == std::vector<float>({.25f,1,-1,0,.1875f,-1})); invariant(r);
    s.origin = {0,.5f}; check(!trim(g,s).valid); // All Paths requires scalar origin.

    // Closed paths stay connected through their original seam. Open paths never gain a closing edge.
    const auto square = parse({0,1,1,0,0}, {0,0,1,1,0});
    s = Settings{}; s.end = {.5f}; s.origin = {.875f};
    r = trim(square,s);
    check(r.x == std::vector<float>({0,0,1,1,-1}));
    check(r.y == std::vector<float>({.5f,0,0,.5f,-1})); invariant(r);
    s.end = {1}; s.origin = {.125f}; r = trim(square,s);
    check(r.x == std::vector<float>({.5f,1,1,0,0,.5f,-1}));
    check(r.y == std::vector<float>({0,0,1,1,0,0,-1}));
    check(r.completeness == std::vector<float>({1,1,1,1}));
    check(r.wrappedX[0] == 0 && r.wrappedX[1] == .5f); invariant(r);
    s.connected = false; r = trim(square,s);
    check(std::count(r.x.begin(),r.x.end(),-1) == 5);
    s = Settings{}; s.origin = {.5f}; r = trim(line,s);
    check(r.x == std::vector<float>({.5f,1,-1,0,.5f,-1}));
    check(r.completeness == std::vector<float>({1})); invariant(r);
    s.end = {.5f};
    check(trim(parse({0,0,0},{0,0,0}),s).x.empty());
    r = trim(parse({0,0,1},{0,0,0}),s); check(r.x == std::vector<float>({.5f,1,-1})); invariant(r);

    // Randomized valid and invalid sizes/topologies; ASan/UBSan cover bounds and arithmetic.
    std::mt19937 rng(48193);
    for(int trial = 0; trial < 1000; ++trial) {
        std::vector<float> x, y;
        int paths = rng()%6;
        for(int p = 0; p < paths; ++p) {
            int count = 1+rng()%8;
            for(int i = 0; i < count; ++i) { x.push_back(float(rng()%20)/20); y.push_back(float(p)/6); }
            x.push_back(-1); y.push_back(-1);
        }
        auto geometry = parse(x,y);
        s = Settings{}; s.scope = static_cast<Scope>(rng()%4); s.metric = static_cast<Metric>(rng()%2);
        s.connected = rng()%2; s.endpointDots = rng()%2; s.keepOrder = rng()%2;
        s.groups = {{0,1,3},{1,2,4}};
        size_t startSize = rng()%2 ? 1 : geometry.paths.size();
        size_t endSize = rng()%2 ? 1 : geometry.edges.size();
        s.start.assign(startSize, float(int(rng()%40)-10)/20);
        s.end.assign(endSize, float(int(rng()%40)-10)/20);
        size_t originSize = rng()%3 ? 1 : geometry.paths.size();
        s.origin.assign(originSize, float(rng()%21)/20);
        r = trim(geometry,s); invariant(r);
        if(r.valid && s.scope != Scope::Groups && s.metric == Metric::Distance) {
            double visibleLength = 0;
            for(size_t i = 1; i < r.x.size(); ++i) if(r.x[i] != -1 && r.x[i-1] != -1)
                visibleLength += std::hypot(r.x[i]-r.x[i-1],r.y[i]-r.y[i-1]);
            double retainedLength = 0;
            for(size_t slot = 0; slot < r.completeness.size(); ++slot)
                retainedLength += geometry.edges[r.sourceEdge[slot]].length * r.completeness[slot];
            check(std::abs(visibleLength-retainedLength) < 1e-5); // No extra seam connectors.
        }
    }
    std::cout << "TrimPathsPro core: " << checks << " checks passed (including 1000 randomized cases).\n";
}
