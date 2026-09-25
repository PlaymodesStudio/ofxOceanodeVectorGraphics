#ifndef vectorPrimitives_h
#define vectorPrimitives_h

#include "ofxOceanodeNodeModel.h"

#include <algorithm>
#include <cfloat>
#include <climits>
#include <cmath>

class vectorPrimitives : public ofxOceanodeNodeModel {
public:
    vectorPrimitives() : ofxOceanodeNodeModel("Vector Primitives") {}

    void setup() override {
        addSeparator("Canvas", ofColor(128));
        addParameter(width.set("Width", 512, 1, INT_MAX));
        addParameter(height.set("Height", 512, 1, INT_MAX));
        addParameter(bgR.set("BG R", 0.0f, 0.0f, 1.0f));
        addParameter(bgG.set("BG G", 0.0f, 0.0f, 1.0f));
        addParameter(bgB.set("BG B", 0.0f, 0.0f, 1.0f));
        addParameter(bgAlpha.set("BG Alpha", 0.0f, 0.0f, 1.0f));
        addParameterDropdown(blendMode, "Blend Mode", 0,
                             {"Normal", "Add", "Multiply", "Screen", "Subtract",
                              "Lighten / Max", "Darken / Min", "Replace"});

        addSeparator("Primitives", ofColor(255, 180, 64));
        addParameter(num.set("Num", 1, 0, INT_MAX));
        // 1: rectangle, 2: circle/ellipse, 3: triangle, 4: line,
        // 5: dashed line, 6: random Bezier.
        addParameter(type.set("Type", {1}, {1}, {6}));
        addParameter(sizeX.set("Size X", {0.25f}, {0.0f}, {1.0f}));
        addParameter(sizeY.set("Size Y", {0.25f}, {0.0f}, {1.0f}));

        addSeparator("Transform", ofColor(0, 128, 255));
        addParameterDropdown(anchorMode, "Anchor Mode", 0,
                             {"Center", "Top Right", "Top Left",
                              "Bottom Right", "Bottom Left"});
        addParameter(x.set("X", {0.5f}, {0.0f}, {1.0f}));
        addParameter(y.set("Y", {0.5f}, {0.0f}, {1.0f}));
        // Rotation follows the normalized-turn convention used by the other
        // vector nodes: 1.0 is one complete clockwise revolution.
        addParameter(rotation.set("Rotation", {0.0f}, {-FLT_MAX}, {FLT_MAX}));

        addSeparator("Special", ofColor(255, 210, 64));
        addParameter(specialA.set("A", {0.1f}, {0.0f}, {1.0f}));
        addParameter(specialB.set("B", {0.1f}, {0.0f}, {1.0f}));
        addParameter(specialC.set("C", {1.0f / 3.0f}, {0.0f}, {1.0f}));
        addParameter(specialD.set("D", {2.0f / 3.0f}, {0.0f}, {1.0f}));

        addSeparator("Effects", ofColor(150, 110, 255));
        addParameter(shadow.set("Shadow", {0.0f}, {0.0f}, {1.0f}));
        addParameter(shadowBlur.set("Shadow Blur", 0.02f, 0.0f, 1.0f));
        addParameter(shadowX.set("Shadow X", {0.02f}, {-1.0f}, {1.0f}));
        addParameter(shadowY.set("Shadow Y", {0.02f}, {-1.0f}, {1.0f}));
        addParameter(glow.set("Glow", {0.0f}, {0.0f}, {1.0f}));
        addParameter(glowBlur.set("Glow Blur", 0.02f, 0.0f, 1.0f));

        addSeparator("Fill", ofColor(255, 64, 255));
        addParameter(r.set("Fill R", {1.0f}, {0.0f}, {1.0f}));
        addParameter(g.set("Fill G", {1.0f}, {0.0f}, {1.0f}));
        addParameter(b.set("Fill B", {1.0f}, {0.0f}, {1.0f}));
        addParameter(alpha.set("Fill Alpha", {1.0f}, {0.0f}, {1.0f}));

        addSeparator("Stroke", ofColor(64, 220, 160));
        addParameter(strokeWeight.set("Stroke Weight", {1.0f}, {0.0f}, {FLT_MAX}));
        addParameter(strokeR.set("Stroke R", {0.0f}, {0.0f}, {1.0f}));
        addParameter(strokeG.set("Stroke G", {0.0f}, {0.0f}, {1.0f}));
        addParameter(strokeB.set("Stroke B", {0.0f}, {0.0f}, {1.0f}));
        addParameter(strokeAlpha.set("Stroke Alpha", {1.0f}, {0.0f}, {1.0f}));

        addSeparator("Output", ofColor(128));
        addOutputParameter(output.set("Texture", nullptr));

        // Kept in the inspector because it is a quality setting rather than a
        // per-object control, but exposing it avoids a hard-coded circle shape.
        addInspectorParameter(circleResolution.set("Circle Resolution", 64, 3, 256));

        setupBlurShader();
    }

