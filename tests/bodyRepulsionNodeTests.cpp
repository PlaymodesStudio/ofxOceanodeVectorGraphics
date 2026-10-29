#include "ofMain.h"
#include "ofxOceanodeConnection.h"
#include <cassert>
#include <iostream>

// Supply host frame duration without requiring a running window. OF parameters/events are real.
static float frameSeconds = 1.0f / 60.0f;
static float testFrameTime() { return frameSeconds; }
#define ofGetLastFrameTime testFrameTime
#include "../src/bodyRepulsion.h"
#undef ofGetLastFrameTime

#include "oceanodeHeadlessStubs.h"

int main() {
    bodyRepulsion n; n.setup();
    ofEventArgs event;
    auto v = [&](const string& key) -> ofParameter<vector<float>>& { return n.parameter<vector<float>>(key); };
    auto tick = [&]() { n.update(event); };
    v("Body_X") = {.5f}; v("Body_Y") = {.5f};
    v("Force_X") = {.4f}; v("Force_Y") = {.5f}; v("FearDistance") = {.2f};
    assert(v("Strength").get() == vector<float>{1});
    v("Return Strength") = {0}; v("Damping") = {0};
    tick(); assert(v("Out_X").get() == vector<float>{.5f}); // Initial reset at exact rest shape.
    tick(); assert(v("Out_X").get()[0] > .5f);
    float previous = v("Out_X").get()[0];
    tick(); assert(v("Out_X").get()[0] > previous); // Continues without input events.
    v("Force_X") = {}; v("Force_Y") = {};
    previous = v("Out_X").get()[0];
    tick(); assert(v("Out_X").get()[0] > previous);

    n.parameter<void>("Reset").trigger(); tick();
    assert(v("Out_X").get() == vector<float>{.5f});
    tick(); assert(v("Out_X").get() == vector<float>{.5f}); // Momentum cleared.
    v("Return Strength") = {8};
    int xNotifications = 0, yNotifications = 0;
    bool expectNewY = false;
    auto yl = v("Out_Y").newListener([&](vector<float>& y) {
        ++yNotifications;
        assert(v("Out_X").get() == vector<float>{.5f});
        assert(y == v("Out_Y").get());
        if(expectNewY) assert(y[0] > .5f);
    });
    auto xl = v("Out_X").newListener([&](vector<float>& x) {
        ++xNotifications;
        assert(x == v("Out_X").get());
        if(expectNewY) assert(v("Out_Y").get()[0] > .5f);
    });
    v("Body_Y") = {.8f}; expectNewY = true;
    tick(); assert(yNotifications == 1 && xNotifications == 1);
    assert(v("Out_Y").get()[0] > .5f && v("Out_Y").get()[0] < .8f);
    // Force_Y alone starts repulsion, with Body held still.
    expectNewY = false;
    n.parameter<void>("Reset").trigger(); tick();
    v("Force_X") = {.5f}; v("Force_Y") = {1.5f};
    tick(); assert(v("Out_Y").get()[0] == .8f);
    v("Force_Y") = {.7f}; tick(); assert(v("Out_Y").get()[0] > .8f);
    // Fear and strength controls are sampled even without coordinate changes.
    n.parameter<void>("Reset").trigger(); tick();
    v("FearDistance") = {0}; tick(); assert(v("Out_Y").get()[0] == .8f);
    v("FearDistance") = {.2f}; v("Repulsion Strength") = {0};
    tick(); assert(v("Out_Y").get()[0] == .8f);
    v("Repulsion Strength") = {40}; tick(); assert(v("Out_Y").get()[0] > .8f);
    const float fullStrengthY = v("Out_Y").get()[0];
    n.parameter<void>("Reset").trigger(); tick();
    v("Strength") = {0}; tick(); assert(v("Out_Y").get()[0] == .8f);
    v("Strength") = {.5f}; tick();
    assert(v("Out_Y").get()[0] > .8f && v("Out_Y").get()[0] < fullStrengthY);
    const float halfStrengthY = v("Out_Y").get()[0];
    v("Strength") = {0}; tick(); assert(v("Out_Y").get()[0] > halfStrengthY);
    v("Strength") = {1};
    ofJson json;
    n.presetRecallBeforeSettingParameters(json); n.presetHasLoaded(); tick();
    assert(v("Out_Y").get() == vector<float>{.8f});
    tick(); assert(v("Out_Y").get()[0] > .8f);
    n.resetPhase(); tick(); assert(v("Out_Y").get() == vector<float>{.8f});
    yl.reset(); xl.reset();

    v("Body_X") = {}; v("Body_Y") = {}; tick();
    assert(v("Out_X").get().empty() && v("Out_Y").get().empty());
    v("Body_X") = {.2f,.3f}; v("Body_Y") = {.2f}; tick();
    assert(v("Out_X").get().empty());
    v("Body_Y") = {.2f,.3f}; v("Force_X") = {}; v("Force_Y") = {};
    tick(); assert(v("Out_X").get() == vector<float>({.2f,.3f}));

    // Stage both outputs before callbacks. Feedback/reset raised there applies next update.
    bodyRepulsion feedback; feedback.setup();
    feedback.parameter<vector<float>>("Force_X") = {.4f};
    feedback.parameter<vector<float>>("Force_Y") = {.4f};
    feedback.update(event);
    bool once = false;
    vector<float> publishedX, publishedY;
    auto observer = feedback.parameter<vector<float>>("Out_Y").newListener([&](vector<float>& y) {
        publishedY = y;
        publishedX = feedback.parameter<vector<float>>("Out_X").get();
        assert(publishedX[0] > .5f && publishedY[0] > .5f);
        if(!once) {
            once = true;
            feedback.parameter<vector<float>>("Body_X") = {.9f};
            feedback.parameter<void>("Reset").trigger();
        }
    });
    feedback.update(event);
    assert(once);
    assert(feedback.parameter<vector<float>>("Out_X").get() == publishedX);
    assert(feedback.parameter<vector<float>>("Out_Y").get() == publishedY);
    observer.reset();
    feedback.update(event);
    assert(feedback.parameter<vector<float>>("Out_X").get() == vector<float>{.9f});
    assert(feedback.parameter<vector<float>>("Out_Y").get() == vector<float>{.5f});

    // Use Oceanode's real scalar-to-vector connection with each vector control.
    bodyRepulsion connected; connected.setup();
    connected.parameter<vector<float>>("Body_X") = {.5f,.5f};
    connected.parameter<vector<float>>("Body_Y") = {.5f,.5f};
    connected.parameter<vector<float>>("Force_X") = {.4f};
    connected.parameter<vector<float>>("Force_Y") = {.5f};
    for(const string name : {"FearDistance", "Strength", "Repulsion Strength", "Return Strength", "Damping"}) {
        ofParameter<float> source;
        source.set("Scalar", 1.25f, 0.0f, 1000.0f);
        ofxOceanodeParameter<float> sourceWrapper; sourceWrapper.bindParameter(source);
        ofxOceanodeParameter<vector<float>> sinkWrapper;
        sinkWrapper.bindParameter(connected.parameter<vector<float>>(name));
        ofxOceanodeConnection<float,vector<float>> connection(sourceWrapper, sinkWrapper, true);
        assert(connected.parameter<vector<float>>(name).get() == vector<float>{1.25f});
        source = .75f;
        assert(connected.parameter<vector<float>>(name).get() == vector<float>{.75f});
        connected.update(event); connected.update(event);
        assert(connected.parameter<vector<float>>("Out_X").get()[0] ==
               connected.parameter<vector<float>>("Out_X").get()[1]);
        assert(connected.parameter<vector<float>>("Out_Y").get()[0] ==
               connected.parameter<vector<float>>("Out_Y").get()[1]);
    }

    bodyRepulsion varying; varying.setup();
    auto bodyParam = [&](const string& name) -> ofParameter<vector<float>>& { return varying.parameter<vector<float>>(name); };
    bodyParam("Body_X") = {.5f,.5f}; bodyParam("Body_Y") = {.5f,.5f};
    bodyParam("Force_X") = {.4f}; bodyParam("Force_Y") = {.5f}; bodyParam("FearDistance") = {.2f};
    bodyParam("Repulsion Strength") = {0,40}; bodyParam("Return Strength") = {0}; bodyParam("Damping") = {0};
    varying.update(event); varying.update(event);
    assert(bodyParam("Out_X").get()[0] == .5f && bodyParam("Out_X").get()[1] > .5f);
    bodyParam("Repulsion Strength") = {40,0}; varying.update(event);
    assert(bodyParam("Out_X").get()[0] > .5f); // Changing vector alone changes response.
    bodyParam("Force_X") = {}; bodyParam("Force_Y") = {};
    bodyParam("Return Strength") = {0,8}; bodyParam("Damping") = {0,50};
    bodyParam("Body_X") = {.8f,.8f};
    const vector<float> previousX = bodyParam("Out_X").get();
    varying.update(event);
    assert(bodyParam("Out_X").get()[0] > previousX[0]); // Prior momentum retained.
    assert(bodyParam("Out_X").get()[1] > previousX[1]); // Second point follows its new spring target.
    std::cout << "PASS: live vector controls, real float connections, uninterrupted simulation, paired output events, "
                 "reset/presets/transport, empty input and deferred feedback.\n";
}
