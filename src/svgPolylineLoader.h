#pragma once

#include "ofxOceanodeNodeModel.h"
#include "ofxSvg.h"
#include "imgui.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <sstream>

class svgPolylineLoader : public ofxOceanodeNodeModel {
public:
    svgPolylineLoader() : ofxOceanodeNodeModel("SVG Polyline Loader") {}

    void setup() override {
        description = "Load SVG outlines as paired coordinate vectors. Y updates before X. "
                      "Each contour ends with -1; closed contours repeat their first vertex. "
                      "Curve Resolution controls segments per Bezier, not equal-distance spacing.";
        addCustomRegion(loadButton.set("Load SVG", [this]() { drawLoadButton(); }),
                        [this]() { drawLoadButton(); });
        addParameter(filename.set("File", ""));
        addParameter(curveResolution.set("Curve Resolution", 32, 2, 1024));
        addParameter(normalize.set("Normalize", true));
        addParameter(rotate90.set("Rotate90º"));
        addParameter(flipX.set("FlipX", false));
        addParameter(flipY.set("FlipY", false));
        addParameter(scale.set("Scale", 1.0f, 0.0f, 1.0f));
        addParameter(rotate.set("Rotate", 0.0f, -1.0f, 1.0f));
        addParameter(moveX.set("MoveX", 0.0f, -1.0f, 1.0f));
        addParameter(moveY.set("MoveY", 0.0f, -1.0f, 1.0f));
        addInspectorParameter(quarterTurns.set("Quarter Turns", 0, 0, 3));
        addOutputParameter(vertexY.set("Vertex_Y", {}, {-FLT_MAX}, {FLT_MAX}));
        addOutputParameter(vertexX.set("Vertex_X", {}, {-FLT_MAX}, {FLT_MAX}));

        listeners.push(filename.newListener([this](string &) { loadSvg(); }));
        listeners.push(curveResolution.newListener([this](int &) { rebuildVertices(); }));
        listeners.push(normalize.newListener([this](bool &) { rebuildVertices(); }));
        listeners.push(rotate90.newListener([this]() {
            if (loaded) quarterTurns = (quarterTurns.get() + 1) % 4;
        }));
        listeners.push(quarterTurns.newListener([this](int &) { rebuildVertices(); }));
        listeners.push(flipX.newListener([this](bool &) { rebuildVertices(); }));
        listeners.push(flipY.newListener([this](bool &) { rebuildVertices(); }));
        listeners.push(scale.newListener([this](float &) { rebuildVertices(); }));
        listeners.push(rotate.newListener([this](float &) { rebuildVertices(); }));
        listeners.push(moveX.newListener([this](float &) { rebuildVertices(); }));
        listeners.push(moveY.newListener([this](float &) { rebuildVertices(); }));
    }

    void presetRecallAfterSettingParameters(ofJson &) override {
        // Reload even when File has the same value (the SVG may have changed).
        loadSvg();
    }

private:
    customGuiRegion loadButton;
    ofParameter<string> filename;
    ofParameter<int> curveResolution, quarterTurns;
    ofParameter<void> rotate90;
    ofParameter<bool> normalize, flipX, flipY;
    ofParameter<float> scale, rotate, moveX, moveY;
    ofParameter<vector<float>> vertexY, vertexX;
    ofxSvg svg;
    bool loaded = false;
    string status;
    ofEventListeners listeners;

    void drawLoadButton() {
        if (ImGui::Button("Load SVG")) {
            auto result = ofSystemLoadDialog("Select SVG file", false, ofToDataPath("", true));
            if (result.bSuccess) {
                const string path = result.getPath();
                if (ofToLower(ofFilePath::getFileExt(path)) == "svg") {
                    if (filename.get() == path) loadSvg();
                    else filename = path;
                } else {
                    status = "Select an .svg file.";
                }
            }
        }
        if (!status.empty()) ImGui::TextWrapped("%s", status.c_str());
    }

    void publish(const vector<float> &ys, const vector<float> &xs) {
        // Coordinate processors in this addon use X as the processing trigger.
        vertexY = ys;
        vertexX = xs;
    }

    void fail(const string &message) {
        loaded = false;
        svg = ofxSvg();
        status = message;
        publish({}, {});
        if (!message.empty()) ofLogWarning("SVG Polyline Loader") << message;
    }

