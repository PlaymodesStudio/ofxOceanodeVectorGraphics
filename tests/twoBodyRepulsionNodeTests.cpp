#include "ofMain.h"
#include "ofxOceanodeConnection.h"
#include <cassert>
#include <iostream>

// Supply host frame duration without requiring a running window. OF parameters/events are real.
static float frameSeconds = 1.0f / 60.0f;
static float testFrameTime() { return frameSeconds; }
#define ofGetLastFrameTime testFrameTime
#include "../src/twoBodyRepulsion.h"
#undef ofGetLastFrameTime

#include "oceanodeHeadlessStubs.h"

int main() {
    twoBodyRepulsion n; n.setup();
    ofEventArgs event;
    auto v = [&](const string& key) -> ofParameter<vector<float>>& { return n.parameter<vector<float>>(key); };
    auto f = [&](const string& key) -> ofParameter<float>& { return n.parameter<float>(key); };
    auto tick = [&]() { n.update(event); };
    for(const string body : {"A", "B"}) {
        assert(v(body + " Radius").get() == vector<float>{0});
        assert(v(body + " Strength").get() == vector<float>{1});
        assert(v(body + " FearDistance").get() == vector<float>{.15f});
        assert(v(body + " Repulsion Strength").get() == vector<float>{40});
        assert(v(body + " Return Strength").get() == vector<float>{8});
        assert(v(body + " Damping").get() == vector<float>{3});
        v(body + " Return Strength") = {0}; v(body + " Damping") = {0};
    }
    assert(f("Border Margin").get() == 0 && f("Border Feather").get() == 0);
    v("A Body_X") = {.45f}; v("A Body_Y") = {.5f};
    v("B Body_X") = {.55f}; v("B Body_Y") = {.5f};
    tick(); // Initial reset publishes the exact rest shapes.
    assert(v("A Out_X").get() == vector<float>{.45f} && v("B Out_X").get() == vector<float>{.55f});
    tick(); assert(v("A Out_X").get()[0] < .45f && v("B Out_X").get()[0] > .55f);
    assert(v("A Out_Y").get() == vector<float>{.5f} && v("B Out_Y").get() == vector<float>{.5f});
    float previousA = v("A Out_X").get()[0], previousB = v("B Out_X").get()[0];
    tick(); // Continues without input events.
    assert(v("A Out_X").get()[0] < previousA && v("B Out_X").get()[0] > previousB);

    n.parameter<void>("Reset").trigger(); tick();
    assert(v("A Out_X").get() == vector<float>{.45f} && v("B Out_X").get() == vector<float>{.55f});
    // Strength of one body only scales the push on the other.
    v("A Strength") = {0}; tick();
    assert(v("A Out_X").get()[0] < .45f && v("B Out_X").get() == vector<float>{.55f});
    v("A Strength") = {1}; v("B Repulsion Strength") = {0}; tick();
    assert(v("B Out_X").get() == vector<float>{.55f});
    v("B Repulsion Strength") = {40}; tick(); assert(v("B Out_X").get()[0] > .55f);

    // Radius widens the interaction: 0.4 apart is outside fear 0.15 until the disks reach.
    n.parameter<void>("Reset").trigger();
    v("A Body_X") = {.3f}; v("B Body_X") = {.7f}; tick(); tick();
    assert(v("A Out_X").get() == vector<float>{.3f} && v("B Out_X").get() == vector<float>{.7f});
    v("A Radius") = {.15f}; v("B Radius") = {.15f}; tick();
    assert(v("A Out_X").get()[0] < .3f && v("B Out_X").get()[0] > .7f);
    v("A Radius") = {0}; v("B Radius") = {0};

    // All four outputs are staged before any callback; each body notifies Y before X.
    n.parameter<void>("Reset").trigger();
    v("A Body_X") = {.45f}; v("B Body_X") = {.55f}; tick();
    vector<string> order;
    vector<float> seenBX;
    auto ay = v("A Out_Y").newListener([&](vector<float>&) { order.push_back("AY"); seenBX = v("B Out_X").get(); });
    auto ax = v("A Out_X").newListener([&](vector<float>&) { order.push_back("AX"); });
    auto by = v("B Out_Y").newListener([&](vector<float>&) { order.push_back("BY"); });
    auto bx = v("B Out_X").newListener([&](vector<float>&) { order.push_back("BX"); });
    tick();
    assert(order == vector<string>({"AY", "AX", "BY", "BX"}));
    assert(seenBX == v("B Out_X").get() && seenBX[0] > .55f);
    ay.reset(); ax.reset(); by.reset(); bx.reset();

    // Border: scalar margin clamps, feather pushes inward.
    n.parameter<void>("Reset").trigger();
    v("A Body_X") = {.02f}; v("B Body_X") = {.98f}; tick();
    assert(v("A Out_X").get() == vector<float>{.02f}); // Unclamped while the border is off.
    f("Border Margin") = .1f; tick();
    assert(std::abs(v("A Out_X").get()[0] - .1f) < 1e-6f && std::abs(v("B Out_X").get()[0] - .9f) < 1e-6f);
    f("Border Margin") = 0; f("Border Feather") = .2f;
    n.parameter<void>("Reset").trigger(); v("A Body_X") = {.1f}; v("B Body_X") = {.5f}; tick(); tick();
    assert(v("A Out_X").get()[0] > .1f && v("B Out_X").get() == vector<float>{.5f});
    f("Border Feather") = 0;

    // Presets and transport reset both bodies.
    ofJson json;
    n.presetRecallBeforeSettingParameters(json); n.presetHasLoaded(); tick();
    assert(v("A Out_X").get() == vector<float>{.1f});
    v("A Body_X") = {.45f}; v("B Body_X") = {.55f}; tick(); tick();
    assert(v("A Out_X").get()[0] < .45f);
    n.resetPhase(); tick(); assert(v("A Out_X").get() == vector<float>{.45f});

    // An invalid body clears its own output while the other keeps simulating.
    tick(); v("A Body_X") = {}; v("A Body_Y") = {}; tick();
    assert(v("A Out_X").get().empty() && v("A Out_Y").get().empty());
    assert(v("B Out_X").get().size() == 1 && v("B Out_X").get()[0] > .55f);
    v("A Body_X") = {.2f,.3f}; v("A Body_Y") = {.2f}; tick();
    assert(v("A Out_X").get().empty());
    v("A Body_Y") = {.2f,.3f}; tick();
    assert(v("A Out_X").get() == vector<float>({.2f,.3f}));

    // Use Oceanode's real scalar-to-vector connection with every vector control of both bodies.
    twoBodyRepulsion connected; connected.setup();
    auto c = [&](const string& key) -> ofParameter<vector<float>>& { return connected.parameter<vector<float>>(key); };
    c("A Body_X") = {.45f,.45f}; c("A Body_Y") = {.5f,.5f};
    c("B Body_X") = {.55f,.55f}; c("B Body_Y") = {.5f,.5f};
    for(const string body : {"A", "B"}) {
        for(const string name : {"Radius", "Strength", "FearDistance", "Repulsion Strength", "Return Strength", "Damping"}) {
            ofParameter<float> source;
            source.set("Scalar", 1.25f, 0.0f, 1000.0f);
            ofxOceanodeParameter<float> sourceWrapper; sourceWrapper.bindParameter(source);
            ofxOceanodeParameter<vector<float>> sinkWrapper;
            sinkWrapper.bindParameter(c(body + " " + name));
            ofxOceanodeConnection<float,vector<float>> connection(sourceWrapper, sinkWrapper, true);
            assert(c(body + " " + name).get() == vector<float>{1.25f});
            source = .75f;
            assert(c(body + " " + name).get() == vector<float>{.75f});
            connected.update(event); connected.update(event);
            for(const string out : {"A Out_X", "A Out_Y", "B Out_X", "B Out_Y"})
                assert(c(out).get()[0] == c(out).get()[1]);
        }
    }

    // Per-point vectors change the response live, keeping momentum.
    twoBodyRepulsion varying; varying.setup();
    auto p = [&](const string& key) -> ofParameter<vector<float>>& { return varying.parameter<vector<float>>(key); };
    p("A Body_X") = {.45f,.45f}; p("A Body_Y") = {.5f,.5f};
    p("B Body_X") = {.55f}; p("B Body_Y") = {.5f};
    p("A Repulsion Strength") = {0,40}; p("A Return Strength") = {0}; p("A Damping") = {0};
    p("B Return Strength") = {0}; p("B Damping") = {0};
    varying.update(event); varying.update(event);
    assert(p("A Out_X").get()[0] == .45f && p("A Out_X").get()[1] < .45f);
    p("A Repulsion Strength") = {40,0}; varying.update(event);
    assert(p("A Out_X").get()[0] < .45f);
    const vector<float> previousX = p("A Out_X").get();
    p("A Return Strength") = {0,8}; p("A Body_X") = {.1f,.1f}; varying.update(event);
    assert(p("A Out_X").get()[0] < previousX[0]); // Prior momentum retained.
    assert(p("A Out_X").get()[1] < previousX[1]); // Second point follows its new spring target.
    std::cout << "PASS: defaults, mutual pushes, strength, radius, paired output events, border, "
                 "reset/presets/transport, invalid bodies, real float connections and live vector controls.\n";
}
