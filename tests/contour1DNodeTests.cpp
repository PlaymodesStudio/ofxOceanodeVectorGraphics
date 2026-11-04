#include "ofMain.h"
#include "ofxOceanodeConnection.h"
#include "../src/contour1D.h"
#include <cassert>
#include <iostream>
void ofxOceanodeAbstractParameter::removeAllConnections() {
    assert(!hasInConnection() && !hasOutConnections() && !isScoped());
}
static void ellipse(vector<float>& x, vector<float>& y, float cx, float cy, float a, float b, int n = 120) {
    x.clear(); y.clear();
    for(int i = 0; i <= n; ++i) {
        const double t = 2 * PI * (i % n) / n;
        x.push_back(float(cx + a * std::cos(t))); y.push_back(float(cy + b * std::sin(t)));
    }
}
int main() {
    contour1D node; node.setup(); ofEventArgs event;
    auto vec = [&](const string& name) -> ofParameter<vector<float>>& { return node.parameter<vector<float>>(name); };
    auto tick = [&]() { node.update(event); };
    vector<float> x, y;
    ellipse(x, y, .5f, .5f, .3f, .1f);
    node.parameter<int>("Size") = 32;
    vec("In.X") = x; vec("In.Y") = y;
    int notified = 0;
    auto counter = vec("Out").newListener([&](vector<float>&) { ++notified; });
    tick();
    assert(notified == 1);
    const auto out = vec("Out").get();
    assert(out.size() == 32);
    for(int k = 0; k < 16; ++k) assert(std::abs(out[k] - out[31 - k]) < .01f);
    assert(std::abs(out[16] - .2f) < .01f);                   // full width at the middle
    assert(vec("Length").get().size() == 1 && std::abs(vec("Length").get()[0] - .6f) < .01f);
    assert(vec("Head.X").get().size() == 1 && vec("Spine.X").get().size() >= 16);
    // Listeners on Out see matching geometry.
    bool paired = false;
    auto check = vec("Out").newListener([&](vector<float>&) {
        assert(std::abs(vec("Head.Y").get()[0] - .8f) < .02f); paired = true;
    });
    ellipse(x, y, .5f, .8f, .3f, .1f);
    vec("In.X") = x; vec("In.Y") = y; tick();
    assert(paired); check.reset();
    tick(); assert(notified == 2);                             // nothing changed: no event
    // Gain and Normalize.
    node.parameter<float>("Gain") = 2; tick();
    assert(std::abs(vec("Out").get()[16] - .4f) < .02f);
    node.parameter<bool>("Normalize") = true; tick();
    float peak = 0; for(float f : vec("Out").get()) peak = std::max(peak, f);
    assert(std::abs(peak - 2.f) < 1e-5f);
    node.parameter<bool>("Normalize") = false; node.parameter<float>("Gain") = 1; tick();
    // Loop profile: 2 x Size, head at both ends.
    node.parameter<int>("Profile") = 3; tick();
    assert(vec("Out").get().size() == 64 && vec("Out").get()[0] < .03f && vec("Out").get()[63] < .03f);
    node.parameter<int>("Profile") = 0;
    // Two shapes: Largest, Index and All.
    vector<float> x2, y2; ellipse(x2, y2, .5f, .2f, .1f, .05f);
    x.push_back(-1); y.push_back(-1);
    x.insert(x.end(), x2.begin(), x2.end()); y.insert(y.end(), y2.begin(), y2.end());
    vec("In.X") = x; vec("In.Y") = y; tick();
    assert(vec("Out").get().size() == 32 && std::abs(vec("Out").get()[16] - .2f) < .01f);
    assert(vec("Length").get().size() == 2);
    node.parameter<int>("Shape") = 1; node.parameter<int>("Index") = 1; tick();
    assert(std::abs(vec("Out").get()[16] - .1f) < .01f);
    node.parameter<int>("Index") = 5; tick();
    assert(vec("Out").get().empty());
    node.parameter<int>("Shape") = 2; tick();
    assert(vec("Out").get().size() == 65 && vec("Out").get()[32] == -1);
    assert(std::count(vec("Spine.X").get().begin(), vec("Spine.X").get().end(), -1.f) == 1);
    // Bad input clears everything.
    vec("In.Y") = {0.5f}; tick();
    assert(vec("Out").get().empty() && vec("Spine.X").get().empty());
    std::cout << "contour1D node tests passed" << std::endl;
    return 0;
}
