//
//  ofxOceanodeVectorGraphics.h
//  example
//
//  Created by Eduard Frigola Bagué on 14/10/2020.
//

#ifndef ofxOceanodeVectorGraphics_h
#define ofxOceanodeVectorGraphics_h

#include "fourierEpicyles.h"
#include "svgTools.h"
#include "fatlineTools.h"

#include "ofxOceanode.h"

class polylineGenerator : public ofxOceanodeNodeModel{
public:
    polylineGenerator() : ofxOceanodeNodeModel("Poly Line"){};
    
    void setup(){
        addParameter(x.set("X", {0}, {0}, {1}));
        addParameter(y.set("Y", {0}, {0}, {1}));
        addParameter(w.set("W", {0}, {0}, {20}));
        addParameter(r.set("R", {1}, {0}, {1}));
        addParameter(g.set("G", {1}, {0}, {1}));
        addParameter(b.set("B", {1}, {0}, {1}));
        addParameter(a.set("A", {1}, {0}, {1}));
		addParameter(fr.set("FillR", {0}, {0}, {1}));
        addParameter(fg.set("FillG", {0}, {0}, {1}));
        addParameter(fb.set("FillB", {0}, {0}, {1}));
        addParameter(fa.set("FillA", {0}, {0}, {1}));
        
        addParameter(output.set("Output", {ofPath()}));
    }
    
    void update(ofEventArgs &args){
        bool close = false;
        vector<vector<glm::vec3>> points;
		int index = 0;
		if(x->size() == y->size()){
			auto iterbegin = x->begin();
			auto iterend = std::find(x->begin(), x->end(), -1);
			int i = 0;
			while (iterend != x->end()){
				points.emplace_back();
				points[i].resize(iterend - iterbegin);
				int beginIndex = iterbegin - x->begin();
				for(int j = 0; j < points[i].size(); j++){
					points[i][j] = glm::vec3(x.get()[beginIndex+j], y.get()[beginIndex+j], 0);
				}
				iterbegin = iterend+1;
				iterend = std::find(iterbegin, x->end(), -1);
				i++;
			}
			points.emplace_back();
			points[i].resize(iterend - iterbegin);
			int beginIndex = iterbegin - x->begin();
			for(int j = 0; j < points[i].size(); j++){
				points[i][j] = glm::vec3(x.get()[beginIndex+j], y.get()[beginIndex+j], 0);
			}
		}else if(x->size() > y->size() && y->size() == 1){
			points.resize(1);
			points[0].resize(close ? x->size() + 1 : x->size());
			for(int i = 0; i < points[0].size(); i++){
				points[0][i] = glm::vec3(x.get()[i], y.get()[0], 0);
			}
		}else if(x->size() < y->size() && x->size() == 1){
			points.resize(1);
			points[0].resize(close ? y->size() + 1 : y->size());
			for(int i = 0; i < points[0].size(); i++){
				points[0][i] = glm::vec3(x.get()[0], y.get()[i], 0);
			}
        }else{
            return;
        }
		if(points.back().size() == 0) points.pop_back();
		
        auto getFromVec = [](const vector<float>& vf, int index) -> float{
            if (!vf.empty()) {
                if(index < vf.size()) return vf[index];
                return vf[0];
            }
            return 0.0f;
        };

		vector<ofPath> lines(points.size());
		
		for(int i = 0 ; i < points.size(); i++){
			lines[i].moveTo(points[i][0]);
			for(int j = 1; j < points[i].size(); j++){
				lines[i].lineTo(points[i][j]);
			}
			lines[i].setStrokeColor(ofFloatColor(getFromVec(r, i), getFromVec(g, i), getFromVec(b, i), getFromVec(a, i)));
			lines[i].setFillColor(ofFloatColor(getFromVec(fr, i), getFromVec(fg, i), getFromVec(fb, i), getFromVec(fa, i)));
			lines[i].setStrokeWidth(getFromVec(w, i));
			
			//lines[i].
		}
        output.set(lines);
    }
    
private:
    ofParameter<vector<float>> x;
    ofParameter<vector<float>> y;
    ofParameter<vector<float>> w;
    ofParameter<vector<float>> r;
    ofParameter<vector<float>> g;
    ofParameter<vector<float>> b;
    ofParameter<vector<float>> a;
    ofParameter<vector<float>> fr;
    ofParameter<vector<float>> fg;
    ofParameter<vector<float>> fb;
    ofParameter<vector<float>> fa;
    ofParameter<vector<ofPath>> output;
};