    void update(ofEventArgs &) override {
        render();
    }

    void deactivate() override {
        output = nullptr;
        fbo.clear();
        effectSourceFbo.clear();
        effectPingFbo.clear();
        effectResultFbo.clear();
    }

private:
    ofParameter<int> width;
    ofParameter<int> height;
    ofParameter<float> bgR;
    ofParameter<float> bgG;
    ofParameter<float> bgB;
    ofParameter<float> bgAlpha;
    ofParameter<vector<int>> blendMode;

    ofParameter<int> num;
    ofParameter<vector<int>> type;
    ofParameter<vector<float>> sizeX;
    ofParameter<vector<float>> sizeY;
    ofParameter<vector<int>> anchorMode;
    ofParameter<vector<float>> x;
    ofParameter<vector<float>> y;
    ofParameter<vector<float>> rotation;
    ofParameter<vector<float>> specialA;
    ofParameter<vector<float>> specialB;
    ofParameter<vector<float>> specialC;
    ofParameter<vector<float>> specialD;

    ofParameter<vector<float>> shadow;
    ofParameter<float> shadowBlur;
    ofParameter<vector<float>> shadowX;
    ofParameter<vector<float>> shadowY;
    ofParameter<vector<float>> glow;
    ofParameter<float> glowBlur;

    ofParameter<vector<float>> r;
    ofParameter<vector<float>> g;
    ofParameter<vector<float>> b;
    ofParameter<vector<float>> alpha;

    ofParameter<vector<float>> strokeWeight;
    ofParameter<vector<float>> strokeR;
    ofParameter<vector<float>> strokeG;
    ofParameter<vector<float>> strokeB;
    ofParameter<vector<float>> strokeAlpha;

    ofParameter<int> circleResolution;
    ofParameter<ofTexture*> output;
    ofFbo fbo;
    ofFbo effectSourceFbo;
    ofFbo effectPingFbo;
    ofFbo effectResultFbo;
    ofShader blurShader;
    bool blurShaderReady = false;

    template <typename T>
    static T valueAt(const vector<T> &values, size_t index, T fallback) {
        if (values.empty()) return fallback;
        if (values.size() == 1 || index >= values.size()) return values.front();
        return values[index];
    }

    ofBlendMode selectedBlendMode(size_t index) const {
        switch (valueAt(blendMode.get(), index, 0)) {
            case 1: return OF_BLENDMODE_ADD;
            case 2: return OF_BLENDMODE_MULTIPLY;
            case 3: return OF_BLENDMODE_SCREEN;
            case 4: return OF_BLENDMODE_SUBTRACT;
            case 5: return OF_BLENDMODE_MAX;
            case 6: return OF_BLENDMODE_MIN;
            case 7: return OF_BLENDMODE_DISABLED;
            case 0:
            default: return OF_BLENDMODE_ALPHA;
        }
    }

    glm::vec2 anchorOffset(float primitiveWidth, float primitiveHeight, size_t index) const {
        switch (valueAt(anchorMode.get(), index, 0)) {
            case 1: // Top Right
                return glm::vec2(-primitiveWidth * 0.5f, primitiveHeight * 0.5f);
            case 2: // Top Left
                return glm::vec2(primitiveWidth * 0.5f, primitiveHeight * 0.5f);
            case 3: // Bottom Right
                return glm::vec2(-primitiveWidth * 0.5f, -primitiveHeight * 0.5f);
            case 4: // Bottom Left
                return glm::vec2(primitiveWidth * 0.5f, -primitiveHeight * 0.5f);
            case 0: // Center
            default:
                return glm::vec2(0.0f);
        }
    }

