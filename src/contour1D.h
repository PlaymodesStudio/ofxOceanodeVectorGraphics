#pragma once

#include "ofxOceanodeNodeModel.h"
#include "contour1DCore.h"
#include <cfloat>

class contour1D : public ofxOceanodeNodeModel {
public:
    contour1D() : ofxOceanodeNodeModel("1D Contour") {}

    void setup() override {
        description = "Unfolds a closed body outline (e.g. a MetaballsV3 contour) along its spine into a "
            "1D profile: the distance from the spine to the outline, from head to tail, resampled to "
            "Size values. The profile depends only on the body's shape: moving, rotating or bending "
            "the body, or changing where its path starts, gives the same vector. Meant to drive "
            "a Spectral Filter Box mask (Size 256) or any other 1D consumer. The two tips of the "
            "body are its farthest pair of outline points, carried from frame to frame with the body "
            "so they never hop; the spine is the midline between the two sides of the outline, "
            "measured by its own length. Head picks which tip comes first: Wide End (the fatter "
            "end, default), Narrow End, Point (the tip nearest Point.X/Y, e.g. Metab Points' "
            "Cap 1 X/Y, one point per shape) or Motion (the tip leading the body's movement). "
            "Every rule keeps last frame's choice unless the other end wins by Hysteresis, so a "
            "symmetric body never swaps ends. Flip swaps head and tail. Profile: Width (both sides "
            "added), Left or Right side alone (left of the head-to-tail direction on screen), or "
            "Loop (left head to tail, then right tail to head: the whole outline unwrapped, 2 x Size "
            "values). Shape chooses the body: Largest, Index, or All (one profile per shape, "
            "separated by -1; not for a mask). Normalize scales each profile so its peak is 1, Gain "
            "multiplies (half widths in 0-1 coordinates are small, so a Gain of 3-10 or Normalize "
            "is usual for a mask). Spine Smooth calms the spine on noisy outlines, Smooth blurs the "
            "profile. Resample is the number of outline points analysed. Length (spine length), "
            "Head/Tail and Spine (-1 separated) are output per shape for drawing and checking.";
        addParameter(xIn.set("In.X", {0.5f}, {-FLT_MAX}, {FLT_MAX}));
        addParameter(yIn.set("In.Y", {0.5f}, {-FLT_MAX}, {FLT_MAX}));
        addParameter(resample.set("Resample", 512, 16, 4096));
        addParameter(size.set("Size", 256, 2, 4096));
        addParameterDropdown(head, "Head", 0, {"Wide End", "Narrow End", "Point", "Motion"});
        addParameter(pointX.set("Point.X", {0.5f}, {-FLT_MAX}, {FLT_MAX}));
        addParameter(pointY.set("Point.Y", {0.5f}, {-FLT_MAX}, {FLT_MAX}));
        addParameter(flip.set("Flip", false));
        addParameterDropdown(profile, "Profile", 0, {"Width", "Left", "Right", "Loop"});
        addParameterDropdown(shapeMode, "Shape", 0, {"Largest", "Index", "All"});
        addParameter(shapeIndex.set("Index", 0, 0, 1024));
        addParameter(normalize.set("Normalize", false));
        addParameter(gain.set("Gain", 1.f, 0.f, 100.f));
        addParameter(spineSmooth.set("Spine Smooth", 0.1f, 0.f, 1.f));
        addParameter(smooth.set("Smooth", 0.f, 0.f, 1.f));
        addInspectorParameter(hysteresis.set("Hysteresis", 0.1f, 0.f, 1.f));
        addOutputParameter(out.set("Out", {}, {-FLT_MAX}, {FLT_MAX}));
        addOutputParameter(length.set("Length", {}, {0.f}, {FLT_MAX}));
        addOutputParameter(headX.set("Head.X", {}, {-FLT_MAX}, {FLT_MAX}));
        addOutputParameter(headY.set("Head.Y", {}, {-FLT_MAX}, {FLT_MAX}));
        addOutputParameter(tailX.set("Tail.X", {}, {-FLT_MAX}, {FLT_MAX}));
        addOutputParameter(tailY.set("Tail.Y", {}, {-FLT_MAX}, {FLT_MAX}));
        addOutputParameter(spineX.set("Spine.X", {}, {-FLT_MAX}, {FLT_MAX}));
        addOutputParameter(spineY.set("Spine.Y", {}, {-FLT_MAX}, {FLT_MAX}));
        for(auto* p : {&xIn, &yIn, &pointX, &pointY})
            listeners.push(p->newListener([this](std::vector<float>&) { dirty = true; }));
        for(auto* p : {&resample, &size, &head, &profile, &shapeMode, &shapeIndex})
            listeners.push(p->newListener([this](int&) { dirty = true; }));
        for(auto* p : {&gain, &spineSmooth, &smooth, &hysteresis})
            listeners.push(p->newListener([this](float&) { dirty = true; }));
        for(auto* p : {&flip, &normalize})
            listeners.push(p->newListener([this](bool&) { dirty = true; }));
    }