class polylineRenderer : public ofxOceanodeNodeModel{
public:
    polylineRenderer() : ofxOceanodeNodeModel("Poly Line Renderer"){};
    
    void setup(){
        auto inputref = addParameter(input.set("Input", {ofPath()}));
        addParameter(width.set("Width", 100, 1, INT_MAX));
        addParameter(height.set("Height", 100, 1, INT_MAX));
        addParameter(texture.set("Tex", nullptr));
        addParameter(output.set("Output", nullptr));
        
        inputref->addReceiveFunc<vector<ofxFatLine>>([this](vector<ofxFatLine> const &fl){
            inputFat.clear();
            for(auto &f : fl){
                if(f.getVertices().size() > 1){
                    ofPath p;
                    p.moveTo(f.getVertices()[0] / glm::vec3(800, 800, 1));
                    for(int i = 1 ; i < f.getVertices().size(); i++){
                        p.lineTo(f.getVertices()[i] / glm::vec3(800, 800, 1));
                    }
                    p.setStrokeColor(ofFloatColor(1, 1, 1, 1));
                    p.setFilled(false);
                    p.setStrokeWidth(1);
                    inputFat.push_back(p);
                }
            }
        });
        inputref->addDisconnectFunc([this](){
            inputFat.clear();
        });
    }
    
    void update(ofEventArgs &a){
        if(!fbo.isAllocated() || fbo.getWidth() != width || fbo.getHeight() != height){
            fbo.allocate(width, height, GL_RGB);
        }

        fbo.begin();
        ofEnableAlphaBlending();
        ofPushMatrix();
        ofClear(0);
		ofScale(width, height);
        
        const vector<ofPath> &toDraw = inputFat.size() > 0 ? inputFat : input.get();
        
        if(texture.get() != nullptr){
            texture.get()->bind();
            for(auto &l : toDraw){
                ofMesh mesh = l.getTessellation();
                for(auto & v: mesh.getVertices()){
                    mesh.addTexCoord(v);//glm::vec2(v.x * texture.get()->getWidth(), v.y * texture.get()->getHeight()));
                }
                mesh.draw();
            }
            texture.get()->unbind();
        }else{
            for(auto &l : toDraw){
                l.draw();//0, 0, width, height);
            }
        }
        
        inputFat.clear();
        
        ofDisableAlphaBlending();
        ofPopMatrix();
        fbo.end();
        
        output = &fbo.getTexture();
    }
    
private:
    ofParameter<int> width, height;
    ofParameter<ofTexture*> texture;
    ofFbo fbo;
    ofParameter<vector<ofPath>> input;
    vector<ofPath> inputFat;
    ofParameter<ofTexture*> output;
};

class removeZeros : public ofxOceanodeNodeModel{
public:
    removeZeros() : ofxOceanodeNodeModel("Remove Zeros"){}
    
    void setup(){
        addParameter(input.set("Input", {0}, {0}, {1}));
        addOutputParameter(output.set("Output", {0}, {0}, {1}));
        
        listener = input.newListener([this](vector<float> &vf){
            tempOut = vf;
            tempOut.erase(std::remove(tempOut.begin(), tempOut.end(), -1), tempOut.end());
            output = tempOut;
        });
    }
    
private:
    ofParameter<vector<float>> input;
    ofParameter<vector<float>> output;
    ofEventListener listener;
    
    vector<float> tempOut;
};

