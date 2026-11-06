#pragma once

#include "ofxOceanodeNodeModel.h"
#include "borderBoundaryCore.h"
#include <cfloat>

class borderBoundary : public ofxOceanodeNodeModel {
public:
    borderBoundary() : ofxOceanodeNodeModel("Border Boundary") {}

    void setup() override {
        description = "Keep XY paths inside a rounded viewport boundary in normalized 0-1 space. "
            "Border distance insets all four sides; Roundness goes from square (0) to circle (1). "
            "Falloff softly compresses coordinates toward the centre near the boundary; zero "
            "hard-clamps along centre rays. Curve powers the falloff attenuation: higher values "
            "give a firmer edge. The interior is unchanged and outputs never exceed the boundary. "
            "Vertex count and -1 path separators are preserved. Only X triggers calculation: "
            "send Y first; controls apply on the next X event. Unequal XY sizes retain the last "
            "outputs. Nonfinite points or an unpaired -1 become paired path separators.";
        addParameter(xIn.set("X", {0.5f}, {-FLT_MAX}, {FLT_MAX}));
        addParameter(yIn.set("Y", {0.5f}, {-FLT_MAX}, {FLT_MAX}));
        addSeparator("Border", ofColor(100));
        addParameter(border.set("Border distance", 0.05f, 0.f, 0.5f));
        addParameter(falloff.set("Falloff", 0.05f, 0.f, 0.5f));
        addParameter(curve.set("Curve", 1.f, 0.1f, 8.f));
        addParameter(roundness.set("Roundness", 0.f, 0.f, 1.f));
        addSeparator("Outputs", ofColor(100));
        addOutputParameter(xOut.set("Out.X", {0.5f}, {-1.f}, {1.f}));
        addOutputParameter(yOut.set("Out.Y", {0.5f}, {-1.f}, {1.f}));
        listener = xIn.newListener([this](std::vector<float>&) {
            const borderBoundaryCore::Controls controls{
                border.get(), falloff.get(), curve.get(), roundness.get()};
            if(!borderBoundaryCore::process(xIn.get(), yIn.get(), controls, bufferX, bufferY)) return;
            // Stage both before either callback, then trigger X-driven consumers last.
            xOut.setWithoutEventNotifications(bufferX);
            yOut.setWithoutEventNotifications(bufferY);
            yOut.set(bufferY);
            xOut.set(bufferX);
        });
    }

private:
    ofParameter<std::vector<float>> xIn, yIn, xOut, yOut;
    ofParameter<float> border, falloff, curve, roundness;
    std::vector<float> bufferX, bufferY;
    ofEventListener listener;
};