    void update(ofEventArgs&) override {
        if(!dirty) return;
        dirty = false;
        contour1DCore::Controls c;
        c.resample = resample; c.size = size; c.head = head; c.flip = flip;
        c.headX = pointX.get(); c.headY = pointY.get();
        c.spineSmooth = spineSmooth; c.smooth = smooth; c.hysteresis = hysteresis;
        const auto r = contour1DCore::analyse(xIn.get(), yIn.get(), c, &tracks);
        if(!r.valid) { tracks.clear(); publish({}, {}, {}, {}, {}, {}, {}, {}); return; }
        tracks = r.tracks;

        std::vector<size_t> chosen;
        if(shapeMode == 0) {
            size_t best = 0; double area = -1;
            for(size_t s = 0; s < r.shapes.size(); ++s) if(r.shapes[s].area > area) { area = r.shapes[s].area; best = s; }
            if(!r.shapes.empty()) chosen.push_back(best);
        } else if(shapeMode == 1) {
            if(size_t(shapeIndex.get()) < r.shapes.size()) chosen.push_back(shapeIndex);
        } else {
            for(size_t s = 0; s < r.shapes.size(); ++s) chosen.push_back(s);
        }
        std::vector<float> o, len, hx, hy, tx, ty, sx, sy;
        for(size_t k = 0; k < chosen.size(); ++k) {
            if(k > 0) { o.push_back(-1); }
            append(o, r.shapes[chosen[k]]);
        }
        for(size_t s = 0; s < r.shapes.size(); ++s) {
            const auto& sh = r.shapes[s];
            len.push_back(float(sh.length));
            hx.push_back(float(sh.head.x)); hy.push_back(float(sh.head.y));
            tx.push_back(float(sh.tail.x)); ty.push_back(float(sh.tail.y));
            if(s > 0) { sx.push_back(-1); sy.push_back(-1); }
            for(const auto& p : sh.spine) { sx.push_back(float(p.x)); sy.push_back(float(p.y)); }
        }
        publish(o, len, hx, hy, tx, ty, sx, sy);
    }

    void presetRecallBeforeSettingParameters(ofJson&) override { tracks.clear(); dirty = true; }
    void presetHasLoaded() override { tracks.clear(); dirty = true; }
    void resetPhase() override { tracks.clear(); dirty = true; }

private:
    void append(std::vector<float>& o, const contour1DCore::Shape& s) {
        std::vector<float> v;
        const size_t n = s.left.size();
        switch(profile.get()) {
            case 1: for(size_t k = 0; k < n; ++k) v.push_back(float(s.left[k])); break;
            case 2: for(size_t k = 0; k < n; ++k) v.push_back(float(s.right[k])); break;
            case 3:
                for(size_t k = 0; k < n; ++k) v.push_back(float(s.left[k]));
                for(size_t k = n; k-- > 0;) v.push_back(float(s.right[k]));
                break;
            default: for(size_t k = 0; k < n; ++k) v.push_back(float(s.left[k] + s.right[k])); break;
        }
        float scale = gain;
        if(normalize) {
            float peak = 0;
            for(float f : v) peak = std::max(peak, f);
            if(peak > 0) scale /= peak;
        }
        for(float f : v) o.push_back(f * scale);
    }

    template<typename T> static void stage(ofParameter<std::vector<float>>& p, const T& v) {
        if(p.get() != v) p.set(v);
    }
    void publish(const std::vector<float>& o, const std::vector<float>& len,
                 const std::vector<float>& hx, const std::vector<float>& hy,
                 const std::vector<float>& tx, const std::vector<float>& ty,
                 const std::vector<float>& sx, const std::vector<float>& sy) {
        // Geometry first, Out last, so a listener on Out sees matching geometry.
        stage(spineY, sy); stage(spineX, sx);
        stage(tailY, ty); stage(tailX, tx);
        stage(headY, hy); stage(headX, hx);
        stage(length, len);
        stage(out, o);
    }

    ofParameter<std::vector<float>> xIn, yIn, pointX, pointY;
    ofParameter<std::vector<float>> out, length, headX, headY, tailX, tailY, spineX, spineY;
    ofParameter<int> resample, size, head, profile, shapeMode, shapeIndex;
    ofParameter<float> gain, spineSmooth, smooth, hysteresis;
    ofParameter<bool> flip, normalize;
    std::vector<contour1DCore::Track> tracks;
    ofEventListeners listeners;
    bool dirty = true;
};
