#pragma once

#include "ofxOceanodeNodeModel.h"
#include "twoBodyRepulsionCore.h"
#include <cfloat>

class twoBodyRepulsion : public ofxOceanodeNodeModel {
public:
    twoBodyRepulsion() : ofxOceanodeNodeModel("TwoBody Repulsion") {}

    void setup() override {
        description = "Inertial repulsion between two bodies. Each Body's coordinates are moving spring "
            "rest positions, and every point is a disk of its Radius. A point flees the other body "
            "when the gap between disk surfaces is inside its FearDistance, accelerating by its "
            "Repulsion Strength times the other body's Strength. Return Strength pulls back; Damping "
            "slows momentum. Radius, Strength, FearDistance, Repulsion Strength, Return Strength and "
            "Damping map to that body's points; one value broadcasts, shorter vectors repeat their "
            "last value, excluding -1 separators. Border Margin keeps every point that far inside the "
            "0-1 canvas, ignoring Radius; Border Feather is the width of a soft push before the margin. Both at zero "
            "leave the output unclamped. Clamp 0/1 holds every point inside the 0-1 canvas, dropping "
            "momentum into the edge, even with the border off. Reset restores both bodies. Point count "
            "or separator layout changes reset that body only. Empty or invalid Body pairs clear that "
            "body's output. Bypass copies all four input coordinates directly to the outputs without "
            "simulation or clamping.";
        addParameter(bypass.set("Bypass", false));
		addParameter(reset.set("Reset"));
        addParameter(clamp.set("Clamp 0/1", false));
		addSeparator("BODY A", ofColor(100));
		addBody(a, "A", 0.3f);
		addSeparator("BODY B", ofColor(100));
        addBody(b, "B", 0.7f);
		addSeparator("MARGIN", ofColor(100));
        addParameter(borderMargin.set("Border Margin", 0.0f, 0.0f, 0.5f));
        addParameter(borderFeather.set("Border Feather", 0.0f, 0.0f, 0.5f));
		
        resetListener = reset.newListener([this]() { resetPending = true; });
    }

    void update(ofEventArgs&) override {
        if(bypass.get()) {
            resetPending = true;
            publishOutputs(a.x.get(), a.y.get(), b.x.get(), b.y.get());
            return;
        }
        const bool doReset = resetPending;
        resetPending = false;
        twoBodyRepulsionCore::Border border;
        border.clamp = clamp.get();
        border.margin = borderMargin.get();
        border.feather = borderFeather.get();
        simulation.update(a.x.get(), a.y.get(), controls(a), b.x.get(), b.y.get(), controls(b),
                          border, ofGetLastFrameTime(), doReset);
        const auto ax = simulation.aX(), ay = simulation.aY(), bx = simulation.bX(), by = simulation.bY();
        publishOutputs(ax, ay, bx, by);
    }

    void presetRecallBeforeSettingParameters(ofJson&) override { resetPending = true; }
    void presetHasLoaded() override { resetPending = true; }
    void resetPhase() override { resetPending = true; }

private:
    void publishOutputs(const vector<float>& ax, const vector<float>& ay,
                        const vector<float>& bx, const vector<float>& by) {
        // Stage all four vectors before callbacks, and notify each body's X last for X-driven consumers.
        const bool aChanged = ax != a.outX.get() || ay != a.outY.get();
        const bool bChanged = bx != b.outX.get() || by != b.outY.get();
        a.outX.setWithoutEventNotifications(ax); a.outY.setWithoutEventNotifications(ay);
        b.outX.setWithoutEventNotifications(bx); b.outY.setWithoutEventNotifications(by);
        if(aChanged) { a.outY.set(ay); a.outX.set(ax); }
        if(bChanged) { b.outY.set(by); b.outX.set(bx); }
    }

    struct BodyPorts {
        ofParameter<vector<float>> x, y, radius, strength, fearDistance, repulsion, returnStrength, damping;
        ofParameter<vector<float>> outX, outY;
    };

    void addBody(BodyPorts& p, const string& prefix, float restX) {
        addParameter(p.x.set(prefix + " Body_X", {restX}, {-FLT_MAX}, {FLT_MAX}));
        addParameter(p.y.set(prefix + " Body_Y", {0.5f}, {-FLT_MAX}, {FLT_MAX}));
        addParameter(p.radius.set(prefix + " Radius", {0.0f}, {0.0f}, {1.0f}));
        addParameter(p.strength.set(prefix + " Strength", {1.0f}, {0.0f}, {1.0f}));
        addParameter(p.fearDistance.set(prefix + " FearDistance", {0.15f}, {0.0f}, {1.0f}));
        addParameter(p.repulsion.set(prefix + " Repulsion Strength", {40.0f}, {0.0f}, {1000.0f}));
        addParameter(p.returnStrength.set(prefix + " Return Strength", {8.0f}, {0.0f}, {1000.0f}));
        addParameter(p.damping.set(prefix + " Damping", {3.0f}, {0.0f}, {100.0f}));
        addOutputParameter(p.outX.set(prefix + " Out_X", {}, {-FLT_MAX}, {FLT_MAX}));
        addOutputParameter(p.outY.set(prefix + " Out_Y", {}, {-FLT_MAX}, {FLT_MAX}));
    }

    static twoBodyRepulsionCore::BodyControls controls(const BodyPorts& p) {
        twoBodyRepulsionCore::BodyControls c;
        c.radius = p.radius.get();
        c.strength = p.strength.get();
        c.fear = p.fearDistance.get();
        c.repulsion = p.repulsion.get();
        c.returnStrength = p.returnStrength.get();
        c.damping = p.damping.get();
        return c;
    }

    BodyPorts a, b;
    ofParameter<bool> bypass, clamp;
    ofParameter<float> borderMargin, borderFeather;
    ofParameter<void> reset;
    ofEventListener resetListener;
    twoBodyRepulsionCore::Simulation simulation;
    bool resetPending = true;
};
