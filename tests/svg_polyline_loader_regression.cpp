#include "svgPolylineLoader.h"
#include <cassert>
#include <fstream>
#include <iostream>

static ofFileDialogResult nextDialog;
static string dialogDefault;
ofFileDialogResult ofSystemLoadDialog(string, bool folders, string defaultPath) {
    assert(!folders);
    dialogDefault = defaultPath;
    return nextDialog;
}
static void writeFile(const string &path, const string &content) {
    std::ofstream(path) << content;
}
static void near(float a, float b) { assert(std::abs(a - b) < 0.0001f); }
int main(int argc, char **argv) {
    assert(argc == 2);
    ofSetDataPathRoot(string(argv[1]) + "/");
    svgPolylineLoader node;
    node.setup();
    auto &p = node.getParameterGroup();
    auto &file = p.get<string>("File");
    auto &x = p.get<vector<float>>("Vertex_X");
    auto &y = p.get<vector<float>>("Vertex_Y");
    auto &res = p.get<int>("Curve Resolution");
    auto &normalize = p.get<bool>("Normalize");
    auto &rotate = p.get<void>("Rotate90º");
    auto &flipX = p.get<bool>("FlipX");
    auto &flipY = p.get<bool>("FlipY");
    auto &scale = p.get<float>("Scale");
    auto &angle = p.get<float>("Rotate");
    auto &moveX = p.get<float>("MoveX");
    auto &moveY = p.get<float>("MoveY");
    assert(scale == 1 && angle == 0 && moveX == 0 && moveY == 0);
    near(scale.getMin(), 0); near(scale.getMax(), 1);
    for (auto* control : {&angle, &moveX, &moveY}) {
        near(control->getMin(), -1); near(control->getMax(), 1);
    }
    auto &turns = node.getInspectorParameterGroup().get<int>("Quarter Turns");
    rotate.trigger(); // Safe before any SVG has loaded.
    assert(turns == 0 && x->empty() && y->empty());
    auto &button = p.get<std::function<void()>>("Load SVG");
    vector<char> order;
    ofEventListeners listeners;
    listeners.push(y.newListener([&](vector<float> &) { order.push_back('Y'); }));
    listeners.push(x.newListener([&](vector<float> &) {
        order.push_back('X');
        assert(x.get().size() == y.get().size());
    }));
    const string path = string(argv[1]) + "/shapes.svg";
    writeFile(path, R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="200" height="100">
      <path fill="none" stroke="black" d="M20 10 L40 30 M60 20 L80 40"/>
      <path fill="red" d="M100 10 L140 10 L140 50 Z"/>
    </svg>)svg");
    file = "shapes.svg"; // Data-relative file, including preset-compatible path.
    assert(order == vector<char>({'Y', 'X'}));
    assert(x->size() == 11 && y->size() == 11);
    // Parser/source order; separate moveTo contours and a closed fill-only path.
    near(x->at(0), .1f); near(y->at(0), .1f);
    near(x->at(3), .3f); near(y->at(3), .2f);
    for (int i : {2, 5, 10}) assert(x->at(i) == -1 && y->at(i) == -1);
    assert(x->at(6) == x->at(9) && y->at(6) == y->at(9));
    normalize = false;
    near(x->at(0), 20); near(y->at(0), 10);
    normalize = true;

    const auto originalX = x.get(), originalY = y.get();
    order.clear();
    rotate.trigger();
    assert(order == vector<char>({'Y', 'X'}));
    near(x->front(), .9f); near(y->front(), .1f);
    for (int i : {2, 5, 10}) assert(x->at(i) == -1 && y->at(i) == -1);
    assert(x->at(6) == x->at(9) && y->at(6) == y->at(9));
    rotate.trigger(); near(x->front(), .9f); near(y->front(), .9f);
    rotate.trigger(); near(x->front(), .1f); near(y->front(), .9f);
    rotate.trigger();
    assert(x.get() == originalX && y.get() == originalY && turns == 0);

    flipX = true; near(x->front(), .9f); near(y->front(), .1f);
    flipY = true; near(x->front(), .9f); near(y->front(), .9f);
    flipX = false; near(x->front(), .1f); near(y->front(), .9f);
    flipY = false;
    assert(x.get() == originalX && y.get() == originalY);
    rotate.trigger();
    flipX = true; flipY = true;
    near(x->front(), .1f); near(y->front(), .9f);
    // Transform source geometry, never the separator entries.
    for (int i : {2, 5, 10}) assert(x->at(i) == -1 && y->at(i) == -1);
    res = 48;
    near(x->front(), .1f); near(y->front(), .9f);
    ofJson transformPreset;
    ofSerialize(transformPreset, turns);
    turns = 0;
    ofDeserialize(transformPreset, turns);
    assert(turns == 1);
    node.presetRecallAfterSettingParameters(transformPreset);
    near(x->front(), .1f); near(y->front(), .9f);
    flipX = false; flipY = false;
    normalize = false;
    near(x->front(), 140); near(y->front(), -30);
    flipX = true; flipY = true;
    near(x->front(), 60); near(y->front(), 130);
    turns = 0; flipX = false; flipY = false; normalize = true;
    assert(x.get() == originalX && y.get() == originalY);

    // Continuous controls: neutral defaults, signed angle/translation and size.
    scale = .5f;
    near(x->front(), .3f); near(y->front(), .3f);
    scale = 0;
    for (size_t i = 0; i < x->size(); ++i) {
        if (originalX[i] != -1) { near(x->at(i), .5f); near(y->at(i), .5f); }
    }
    scale = 1;
    angle = 1;
    near(x->at(1), .7f); near(y->at(1), .2f);
    angle = -1;
    near(x->at(1), .3f); near(y->at(1), .8f);
    angle = .5f;
    near(x->front(), .5f); near(y->front(), .5f - .4f * std::sqrt(2.0f));
    angle = 0;
    order.clear();
    moveX = .25f;
    assert(order == vector<char>({'Y', 'X'}));
    moveY = -.5f;
    near(x->front(), .35f); near(y->front(), -.4f);
    moveX = -1; moveY = 1;
    near(x->at(1), -.8f); near(y->at(1), 1.3f);
    moveX = 0; moveY = 0;
    assert(x.get() == originalX && y.get() == originalY);

    // Quarter turns + continuous rotation, then flip/scale, then translation.
    rotate.trigger(); angle = -.5f; flipX = true;
    scale = .5f; moveX = .2f; moveY = -.1f;
    const float combinedY = .5f - .2f * std::sqrt(2.0f) - .1f;
    near(x->front(), .7f); near(y->front(), combinedY);
    for (int i : {2, 5, 10}) assert(x->at(i) == -1 && y->at(i) == -1);
    assert(x->at(6) == x->at(9) && y->at(6) == y->at(9));
    res = 24;
    near(x->front(), .7f); near(y->front(), combinedY);
    // Save/recall uses real OF serialization for the new controls.
    ofJson continuousPreset;
    for (auto* control : {&scale, &angle, &moveX, &moveY}) ofSerialize(continuousPreset, *control);
    scale = 1; angle = 0; moveX = 0; moveY = 0;
    for (auto* control : {&scale, &angle, &moveX, &moveY}) ofDeserialize(continuousPreset, *control);
    node.presetRecallAfterSettingParameters(continuousPreset);
    near(x->front(), .7f); near(y->front(), combinedY);
    turns = 0; angle = 0; flipX = false;
    moveX = .25f; moveY = -.5f;
    normalize = false;
    near(x->front(), 110); near(y->front(), -20);
    angle = 1;
    near(x->front(), 170); near(y->front(), -40);
    scale = 1; angle = 0; moveX = 0; moveY = 0; normalize = true;
    assert(x.get() == originalX && y.get() == originalY);

    const string curves = string(argv[1]) + "/curves.svg";
    writeFile(curves, R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="10,20,200,100">
      <path fill="none" stroke="black" d="M10 20 C60 20 60 120 110 120 Q160 120 210 20"/>
    </svg>)svg");
    res = 4;
    file = curves;
    near(x->front(), 0); near(y->front(), 0);
    near(x->at(x->size()-2), 1); near(y->at(y->size()-2), 0);
    const auto lowCount = x->size();
    res = 32;
    assert(x->size() > lowCount && x->size() == y->size());
    assert(x->back() == -1 && y->back() == -1);

    const string primitives = string(argv[1]) + "/primitives.svg";
    writeFile(primitives, R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100">
      <g transform="translate(10 20)"><polyline fill="none" stroke="black" points="0,0 10,0 10,10"/></g>
      <circle cx="50" cy="50" r="10"/>
      <path fill="none" stroke="black" d="M70 70 A10 10 0 0 1 90 70"/>
    </svg>)svg");
    file = primitives;
    near(x->front(), .1f); near(y->front(), .2f);
    assert(std::count(x->begin(), x->end(), -1) == 3);
    assert(x->size() == y->size());

    // Cancel preserves current data; selecting the same filename reloads it.
    const auto saved = x.get();
    nextDialog.bSuccess = false;
    ImGui::pressLoad = true; button.get()();
    assert(x.get() == saved && file.get() == primitives);
    assert(dialogDefault == ofToDataPath("", true));
    nextDialog.bSuccess = true; nextDialog.filePath = primitives;
    writeFile(primitives, R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100"><line x1="10" y1="20" x2="30" y2="40" stroke="black"/></svg>)svg");
    ImGui::pressLoad = true; button.get()();
    assert(x->size() == 3); near(x->at(1), .3f);
    nextDialog.filePath = string(argv[1]) + "/wrong.txt";
    ImGui::pressLoad = true; button.get()();
    assert(file.get() == primitives && x->size() == 3);
    writeFile(primitives, R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100"><line x1="10" y1="20" x2="80" y2="90" stroke="black"/></svg>)svg");
    ofJson preset;
    node.presetRecallAfterSettingParameters(preset);
    near(x->at(1), .8f); near(y->at(1), .9f);

    file = "missing.svg";
    assert(x->empty() && y->empty());
    writeFile(path, "<broken>"); file = path;
    assert(x->empty() && y->empty());
    writeFile(path, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" height=\"100\"/>");
    node.presetRecallAfterSettingParameters(preset);
    assert(x->empty() && y->empty());
    file = "";
    assert(x->empty() && y->empty());
    std::cout << "SVG loader regression passed: geometry, normalization, sampling, rotation/flips/scale/move, Y/X order, dialog, reload and errors.\n";
}