    static float randomSigned(float seed, float salt) {
        const float value = std::sin(seed * 12.9898f + salt * 78.233f) * 43758.5453f;
        return (value - std::floor(value)) * 2.0f - 1.0f;
    }

    void addDashedLine(ofPath &path, float primitiveWidth, float primitiveHeight,
                       size_t index) const {
        const glm::vec2 start(-primitiveWidth * 0.5f, -primitiveHeight * 0.5f);
        const glm::vec2 end(primitiveWidth * 0.5f, primitiveHeight * 0.5f);
        const glm::vec2 delta = end - start;
        const float length = glm::length(delta);
        const float dashFraction = valueAt(specialA.get(), index, 0.1f);
        const float gapFraction = valueAt(specialB.get(), index, 0.1f);
        const float periodFraction = dashFraction + gapFraction;

        if (length <= 0.0f || dashFraction <= 0.0f || periodFraction <= 0.0f) return;

        const glm::vec2 direction = delta / length;
        // Avoid an unbounded number of path segments if a connected value is
        // extremely close to zero, while preserving the dash-to-gap ratio.
        const float safePeriodFraction = std::max(periodFraction, 1.0f / 10000.0f);
        const float step = safePeriodFraction * length;
        const float dashLength = step * (dashFraction / periodFraction);
        for (float distance = 0.0f; distance < length; distance += step) {
            const float dashEnd = std::min(distance + dashLength, length);
            path.moveTo(start + direction * distance);
            path.lineTo(start + direction * dashEnd);
        }
    }

    void addRandomBezier(ofPath &path, float primitiveWidth, float primitiveHeight,
                         size_t index) const {
        const glm::vec2 start(-primitiveWidth * 0.5f, -primitiveHeight * 0.5f);
        const glm::vec2 end(primitiveWidth * 0.5f, primitiveHeight * 0.5f);
        const glm::vec2 delta = end - start;
        const float length = glm::length(delta);
        glm::vec2 perpendicular(0.0f, -1.0f);
        if (length > 0.0f) {
            perpendicular = glm::vec2(-delta.y, delta.x) / length;
        }

        const float eccentricity = valueAt(specialA.get(), index, 0.1f);
        const float amplitude = eccentricity * std::max(std::abs(primitiveWidth),
                                                        std::abs(primitiveHeight));
        const float seed = valueAt(specialB.get(), index, 0.1f) +
                           static_cast<float>(index) * 101.317f;
        const float control1Position = valueAt(specialC.get(), index, 1.0f / 3.0f);
        const float control2Position = valueAt(specialD.get(), index, 2.0f / 3.0f);
        const glm::vec2 control1 = start + delta * control1Position +
                                   perpendicular * randomSigned(seed, 1.0f) * amplitude;
        const glm::vec2 control2 = start + delta * control2Position +
                                   perpendicular * randomSigned(seed, 2.0f) * amplitude;

        path.moveTo(start);
        path.bezierTo(control1, control2, end);
    }

    ofPath makePrimitive(int primitiveType, float primitiveWidth, float primitiveHeight,
                         size_t index) const {
        ofPath path;
        path.setCircleResolution(circleResolution.get());

        switch (primitiveType) {
            case 2:
                path.ellipse(0.0f, 0.0f, primitiveWidth, primitiveHeight);
                break;
            case 3:
                path.triangle(-primitiveWidth * 0.5f, primitiveHeight * 0.5f,
                              0.0f, -primitiveHeight * 0.5f,
                              primitiveWidth * 0.5f, primitiveHeight * 0.5f);
                break;
            case 4:
                path.moveTo(-primitiveWidth * 0.5f, -primitiveHeight * 0.5f);
                path.lineTo(primitiveWidth * 0.5f, primitiveHeight * 0.5f);
                break;
            case 5:
                addDashedLine(path, primitiveWidth, primitiveHeight, index);
                break;
            case 6:
                addRandomBezier(path, primitiveWidth, primitiveHeight, index);
                break;
            case 1:
            default:
                path.rectangle(-primitiveWidth * 0.5f, -primitiveHeight * 0.5f,
                               primitiveWidth, primitiveHeight);
                break;
        }

        return path;
    }