namespace ofxOceanodeVectorGraphics{
static void registerModels(ofxOceanode &o){
    o.registerModel<fatlineGenerator>("Vector Graphics");
    o.registerModel<fatlineRenderer>("Vector Graphics");
    o.registerModel<fourierShape>("Vector Graphics");
	o.registerModel<inverseFourierShape>("Vector Graphics");
    o.registerModel<svgModulator>("Vector Graphics");
	o.registerModel<polylineGenerator>("Vector Graphics");
	o.registerModel<polylineRenderer>("Vector Graphics");
	o.registerModel<manualPoints>("Vector Graphics");
	o.registerModel<pointsResampler>("Vector Graphics");
    o.registerModel<pointsResamplerIndex>("Vector Graphics");
	o.registerModel<dotMaker>("Vector Graphics");
	o.registerModel<segmentsMaker>("Vector Graphics");
	o.registerModel<multiSegments>("Vector Graphics");
	o.registerModel<shapeTransform>("Vector Graphics");
	o.registerModel<polygonCreator>("Vector Graphics");
	o.registerModel<pointMatrix>("Vector Graphics");
    o.registerModel<removeZeros>("Vector Graphics");
    o.registerModel<bezierCurve>("Vector Graphics");
    o.registerModel<radialModulation>("Vector Graphics");
    o.registerModel<shapeInterpolator>("Vector Graphics");
    o.registerModel<pointAtPercent>("Vector Graphics");
    o.registerModel<shapeFeather>("Vector Graphics");
    o.registerModel<shapeLength>("Vector Graphics");
    o.registerModel<borderMask>("Border Mask");
}
static void registerType(ofxOceanode &o){
    o.registerType<ofPolyline>("Polyline");
    o.registerType<vector<ofPath>>("v_Poly");
    o.registerType<ofxFatLine>("Fatline");
    o.registerType<vector<ofxFatLine>>("v_Fatline");
#ifndef ofxOceanodeTextures_h
    o.registerType<ofTexture*>("Texture");
#endif
}
static void registerScope(ofxOceanode &o){
    o.registerScope<ofPolyline>([](ofxOceanodeAbstractParameter *p, ImVec2 size){
        auto size2 = ImGui::GetContentRegionAvail();
        bool keepAspectRatio = (p->getFlags() & ofxOceanodeParameterFlags_ScopeKeepAspectRatio);
        if(keepAspectRatio)
        {
            if(size.y>size.x) size2.y = size2.x;
            else size2.x = size.y;
        }

        auto polylineVertices = p->cast<ofPolyline>().getParameter().get().getVertices();
        
        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        if(polylineVertices.size() > 1){
            for (int n = 0; n < polylineVertices.size() - 1; n++)
                draw_list->AddLine(ImVec2(origin.x + (polylineVertices[n].x * size2.x), origin.y + (polylineVertices[n].y * size2.y)), ImVec2(origin.x + (polylineVertices[n + 1].x * size2.x), origin.y + (polylineVertices[n + 1].y * size2.y)), IM_COL32(255, 255, 0, 255), 2.0f);
        }
    });
    o.registerScope<vector<ofPath>>([](ofxOceanodeAbstractParameter *p, ImVec2 size){
        auto polylines = p->cast<vector<ofPath>>().getParameter().get();
        auto size2 = ImGui::GetContentRegionAvail();
        bool keepAspectRatio = (p->getFlags() & ofxOceanodeParameterFlags_ScopeKeepAspectRatio);
        if(keepAspectRatio)
        {
            if(size.y>size.x) size2.y = size2.x;
            else size2.x = size.y;
        }

        for(auto p : polylines){
            auto polylineVertices = p.getOutline()[0].getVertices();
            ImVec2 origin = ImGui::GetCursorScreenPos();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            if(polylineVertices.size() > 1){
                for (int n = 0; n < polylineVertices.size() - 1; n++)
                    draw_list->AddLine(ImVec2(origin.x + (polylineVertices[n].x * size2.x), origin.y + (polylineVertices[n].y * size2.y)), ImVec2(origin.x + (polylineVertices[n + 1].x * size2.x), origin.y + (polylineVertices[n + 1].y * size2.y)), IM_COL32(p.getStrokeColor().r, p.getStrokeColor().g, p.getStrokeColor().b, p.getStrokeColor().a), p.getStrokeWidth());
            }
        }
    });
	o.registerScope<ofxFatLine>([](ofxOceanodeAbstractParameter *p, ImVec2 size)
    {
        auto polylineVertices = p->cast<ofxFatLine>().getParameter().get().getVertices();
        auto size2 = ImGui::GetContentRegionAvail();
        bool keepAspectRatio = (p->getFlags() & ofxOceanodeParameterFlags_ScopeKeepAspectRatio);
        if(keepAspectRatio)
        {
            if(size.y>size.x) size2.y = size2.x;
            else size2.x = size.y;
        }

        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        if(polylineVertices.size() > 1){
            for (int n = 0; n < polylineVertices.size() - 1; n++)
                draw_list->AddLine(ImVec2(origin.x + (polylineVertices[n].x * size2.x), origin.y + (polylineVertices[n].y * size2.y)), ImVec2(origin.x + (polylineVertices[n + 1].x * size2.x), origin.y + (polylineVertices[n + 1].y * size2.y)), IM_COL32(255, 255, 0, 255), 2.0f);
        }
    });
    o.registerScope<vector<ofxFatLine>>([](ofxOceanodeAbstractParameter *p, ImVec2 size){
        auto polylines = p->cast<vector<ofxFatLine>>().getParameter().get();
		auto size2 = ImGui::GetContentRegionAvail();
        bool keepAspectRatio = (p->getFlags() & ofxOceanodeParameterFlags_ScopeKeepAspectRatio);
        if(keepAspectRatio)
        {
            if(size.y>size.x) size2.y = size2.x;
            else size2.x = size.y;
        }
        for(auto p : polylines){
            auto polylineVertices = p.getVertices();
			auto colors = p.getColors();
			glm::vec2 minSize = glm::vec2(0,0);
			glm::vec2 maxSize = glm::vec2(800,800);
			
			
//			glm::vec2 minSize = glm::vec2(10000,10000);
//			glm::vec2 maxSize = glm::vec2(0,0);
//			for(auto p_ : polylineVertices){
//				if(p_.x < minSize.x) minSize.x = p_.x;
//				if(p_.y < minSize.y) minSize.y = p_.y;
//
//				if(p_.x > maxSize.x) maxSize.x = p_.x;
//				if(p_.y > maxSize.y) maxSize.y = p_.y;
//			}
			
//			minSize.x *= size.x;
//			minSize.y *= size.y;
//			maxSize.x *= size.x;
//			maxSize.y *= size.y;
			
            ImVec2 origin = ImGui::GetCursorScreenPos();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            if(polylineVertices.size() > 1){
                for (int n = 0; n < polylineVertices.size() - 1; n++){
                    glm::vec2 middlePoint = glm::mix(glm::vec2(polylineVertices[n].x, polylineVertices[n].y), glm::vec2(polylineVertices[n+1].x, polylineVertices[n+1].y), 0.5f);
                    draw_list->AddLine(ImVec2(origin.x + (ofMap(polylineVertices[n].x, minSize.x, maxSize.x, 0, size2.x)), origin.y + (ofMap(polylineVertices[n].y, minSize.y, maxSize.y, 0, size2.y))), ImVec2(origin.x + (ofMap(middlePoint.x, minSize.x, maxSize.x, 0, size2.x)), origin.y + (ofMap(middlePoint.y, minSize.y, maxSize.y, 0, size2.y))), IM_COL32(colors[n].r*255, colors[n].g*255, colors[n].b*255, colors[n].a*255), p.getWeight(n));
                    draw_list->AddLine(ImVec2(origin.x + (ofMap(middlePoint.x, minSize.x, maxSize.x, 0, size2.x)), origin.y + (ofMap(middlePoint.y, minSize.y, maxSize.y, 0, size2.y))), ImVec2(origin.x + (ofMap(polylineVertices[n+1].x, minSize.x, maxSize.x, 0, size2.x)), origin.y + (ofMap(polylineVertices[n+1].y, minSize.y, maxSize.y, 0, size2.y))), IM_COL32(colors[n+1].r*255, colors[n+1].g*255, colors[n+1].b*255, colors[n+1].a*255), p.getWeight(n+1));
                }
            }
            else if(polylineVertices.size()==1)
            {
                //draw_list->AddCircle(ImVec2( X , Y,  RADIUS , ImGui::GetColorU32(ImGuiCol_Text));
                draw_list->AddCircleFilled(ImVec2(origin.x + (ofMap(polylineVertices[0].x, minSize.x, maxSize.x, 0, size2.x)),
                                     origin.y + (ofMap(polylineVertices[0].y, minSize.y, maxSize.y, 0, size2.y))),
                                    5,
                                    IM_COL32(colors[0].r*255, colors[0].g*255, colors[0].b*255, colors[0].a*255), 4);
            }

        }
    });
#ifndef ofxOceanodeTextures_h
    o.registerScope<ofTexture*>([](ofxOceanodeAbstractParameter *p, ImVec2 size){

        auto tex = p->cast<ofTexture*>().getParameter().get();
        auto size2 = ImGui::GetContentRegionAvail();
        bool keepAspectRatio = (p->getFlags() & ofxOceanodeParameterFlags_ScopeKeepAspectRatio);
        float sizeAspectRatio=size.x/size.y;
        float texAspectRatio;
        if(tex != nullptr){
            texAspectRatio = tex->getWidth() / tex->getHeight();
        }
        if(keepAspectRatio)
        {
            if(sizeAspectRatio<texAspectRatio)
            {
                size2.y = size2.x / texAspectRatio;
                size2.x = size.x;
            }
            else
            {
                size2.x = size2.y * texAspectRatio;
                size2.y = size.y;
            }
        }
        
        if(tex != nullptr){
            ImTextureID textureID = (ImTextureID)(uintptr_t)tex->texData.textureID;
            ImGui::Image(textureID, size2);
        }
    });
#endif
}
static void registerCollection(ofxOceanode &o){
    registerModels(o);
    registerType(o);
    registerScope(o);
}
}

#endif /* ofxOceanodeVectorGraphics_h */
