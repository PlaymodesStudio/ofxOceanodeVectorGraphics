#pragma once

#include "ofxOceanodeNodeModel.h"
#include "waveItCore.h"
#include <cfloat>

class waveIt : public ofxOceanodeNodeModel {
public:
    waveIt() : ofxOceanodeNodeModel("Wave It") {}

    void setup() override {
        description = "Deform XY paths along their local perpendicular. Wave In is normalized: "
            "0.5 is neutral; 0/1 displace by -/+ Amount. The full wave maps by distance "
            "along each separate shape. Paired -1 separators and vertex counts are preserved. "
            "Repeated endpoints identify closed loops, with periodic wave interpolation and "
            "outward positive displacement. Open paths use the left normal of their direction "
            "and map first/last wave samples to their endpoints. Outputs are unclamped. "
            "Amount, Stiffness and Motion Influence vectors map by distance; single values broadcast. "
            "Direct follows immediately; Spring adds inertia with fixed damping. Stiffness "
            "sets response speed; zero holds the current offset. Empty Wave In targets zero "
            "deformation. Motion Influence (0-1) makes input motion excite springs along the normal, "
            "even with neutral Wave In. It is inactive in Direct mode or at zero Stiffness. "
            "Invalid XY pairs clear output. Topology changes reset spring motion.";
        addParameter(xIn.set("In.X", {0.5f}, {-FLT_MAX}, {FLT_MAX}));
        addParameter(yIn.set("In.Y", {0.5f}, {-FLT_MAX}, {FLT_MAX}));
        addParameter(waveIn.set("Wave In", {0.5f}, {0.f}, {1.f}));
        addParameter(amount.set("Amount", {0.05f}, {-1.f}, {1.f}));
        addParameterDropdown(response, "Response", 0, {"Direct", "Spring"});
        addParameter(stiffness.set("Stiffness", {40.f}, {0.f}, {1000.f}));
        addParameter(motionInfluence.set("Motion Influence", {0.f}, {0.f}, {1.f}));
        addOutputParameter(xOut.set("Out.X", {}, {-FLT_MAX}, {FLT_MAX}));
        addOutputParameter(yOut.set("Out.Y", {}, {-FLT_MAX}, {FLT_MAX}));
        for(auto* p : {&xIn, &yIn, &waveIn, &amount, &stiffness, &motionInfluence})
            listeners.push(p->newListener([this](std::vector<float>&) { dirty = true; }));
        listeners.push(response.newListener([this](int&) { dirty = true; }));
        dirty = true;
    }

    void update(ofEventArgs&) override {
        if(!dirty && response.get() != 1) return;
        dirty = false;
        const auto result = simulation.update(xIn.get(), yIn.get(), waveIn.get(), amount.get(),
                                              stiffness.get(), response.get() == 1, ofGetLastFrameTime(),
                                              motionInfluence.get());
        const bool changed = result.x != xOut.get() || result.y != yOut.get();
        // Stage the pair before either callback; notify X last for X-driven nodes.
        xOut.setWithoutEventNotifications(result.x);
        yOut.setWithoutEventNotifications(result.y);
        if(changed) {
            yOut.set(result.y);
            xOut.set(result.x);
        }
    }

    void presetRecallBeforeSettingParameters(ofJson&) override { simulation.reset(); dirty = true; }
    void presetHasLoaded() override { simulation.reset(); dirty = true; }
    void resetPhase() override { simulation.reset(); dirty = true; }

private:
    ofParameter<std::vector<float>> xIn, yIn, waveIn, amount, stiffness, motionInfluence, xOut, yOut;
    ofParameter<int> response;
    waveItCore::Simulation simulation;
    ofEventListeners listeners;
    bool dirty = true;
};