    static ofFloatColor withScaledAlpha(ofFloatColor color, float scale) {
        color.a = ofClamp(color.a * scale, 0.0f, 1.0f);
        return color;
    }

    struct RenderItem {
        ofPath path;
        glm::vec2 position = glm::vec2(0.0f);
        glm::vec2 anchor = glm::vec2(0.0f);
        glm::vec2 shadowOffset = glm::vec2(0.0f);
        float rotationTurns = 0.0f;
        float shadowAmount = 0.0f;
        float glowAmount = 0.0f;
        ofFloatColor fillColor;
        ofFloatColor strokeColor;
        ofBlendMode blendMode = OF_BLENDMODE_ALPHA;
    };

    static void drawTransformedPath(const ofPath &path, const glm::vec2 &position,
                                    float rotationTurns, const glm::vec2 &offset) {
        ofPushMatrix();
        ofTranslate(position.x, position.y);
        ofRotateDeg(rotationTurns * 360.0f);
        ofTranslate(offset.x, offset.y);
        path.draw();
        ofPopMatrix();
    }

    void setupBlurShader() {
        if (!ofIsGLProgrammableRenderer()) {
            ofLogWarning("vectorPrimitives")
                << "Pixel blur requires the programmable OpenGL renderer";
            return;
        }

        const string vertexSource = R"(
            #version 410
            uniform mat4 modelViewProjectionMatrix;
            layout(location = 0) in vec4 position;
            void main() {
                gl_Position = modelViewProjectionMatrix * position;
            }
        )";

        const string fragmentSource = R"(
            #version 410
            uniform sampler2D sourceTex;
            uniform vec2 resolution;
            uniform vec2 direction;
            uniform float radius;
            out vec4 fragColor;

