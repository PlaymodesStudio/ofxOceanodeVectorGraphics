#include "../src/trimPathsPro.h"
#include <cassert>
#include <iostream>

int main() {
    trimPathsPro n;
    n.setup();
    ofEventArgs event;
    auto floats = [&](const string& key) -> ofParameter<vector<float>>& { return n.parameter<vector<float>>(key); };
    auto integer = [&](const string& key) -> ofParameter<int>& { return n.parameter<int>(key); };
    auto tick = [&]() { n.update(event); };
    floats("In.X") = {0,.25f,1,-1,0,1}; floats("In.Y") = {0,0,0,-1,1,1};
    tick();
    assert(floats("Out.X").get() == vector<float>({0,.25f,1,-1,0,1,-1}));
    int notifications = 0;
    auto listener = floats("Out.X").newListener([&](vector<float>&) { ++notifications; });
    floats("End") = {.5f,.75f}; floats("Start") = {0};
    assert(notifications == 0); // Changes coalesce rather than calculating intermediate states.
    tick(); assert(notifications == 1);
    assert(floats("Out.X").get() == vector<float>({0,.25f,.5f,-1,0,.75f,-1}));
    tick(); assert(notifications == 1);
    floats("In.Y") = {.2f,.2f,.2f,-1,.8f,.8f}; tick(); assert(notifications == 2);
    assert(floats("Out.Y").get().front() == .2f);
    integer("Scope") = 0; tick();
    assert(floats("Out.X").get() == vector<float>({0,.125f,-1,.25f,.625f,-1,0,.75f,-1}));

    // Observe the FIRST published output, before the X notification, and verify all new data is staged.
    bool observed = false;
    auto stateListener = n.parameter<bool>("Valid").newListener([&](bool&) {
        assert(floats("Out.X").get() == vector<float>({0,.5f,-1}));
        assert(floats("Out.Y").get() == vector<float>({.6f,.6f,-1}));
        assert(integer("Group Count").get() == 1);
        assert(integer("Edge Count").get() == 1);
        observed = true;
    });
    floats("In.X") = {0,1}; floats("In.Y") = {.6f,.6f}; floats("End") = {.5f};
    tick(); assert(observed);

    // Separate instance avoids retaining observation callbacks while testing other outputs.
    trimPathsPro grouped; grouped.setup();
    grouped.parameter<vector<float>>("In.X") = {0,1};
    grouped.parameter<vector<float>>("In.Y") = {0,0};
    grouped.parameter<int>("Scope") = 3;
    ofJson populated = {{"TrimPathsProGroups", {{0},{0}}}};
    grouped.presetRecallBeforeSettingParameters(populated);
    grouped.parameter<vector<float>>("Start") = {0,.75f};
    grouped.parameter<vector<float>>("End") = {.25f,1};
    grouped.update(event);
    assert(grouped.parameter<int>("Group Count").get() == 2);
    assert(grouped.parameter<vector<float>>("Out.X").get() == vector<float>({0,.25f,-1,.75f,1,-1}));
    ofJson saved; grouped.presetSave(saved); assert(saved == populated);
    ofJson empty = {{"TrimPathsProGroups", ofJson::array({ofJson::array(),ofJson::array()})}};
    grouped.presetRecallBeforeSettingParameters(empty); grouped.update(event);
    grouped.presetSave(saved); assert(saved == empty);
    assert(grouped.parameter<int>("Group Count").get() == 2); // Empty groups still occupy modulation slots.
    assert(grouped.parameter<vector<float>>("Out.X").get().empty());
    grouped.presetRecallBeforeSettingParameters(populated);
    grouped.parameter<int>("Num Groups") = 1;
    grouped.parameter<vector<float>>("Start") = {0}; grouped.parameter<vector<float>>("End") = {1};
    grouped.update(event);
    assert(grouped.parameter<vector<int>>("Source.Group").get() == vector<int>({0}));
    assert(grouped.parameter<int>("Group Count").get() == 1);
    ofJson malformed = {{"TrimPathsProGroups", ofJson::array({ofJson::array({0,0,-1,"bad",1.5,18446744073709551615ull}), "bad"})}};
    grouped.presetRecallBeforeSettingParameters(malformed); grouped.presetSave(saved);
    assert(saved["TrimPathsProGroups"] == ofJson::array({ofJson::array({0}),ofJson::array()}));
    grouped.parameter<int>("Num Groups") = -20; assert(grouped.parameter<int>("Num Groups").get() == 1);
    grouped.parameter<int>("Num Groups") = 1000; assert(grouped.parameter<int>("Num Groups").get() == 100);
    grouped.parameter<int>("Scope") = 1; grouped.update(event);
    assert(grouped.parameter<int>("Group Count").get() == 100); // Available outside Groups scope too.

    trimPathsPro feedback; feedback.setup();
    feedback.parameter<vector<float>>("In.X") = {0,1}; feedback.parameter<vector<float>>("In.Y") = {0,0};
    bool once = false;
    auto feedbackListener = feedback.parameter<vector<float>>("Out.X").newListener([&](vector<float>&) {
        if(!once) { once = true; feedback.parameter<vector<float>>("End") = {.5f}; }
    });
    feedback.update(event);
    assert(feedback.parameter<vector<float>>("Out.X").get() == vector<float>({0,1,-1}));
    feedback.update(event);
    assert(feedback.parameter<vector<float>>("Out.X").get() == vector<float>({0,.5f,-1}));
    feedback.parameter<vector<float>>("In.Y") = {0}; feedback.update(event);
    assert(!feedback.parameter<bool>("Valid").get());
    assert(feedback.parameter<vector<float>>("Out.X").get().empty());

    trimPathsPro shifted; shifted.setup();
    shifted.parameter<vector<float>>("In.X") = {0,1};
    shifted.parameter<vector<float>>("In.Y") = {0,0};
    shifted.parameter<vector<float>>("End") = {.25f};
    shifted.update(event);
    shifted.parameter<vector<float>>("Origin") = {.875f};
    bool observedWrappedOutputs = false;
    auto shiftedListener = shifted.parameter<vector<float>>("Out.X").newListener([&](vector<float>&) {
        assert(shifted.parameter<vector<float>>("WrappedSegment.X").get() == vector<float>({0,.125f,-1}));
        assert(shifted.parameter<vector<float>>("WrappedSegment.Y").get() == vector<float>({0,0,-1}));
        assert(shifted.parameter<vector<float>>("Completeness").get() == vector<float>({.25f}));
        observedWrappedOutputs = true;
    });
    shifted.update(event); // Origin alone triggers the update, without new X or Y.
    assert(observedWrappedOutputs);
    assert(shifted.parameter<vector<float>>("Out.X").get() == vector<float>({.875f,1,-1,0,.125f,-1}));
    shiftedListener.reset();
    shifted.parameter<vector<float>>("Origin") = {.5f}; shifted.update(event);
    assert(shifted.parameter<vector<float>>("Out.X").get() == vector<float>({.5f,.75f,-1}));
    assert(shifted.parameter<vector<float>>("WrappedSegment.X").get() == vector<float>({0,0,-1}));
    shifted.parameter<int>("Origin Map") = 4; shifted.update(event);
    assert(!shifted.parameter<bool>("Valid").get());
    assert(shifted.parameter<string>("Status").get().find("Origin") != string::npos);
    assert(shifted.parameter<vector<float>>("WrappedSegment.X").get() == vector<float>({0,0,-1}));

    // Exercise the real ImGui group editor without a window/GPU backend.
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(900,900); io.DeltaTime = 1.0f/60;
    unsigned char* pixels = nullptr; int width = 0, height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
    n.parameter<bool>("Show GUI") = true;
    for(int frame = 0; frame < 2; ++frame) {
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0,0)); ImGui::SetNextWindowSize(ImVec2(650,800));
        n.draw(event); ImGui::Render();
    }
    assert(ImGui::GetDrawData()->TotalVtxCount > 0);
    ImGui::DestroyContext();
    std::cout << "TrimPathsPro node: real OF events, batching, coherent outputs, feedback, presets and headless ImGui passed.\n";
}
