#pragma once

#include "ofxOceanodeNodeModel.h"
#include "bodyRepulsionCore.h"
#include <cfloat>

class bodyRepulsion : public ofxOceanodeNodeModel {
public:
    bodyRepulsion() : ofxOceanodeNodeModel("Body Repulsion") {}

    void setup() override {
        description = "Inertial point repulsion. Body coordinates are moving spring rest positions; "
            "Force points repel simulated positions inside each point's FearDistance. "
            "One fear value broadcasts; shorter vectors repeat their last value, excluding -1 separators. "
            "Strength multiplies each Force point's repulsion, default 1; scalar values broadcast, "
            "shorter vectors repeat their last value, excluding Force separators. Zero disables that repeller. "
            "Repulsion Strength, Return Strength and Damping map to Body points; one value broadcasts, "
            "shorter vectors repeat their last value, excluding Body separators. "
            "Return Strength 0 allows free drifting; Damping slows momentum. Reset restores the current body. "
            "Inputs are sampled each update; point count or separator layout changes reset the simulation. "
            "Output is unclamped. Empty or invalid Body pairs clear the output; invalid Force pairs are ignored.";
        addParameter(bodyX.set("Body_X", {0.5f}, {-FLT_MAX}, {FLT_MAX}));
        addParameter(bodyY.set("Body_Y", {0.5f}, {-FLT_MAX}, {FLT_MAX}));
        addParameter(forceX.set("Force_X", {}, {-FLT_MAX}, {FLT_MAX}));
        addParameter(forceY.set("Force_Y", {}, {-FLT_MAX}, {FLT_MAX}));
        addParameter(strength.set("Strength", {1.0f}, {0.0f}, {1.0f}));
        addParameter(fearDistance.set("FearDistance", {0.15f}, {0.0f}, {1.0f}));
        addParameter(repulsion.set("Repulsion Strength", {40.0f}, {0.0f}, {1000.0f}));
        addParameter(returnStrength.set("Return Strength", {8.0f}, {0.0f}, {1000.0f}));
        addParameter(damping.set("Damping", {3.0f}, {0.0f}, {100.0f}));
        addParameter(reset.set("Reset"));
        addOutputParameter(outX.set("Out_X", {}, {-FLT_MAX}, {FLT_MAX}));
        addOutputParameter(outY.set("Out_Y", {}, {-FLT_MAX}, {FLT_MAX}));
        resetListener = reset.newListener([this]() { resetPending = true; });
    }

    void update(ofEventArgs&) override {
        const bool doReset = resetPending;
        resetPending = false;
        bodyRepulsionCore::Settings settings;
        settings.repulsion = repulsion.get();
        settings.returnStrength = returnStrength.get();
        settings.damping = damping.get();
        simulation.update(bodyX.get(), bodyY.get(), forceX.get(), forceY.get(),
                          fearDistance.get(), strength.get(), settings, ofGetLastFrameTime(), doReset);
        // Stage both vectors before callbacks, and notify X last for X-driven consumers.
        const auto x = simulation.x(), y = simulation.y();
        const bool changedX = x != outX.get(), changedY = y != outY.get();
        outX.setWithoutEventNotifications(x); outY.setWithoutEventNotifications(y);
        if(changedX || changedY) {
            outY.set(y);
            outX.set(x);
        }
    }

    void presetRecallBeforeSettingParameters(ofJson&) override { resetPending = true; }
    void presetHasLoaded() override { resetPending = true; }
    void resetPhase() override { resetPending = true; }

private:
    ofParameter<vector<float>> bodyX, bodyY, forceX, forceY, strength, fearDistance, outX, outY;
    ofParameter<vector<float>> repulsion, returnStrength, damping;
    ofParameter<void> reset;
    ofEventListener resetListener;
    bodyRepulsionCore::Simulation simulation;
    bool resetPending = true;
};