            void main() {
                vec2 uv = gl_FragCoord.xy / resolution;
                float sampleScale = max(radius, 0.0) / 3.23076923;
                vec2 stepOffset = direction * sampleScale / resolution;

                vec4 color = texture(sourceTex, uv) * 0.22702703;
                color += texture(sourceTex, uv + stepOffset * 1.38461538) * 0.31621622;
                color += texture(sourceTex, uv - stepOffset * 1.38461538) * 0.31621622;
                color += texture(sourceTex, uv + stepOffset * 3.23076923) * 0.07027027;
                color += texture(sourceTex, uv - stepOffset * 3.23076923) * 0.07027027;
                fragColor = color;
            }
        )";

        blurShaderReady = blurShader.setupShaderFromSource(GL_VERTEX_SHADER, vertexSource) &&
                          blurShader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragmentSource) &&
                          blurShader.bindDefaults() && blurShader.linkProgram();
        if (!blurShaderReady) {
            ofLogWarning("vectorPrimitives")
                << "Could not compile the shadow/glow blur shader";
        }
    }

    static ofFbo::Settings fboSettings(int canvasWidth, int canvasHeight) {
        ofFbo::Settings settings;
        settings.width = canvasWidth;
        settings.height = canvasHeight;
        settings.internalformat = GL_RGBA8;
        settings.numColorbuffers = 1;
        settings.useDepth = false;
        settings.useStencil = false;
        settings.textureTarget = GL_TEXTURE_2D;
        settings.minFilter = GL_LINEAR;
        settings.maxFilter = GL_LINEAR;
        return settings;
    }

    static void ensureFbo(ofFbo &target, int canvasWidth, int canvasHeight) {
        if (!target.isAllocated() ||
            target.getWidth() != canvasWidth ||
            target.getHeight() != canvasHeight) {
            target.allocate(fboSettings(canvasWidth, canvasHeight));
        }
    }

    void renderEffectSource(const vector<RenderItem> &items, bool shadowPass) {
        effectSourceFbo.begin();
        ofClear(0, 0, 0, 0);
        ofPushStyle();
        ofEnableBlendMode(shadowPass ? OF_BLENDMODE_ALPHA : OF_BLENDMODE_ADD);

        for (const RenderItem &item : items) {
            const float amount = shadowPass ? item.shadowAmount : item.glowAmount;
            if (amount <= 0.0f) continue;

            ofPath effectPath = item.path;
            if (shadowPass) {
                effectPath.setFillColor(ofFloatColor(0.0f, 0.0f, 0.0f,
                    ofClamp(item.fillColor.a * amount, 0.0f, 1.0f)));
                effectPath.setStrokeColor(ofFloatColor(0.0f, 0.0f, 0.0f,
                    ofClamp(item.strokeColor.a * amount, 0.0f, 1.0f)));
            } else {
                effectPath.setFillColor(withScaledAlpha(item.fillColor, amount));
                effectPath.setStrokeColor(withScaledAlpha(item.strokeColor, amount));
            }

            drawTransformedPath(effectPath,
                                item.position + (shadowPass ? item.shadowOffset : glm::vec2(0.0f)),
                                item.rotationTurns, item.anchor);
        }

        ofPopStyle();
        effectSourceFbo.end();
    }

    void blurPass(const ofTexture &source, ofFbo &destination,
                  const glm::vec2 &direction, float radiusPixels) {
        destination.begin();
        ofClear(0, 0, 0, 0);
        ofPushStyle();
        ofDisableAlphaBlending();
        ofSetColor(255);

        blurShader.begin();
        blurShader.setUniformTexture("sourceTex", source, 0);
        blurShader.setUniform2f("resolution", destination.getWidth(), destination.getHeight());
        blurShader.setUniform2f("direction", direction);
        blurShader.setUniform1f("radius", radiusPixels);
        ofDrawRectangle(0, 0, destination.getWidth(), destination.getHeight());
        blurShader.end();

        ofPopStyle();
        destination.end();
    }

    void copyTexture(const ofTexture &source, ofFbo &destination) {
        destination.begin();
        ofClear(0, 0, 0, 0);
        ofPushStyle();
        ofDisableAlphaBlending();
        ofSetColor(255);
        source.draw(0, 0, destination.getWidth(), destination.getHeight());
        ofPopStyle();
        destination.end();
    }

    void blurEffect(float radiusPixels) {
        if (!blurShaderReady || radiusPixels <= 0.0f) {
            copyTexture(effectSourceFbo.getTexture(), effectResultFbo);
            return;
        }

        blurPass(effectSourceFbo.getTexture(), effectPingFbo,
                 glm::vec2(1.0f, 0.0f), radiusPixels);
        blurPass(effectPingFbo.getTexture(), effectResultFbo,
                 glm::vec2(0.0f, 1.0f), radiusPixels);
    }

    void render() {
        const int canvasWidth = width.get();
        const int canvasHeight = height.get();
        if (canvasWidth <= 0 || canvasHeight <= 0) {
            output = nullptr;
            return;
        }

        ensureFbo(fbo, canvasWidth, canvasHeight);

        const int objectCount = std::max(0, num.get());
        // A shared reference preserves primitive aspect ratios on non-square
        // canvases: equal Size X/Y values produce a square or true circle.
        const float sizeReference = static_cast<float>(std::min(canvasWidth, canvasHeight));
        vector<RenderItem> items;
        items.reserve(static_cast<size_t>(objectCount));
        bool hasShadow = false;
        bool hasGlow = false;

        for (int i = 0; i < objectCount; ++i) {
            const size_t index = static_cast<size_t>(i);
            const int primitiveType = ofClamp(valueAt(type.get(), index, 1), 1, 6);
            const float primitiveWidth = valueAt(sizeX.get(), index, 0.25f) * sizeReference;
            const float primitiveHeight = valueAt(sizeY.get(), index, 0.25f) * sizeReference;

            ofPath path = makePrimitive(primitiveType, primitiveWidth, primitiveHeight, index);
            path.setFilled(primitiveType <= 3);
            const ofFloatColor fillColor(
                ofClamp(valueAt(r.get(), index, 1.0f), 0.0f, 1.0f),
                ofClamp(valueAt(g.get(), index, 1.0f), 0.0f, 1.0f),
                ofClamp(valueAt(b.get(), index, 1.0f), 0.0f, 1.0f),
                ofClamp(valueAt(alpha.get(), index, 1.0f), 0.0f, 1.0f));
            const ofFloatColor strokeColor(
                ofClamp(valueAt(strokeR.get(), index, 0.0f), 0.0f, 1.0f),
                ofClamp(valueAt(strokeG.get(), index, 0.0f), 0.0f, 1.0f),
                ofClamp(valueAt(strokeB.get(), index, 0.0f), 0.0f, 1.0f),
                ofClamp(valueAt(strokeAlpha.get(), index, 1.0f), 0.0f, 1.0f));
            path.setFillColor(fillColor);
            path.setStrokeColor(strokeColor);
            path.setStrokeWidth(std::max(0.0f, valueAt(strokeWeight.get(), index, 1.0f)));

            const glm::vec2 position(valueAt(x.get(), index, 0.5f) * canvasWidth,
                                     valueAt(y.get(), index, 0.5f) * canvasHeight);
            const float rotationTurns = valueAt(rotation.get(), index, 0.0f);
            const glm::vec2 offset = anchorOffset(primitiveWidth, primitiveHeight, index);

            RenderItem item;
            item.path = std::move(path);
            item.position = position;
            item.anchor = offset;
            item.rotationTurns = rotationTurns;
            item.shadowAmount = ofClamp(valueAt(shadow.get(), index, 0.0f), 0.0f, 1.0f);
            item.glowAmount = ofClamp(valueAt(glow.get(), index, 0.0f), 0.0f, 1.0f);
            item.shadowOffset = glm::vec2(
                valueAt(shadowX.get(), index, 0.02f) * sizeReference,
                valueAt(shadowY.get(), index, 0.02f) * sizeReference);
            item.fillColor = fillColor;
            item.strokeColor = strokeColor;
            item.blendMode = selectedBlendMode(index);
            hasShadow = hasShadow || item.shadowAmount > 0.0f;
            hasGlow = hasGlow || item.glowAmount > 0.0f;
            items.emplace_back(std::move(item));
        }

        if (hasShadow || hasGlow) {
            ensureFbo(effectSourceFbo, canvasWidth, canvasHeight);
            ensureFbo(effectPingFbo, canvasWidth, canvasHeight);
            ensureFbo(effectResultFbo, canvasWidth, canvasHeight);
        }

        // Effects are rasterized from the complete vector layer and blurred in
        // two GPU passes. This keeps them pixel-based while the final paths stay
        // sharp, and makes cost independent of the blur radius.
        if (hasShadow) {
            renderEffectSource(items, true);
            blurEffect(shadowBlur.get() * sizeReference);
        }

        fbo.begin();
        ofClear(ofFloatColor(bgR.get(), bgG.get(), bgB.get(), bgAlpha.get()));
        ofPushStyle();
        ofSetColor(255);
        if (hasShadow) {
            ofEnableBlendMode(OF_BLENDMODE_ALPHA);
            effectResultFbo.draw(0, 0, canvasWidth, canvasHeight);
        }
        ofPopStyle();
        fbo.end();

        if (hasGlow) {
            renderEffectSource(items, false);
            blurEffect(glowBlur.get() * sizeReference);
        }

        fbo.begin();
        ofPushStyle();
        ofSetColor(255);
        if (hasGlow) {
            ofEnableBlendMode(OF_BLENDMODE_ADD);
            effectResultFbo.draw(0, 0, canvasWidth, canvasHeight);
        }
        for (const RenderItem &item : items) {
            ofEnableBlendMode(item.blendMode);
            drawTransformedPath(item.path, item.position,
                                item.rotationTurns, item.anchor);
        }
        ofPopStyle();
        fbo.end();

        output = &fbo.getTexture();
    }
};

#endif /* vectorPrimitives_h */