    void loadSvg() {
        if (filename.get().empty()) {
            fail("");
            return;
        }
        const string path = ofToDataPath(filename.get(), true);
        if (ofToLower(ofFilePath::getFileExt(path)) != "svg" || !ofFile::doesFileExist(path)) {
            fail("Cannot read SVG: " + path);
            return;
        }
        ofXml document;
        if (!document.parse(ofBufferFromFile(path).getText())) {
            fail("Invalid SVG XML: " + path);
            return;
        }
        auto root = document.getChild("svg");
        if (!root) {
            fail("The document has no SVG root: " + path);
            return;
        }
        // Illustrator commonly exports just a viewBox. libsvgtiny needs a
        // viewport size to apply the viewBox transform correctly.
        const auto viewBoxAttribute = root.getAttribute("viewBox");
        if (viewBoxAttribute) {
            string viewBox = viewBoxAttribute.getValue();
            std::replace(viewBox.begin(), viewBox.end(), ',', ' ');
            std::istringstream values(viewBox);
            float x, y, width, height;
            if (values >> x >> y >> width >> height && std::isfinite(width) &&
                std::isfinite(height) && width > 0 && height > 0) {
                if (!root.getAttribute("width")) root.setAttribute("width", width);
                if (!root.getAttribute("height")) root.setAttribute("height", height);
            }
        }
        svg = ofxSvg();
        svg.loadFromString(document.toString(), path);
        loaded = svg.getNumPath() > 0;
        if (!loaded) {
            fail("No supported vector paths in SVG: " + path);
            return;
        }
        rebuildVertices();
    }

    void rebuildVertices() {
        if (!loaded) return;
        float scaleX = 1.0f, scaleY = 1.0f;
        if (normalize) {
            const float width = svg.getWidth(), height = svg.getHeight();
            if (!std::isfinite(width) || !std::isfinite(height) || width <= 0 || height <= 0) {
                status = "Normalize needs a positive SVG width/height or viewBox.";
                publish({}, {});
                return;
            }
            scaleX /= width;
            scaleY /= height;
        }
        vector<float> xs, ys;
        const float centerX = normalize ? 0.5f : svg.getWidth() * 0.5f;
        const float centerY = normalize ? 0.5f : svg.getHeight() * 0.5f;
        // Move remains a fraction of the viewport even with Normalize disabled.
        const float offsetX = moveX.get() * (normalize ? 1.0f : svg.getWidth());
        const float offsetY = moveY.get() * (normalize ? 1.0f : svg.getHeight());
        constexpr float halfPi = 1.57079632679489661923f;
        const float angle = rotate.get() * halfPi;
        const float cosine = std::cos(angle), sine = std::sin(angle);
        auto appendVertex = [&](const glm::vec3 &vertex) {
            float x = vertex.x * scaleX, y = vertex.y * scaleY;
            const float dx = x - centerX, dy = y - centerY;
            // SVG Y points down, so this is clockwise. Rebuild from the
            // source on every edit: four clicks restore the original exactly.
            switch (quarterTurns.get()) {
                case 1: x = centerX - dy; y = centerY + dx; break;
                case 2: x = centerX - dx; y = centerY - dy; break;
                case 3: x = centerX + dy; y = centerY - dx; break;
                default: break;
            }
            // Add the continuous angle to the button's quarter-turn rotation.
            if (rotate.get() != 0.0f) {
                const float rx = x - centerX, ry = y - centerY;
                x = centerX + rx * cosine - ry * sine;
                y = centerY + rx * sine + ry * cosine;
            }
            // Flips use the output axes, after rotation. Move is applied last.
            if (flipX) x = 2.0f * centerX - x;
            if (flipY) y = 2.0f * centerY - y;
            if (scale.get() != 1.0f) {
                x = centerX + (x - centerX) * scale.get();
                y = centerY + (y - centerY) * scale.get();
            }
            x += offsetX;
            y += offsetY;
            xs.push_back(x);
            ys.push_back(y);
        };
        for (auto geometry : svg.getPaths()) {
            // ODD returns original subpaths instead of tessellated fill/stroke
            // boundaries, preserving open paths, holes and fill-only shapes.
            geometry.setPolyWindingMode(OF_POLY_WINDING_ODD);
            geometry.setCurveResolution(std::max(2, std::min(1024, curveResolution.get())));
            for (const auto &outline : geometry.getOutline()) {
                const auto &vertices = outline.getVertices();
                if (vertices.empty()) continue;
                if (!std::all_of(vertices.begin(), vertices.end(), [](const glm::vec3 &v) {
                    return std::isfinite(v.x) && std::isfinite(v.y);
                })) continue;
                for (const auto &v : vertices) {
                    appendVertex(v);
                }
                // libsvgtiny marks <line> closed too; avoid retracing a
                // two-point line. Real closed contours need a closing edge.
                if (outline.isClosed() && vertices.size() > 2 && vertices.front() != vertices.back()) {
                    appendVertex(vertices.front());
                }
                // Include the final terminator, matching the addon's resamplers.
                xs.push_back(-1.0f);
                ys.push_back(-1.0f);
            }
        }
        status = xs.empty() ? "No usable contours in SVG." : "";
        publish(ys, xs);
    }
};
