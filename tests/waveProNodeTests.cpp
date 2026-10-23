#include "ofMain.h"
#include "ofxOceanodeConnection.h"
static float frameSeconds = 1.f / 60.f;
static float testFrameTime() { return frameSeconds; }
#define ofGetLastFrameTime testFrameTime
#include "../src/wavePro.h"
#undef ofGetLastFrameTime
#include <cassert>
#include <iostream>
void ofxOceanodeAbstractParameter::removeAllConnections() {
    assert(!hasInConnection() && !hasOutConnections() && !isScoped());
}
int main() {
    wavePro node; node.setup(); ofEventArgs event;
    auto vec = [&](const string& name) -> ofParameter<vector<float>>& { return node.parameter<vector<float>>(name); };
    auto tick = [&]() { node.update(event); };
    node.parameter<int>("Resample") = 0;          // keep input points so outputs compare 1:1
    vec("Normal Smooth") = {0};
    vec("In.X") = {0, .5f, 1}; vec("In.Y") = {0, 0, 0}; vec("Amount") = {.1f};
    tick();
    assert(vec("Out.X").get() == vec("In.X").get() && vec("Out.Y").get() == vec("In.Y").get());
    int notified = 0;
    auto counter = vec("Out.X").newListener([&](vector<float>&) { ++notified; });
    // Direct mode: batched, only recomputes when an input changed.
    vec("Wave In") = {1}; assert(notified == 0); tick(); assert(notified == 1);
    for(float v : vec("Out.Y").get()) assert(std::abs(v - .1f) < 1e-6);
    tick(); assert(notified == 1);
    // X listeners see the matching Y.
    bool paired = false;
    auto check = vec("Out.X").newListener([&](vector<float>&) {
        assert(std::abs(vec("Out.Y").get()[0] - .3f) < 1e-6); paired = true;
    });
    vec("Amount") = {.3f}; tick(); assert(paired); check.reset();
    // Spring animates without input changes and reaches the target.
    node.parameter<int>("Response") = 1;
    vec("Amount") = {.1f}; vec("Damping") = {1}; vec("Stiffness") = {200};
    for(int i = 0; i < 300; ++i) tick();
    assert(std::abs(vec("Out.Y").get()[1] - .1f) < 1e-4);
    // Preset load resets the membrane to the rest shape.
    // Preset load restarts the membrane on the current wave, at rest.
    node.presetHasLoaded(); frameSeconds = 0; tick();
    for(float v : vec("Out.Y").get()) assert(std::abs(v - .1f) < 1e-6);
    frameSeconds = 1.f / 60;
    // A float connection broadcasts into vector controls.
    ofParameter<float> scalar; scalar.set("Scalar", .5f, 0, 20);
    ofxOceanodeParameter<float> source; source.bindParameter(scalar);
    ofxOceanodeParameter<vector<float>> sink; sink.bindParameter(vec("Propagation"));
    {
        ofxOceanodeConnection<float, vector<float>> connection(source, sink, true);
        scalar = 2.f;
        assert(vec("Propagation").get() == vector<float>({2.f}));
    }
    // Bypass passes input through untouched, also in Spring; turning it off resumes the effect.
    node.parameter<bool>("Bypass") = true;
    vec("In.Y") = {.2f, .2f, .2f}; tick();
    assert(vec("Out.X").get() == vec("In.X").get() && vec("Out.Y").get() == vec("In.Y").get());
    node.parameter<bool>("Bypass") = false;
    for(int i = 0; i < 300; ++i) tick();
    assert(std::abs(vec("Out.Y").get()[1] - .3f) < 1e-4);
    // Resample: fixed point count per shape, whatever the input count.
    node.parameter<int>("Resample") = 64; tick();
    assert(vec("Out.X").get().size() == 64);
    vec("In.X") = {0, .2f, .5f, .7f, 1}; vec("In.Y") = {0, 0, 0, 0, 0}; tick();
    assert(vec("Out.X").get().size() == 64 && vec("Out.Y").get().size() == 64);
    node.parameter<int>("Resample") = 0;
    vec("In.X") = {0, .5f, 1}; vec("In.Y") = {0, 0, 0};
    // Invalid XY clears both outputs.
    vec("In.Y") = {0}; tick();
    assert(vec("Out.X").get().empty() && vec("Out.Y").get().empty());
    std::cout << "Wave Pro node: real OF events, batching, paired outputs, spring, presets, scalar connections passed.\n";
}
