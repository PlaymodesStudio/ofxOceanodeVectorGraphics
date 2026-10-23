#pragma once

#include "ofxOceanodeNodeModel.h"
#include "waveProCore.h"
#include <cfloat>

class wavePro : public ofxOceanodeNodeModel {
public:
    wavePro() : ofxOceanodeNodeModel("Wave Pro") {}

    void setup() override {
        description = "Ripples XY contours along their normals. Wave In (0-1, 0.5 neutral) is "
            "stretched from start to end of every separate shape. Wave Offset and Wave Repeat "
            "shift and tile the wave, one value per shape. Distribute By Length shares the Wave Cycles "
            "contained in Wave In between shapes in whole cycles, in proportion to their "
            "perimeters (at least one each when possible), so all blobs get a similar "
            "wavelength; shares change only when clearly unfair, to avoid flicker. Anchor sets where closed shapes start "
            "reading the wave, always running clockwise: Tracked (default) follows the body from "
            "frame to frame, so the wave stays put when the input's start point or direction "
            "changes (e.g. MetaballsV3) and never hops as the body bends or turns; Stable uses "
            "the outline's right side (+X from the centre) every frame; Path Start uses the "
            "input order. Resample rebuilds every shape with N evenly spaced points (0 = keep "
            "the input points); this keeps the wave steady on contours whose points slide, like "
            "MetaballsV3, and keeps Spring state when their point count changes. Amount, Normal Smooth, Stiffness, "
            "Damping, Propagation and Motion Influence are vectors resampled along each "
            "perimeter; single values broadcast. Spring turns the contour into an elastic "
            "membrane that starts at rest on the wave: Stiffness pulls toward the wave (0 = "
            "plastic, keeps its dents), Damping sets ring-out (0 rings forever, 1 no overshoot), "
            "Propagation makes disturbances travel along the contour (perimeters per second) "
            "without flattening the wave, Motion Influence makes it react to the body's "
            "acceleration (starts, stops, turns, spin) like jelly. Bypass passes the input through unchanged and resets the membrane.";
        addParameter(bypass.set("Bypass", false));
        addParameter(xIn.set("In.X", {0.5f}, {-FLT_MAX}, {FLT_MAX}));
        addParameter(yIn.set("In.Y", {0.5f}, {-FLT_MAX}, {FLT_MAX}));
        addParameter(wave.set("Wave In", {0.5f}, {0.f}, {1.f}));
        addParameter(waveOffset.set("Wave Offset", {0.f}, {-1.f}, {1.f}));
        addParameter(waveRepeat.set("Wave Repeat", {1.f}, {0.f}, {32.f}));
        addParameterDropdown(distribute, "Distribute", 0, {"Per Shape", "By Length"});
        addParameter(waveCycles.set("Wave Cycles", 1, 1, 1024));
        addParameterDropdown(anchor, "Anchor", 2, {"Path Start", "Stable", "Tracked"});
        addParameter(resample.set("Resample", 512, 0, 4096));
        addParameter(amount.set("Amount", {0.05f}, {-1.f}, {1.f}));
        addParameter(smooth.set("Normal Smooth", {0.05f}, {0.f}, {1.f}));
        addParameterDropdown(response, "Response", 0, {"Direct", "Spring"});
        addParameter(stiffness.set("Stiffness", {40.f}, {0.f}, {1000.f}));
        addParameter(damping.set("Damping", {0.3f}, {0.f}, {4.f}));
        addParameter(propagation.set("Propagation", {0.f}, {0.f}, {20.f}));
        addParameter(motion.set("Motion Influence", {0.f}, {0.f}, {1.f}));
        addOutputParameter(xOut.set("Out.X", {}, {-FLT_MAX}, {FLT_MAX}));
        addOutputParameter(yOut.set("Out.Y", {}, {-FLT_MAX}, {FLT_MAX}));
        for(auto* p : {&xIn, &yIn, &wave, &waveOffset, &waveRepeat, &amount, &smooth,
                       &stiffness, &damping, &propagation, &motion})
            listeners.push(p->newListener([this](std::vector<float>&) { dirty = true; }));
        listeners.push(response.newListener([this](int&) { dirty = true; }));
        listeners.push(anchor.newListener([this](int&) { dirty = true; }));
        listeners.push(distribute.newListener([this](int&) { dirty = true; }));
        listeners.push(waveCycles.newListener([this](int&) { dirty = true; }));
        listeners.push(resample.newListener([this](int&) { dirty = true; }));
        listeners.push(bypass.newListener([this](bool&) { simulation.reset(); dirty = true; }));
    }

    void update(ofEventArgs&) override {
        const bool spring = response.get() == 1;
        if(!dirty && (!spring || bypass)) return;   // Spring keeps simulating while inputs are still
        dirty = false;
        if(bypass) { publish({xIn.get(), yIn.get()}); return; }
        const waveProCore::Controls controls{wave.get(), waveOffset.get(), waveRepeat.get(),
            amount.get(), smooth.get(), stiffness.get(), damping.get(), propagation.get(), motion.get(),
            anchor.get(), distribute.get(), waveCycles.get(), resample.get()};
        publish(simulation.update(xIn.get(), yIn.get(), controls, spring, ofGetLastFrameTime()));
    }

    void presetRecallBeforeSettingParameters(ofJson&) override { simulation.reset(); dirty = true; }
    void presetHasLoaded() override { simulation.reset(); dirty = true; }
    void resetPhase() override { simulation.reset(); dirty = true; }

private:
    void publish(const waveProCore::Result& result) {
        if(result.x == xOut.get() && result.y == yOut.get()) return;
        // Stage both before notifying, so listeners on X never see a stale Y.
        yOut.setWithoutEventNotifications(result.y);
        xOut.setWithoutEventNotifications(result.x);
        yOut.set(result.y);
        xOut.set(result.x);
    }

    ofParameter<std::vector<float>> xIn, yIn, wave, waveOffset, waveRepeat, amount, smooth;
    ofParameter<std::vector<float>> stiffness, damping, propagation, motion, xOut, yOut;
    ofParameter<int> response, anchor, distribute, waveCycles, resample;
    ofParameter<bool> bypass;
    waveProCore::Simulation simulation;
    ofEventListeners listeners;
    bool dirty = true;
};
