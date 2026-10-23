#include "ofMain.h"
#include "ofxOceanodeConnection.h"
static float frameSeconds = 1.f / 60.f;
static float testFrameTime() { return frameSeconds; }
#define ofGetLastFrameTime testFrameTime
#include "../src/waveIt.h"
#undef ofGetLastFrameTime
#include <cassert>
#include <iostream>
void ofxOceanodeAbstractParameter::removeAllConnections() {
    assert(!hasInConnection() && !hasOutConnections() && !isScoped());
}
int main() {
    waveIt node; node.setup(); ofEventArgs event;
    auto vec = [&](const string& name)->ofParameter<vector<float>>& { return node.parameter<vector<float>>(name); };
    auto tick = [&]() { node.update(event); };
    vec("In.X")={0,.5f,1}; vec("In.Y")={0,0,0};
    vec("Amount")={.1f};
    tick();
    assert(vec("Out.X").get()==vec("In.X").get());
    int count=0;
    auto listener=vec("Out.X").newListener([&](vector<float>&) { ++count; });
    vec("Wave In")={1};
    assert(count==0); tick(); assert(count==1);
    for(float v:vec("Out.Y").get()) assert(std::abs(v-.1f)<1e-6);
    tick(); assert(count==1);
    vec("Amount")={.2f}; tick(); assert(count==2);
    vec("In.Y")={.2f,.2f,.2f}; tick(); assert(count==3);
    vec("Wave In").setWithoutEventNotifications({.5f});
    node.presetHasLoaded(); tick(); assert(vec("Out.Y").get()==vec("In.Y").get());
    bool observed=false;
    auto pair=vec("Out.Y").newListener([&](vector<float>&) {
        assert(vec("Out.X").get()==vector<float>({.1f,.6f,.9f}));
        assert(vec("Out.Y").get()==vector<float>({.3f,.3f,.3f})); observed=true;
    });
    vec("In.X")={.1f,.6f,.9f}; vec("In.Y")={.3f,.3f,.3f}; tick(); assert(observed); pair.reset();
    bool once=false;
    auto feedback=vec("Out.X").newListener([&](vector<float>&) {
        if(!once) { once=true; vec("Wave In")={0}; }
    });
    vec("Wave In")={1}; tick(); assert(once);
    assert(std::abs(vec("Out.Y").get()[0]-.5f)<1e-6);
    tick(); assert(std::abs(vec("Out.Y").get()[0]-.1f)<1e-6);
    vec("In.Y")={0}; tick(); assert(vec("Out.X").get().empty() && vec("Out.Y").get().empty());
    waveIt spring; spring.setup();
    auto sv=[&](const string& name)->ofParameter<vector<float>>& { return spring.parameter<vector<float>>(name); };
    sv("In.X")={0,.25f,1}; sv("In.Y")={0,0,0};
    sv("Wave In")={1}; sv("Amount")={.2f}; sv("Stiffness")={0,100};
    spring.parameter<int>("Response")=1;
    spring.update(event);
    auto sy=sv("Out.Y").get();
    assert(sy[0]==0 && sy[1]>0 && sy[2]>sy[1]);
    spring.update(event); assert(sv("Out.Y").get()[2]>sy[2]);
    sv("Stiffness")={0}; sy=sv("Out.Y").get(); spring.update(event);
    assert(sv("Out.Y").get()==sy);
    sv("Stiffness")={100}; spring.update(event); assert(sv("Out.Y").get()[0]>0);
    frameSeconds=0;
    spring.resetPhase(); spring.update(event); assert(sv("Out.Y").get()==vector<float>({0,0,0}));
    frameSeconds=1.f/60; spring.update(event); assert(sv("Out.Y").get()[0]>0);
    frameSeconds=0; spring.presetHasLoaded(); spring.update(event);
    assert(sv("Out.Y").get()==vector<float>({0,0,0}));
    spring.parameter<int>("Response")=0; sv("Amount")={0,.2f}; spring.update(event);
    assert(std::abs(sv("Out.Y").get()[1]-.05f)<1e-6);
    // Float connections still broadcast into the new vector controls.
    ofParameter<float> scalar; scalar.set("Scalar",.1f,-1,1);
    ofxOceanodeParameter<float> source; source.bindParameter(scalar);
    ofxOceanodeParameter<vector<float>> sink; sink.bindParameter(sv("Amount"));
    {
        ofxOceanodeConnection<float,vector<float>> connection(source,sink,true);
        scalar=.3f;
        assert(sv("Amount").get()==vector<float>({.3f}));
        spring.update(event);
        for(float v:sv("Out.Y").get()) assert(std::abs(v-.3f)<1e-6);
    }
    waveIt moving; moving.setup();
    auto mv=[&](const string& name)->ofParameter<vector<float>>& { return moving.parameter<vector<float>>(name); };
    mv("In.X")={0,.25f,1}; mv("In.Y")={0,0,0};
    moving.parameter<int>("Response")=1;
    assert(mv("Motion Influence").get()==vector<float>({0}));
    mv("Amount")={0}; mv("Motion Influence")={0,1};
    frameSeconds=1.f/60;
    moving.update(event);
    mv("In.Y")={.1f,.1f,.1f}; moving.update(event);
    auto movingY=mv("Out.Y").get();
    assert(std::abs(movingY[0]-.1f)<1e-6 && movingY[2]<movingY[1]);
    moving.update(event); assert(mv("Out.Y").get()[2]>movingY[2]); // stationary input rebounds
    moving.presetHasLoaded(); moving.update(event);
    for(float value:mv("Out.Y").get()) assert(std::abs(value-.1f)<1e-6);
    // A real scalar connection broadcasts to Motion Influence too.
    ofParameter<float> influence; influence.set("Influence",.4f,0,1);
    ofxOceanodeParameter<float> influenceSource; influenceSource.bindParameter(influence);
    ofxOceanodeParameter<vector<float>> influenceSink; influenceSink.bindParameter(mv("Motion Influence"));
    {
        ofxOceanodeConnection<float,vector<float>> connection(influenceSource,influenceSink,true);
        assert(mv("Motion Influence").get()==vector<float>({.4f}));
        influence=1; moving.update(event);
        mv("In.Y")={.2f,.2f,.2f}; moving.update(event);
        assert(mv("Out.Y").get()[0]<.2f);
        influence=0; moving.resetPhase(); moving.update(event);
        mv("In.Y")={.3f,.3f,.3f}; moving.update(event);
        assert(mv("Out.Y").get()==mv("In.Y").get());
    }
    std::cout << "Wave It Motion Influence node: vector mapping, Y-only motion, idle rebound, default bypass, preset/reset and scalar connections passed.\n";
    std::cout << "Wave It spring node: live stiffness, continuous motion, reset/preset, vector Amount and real scalar-to-vector connections passed.\n";
    std::cout << "Wave It node: real OF events, Wave/Amount/Y-only updates, batching, paired output, presets and feedback passed.\n";
}
