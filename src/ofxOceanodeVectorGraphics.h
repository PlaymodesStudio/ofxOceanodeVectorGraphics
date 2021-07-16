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
        addParameter(r.set("R", {0}, {0}, {1}));
        addParameter(g.set("G", {0}, {0}, {1}));
        addParameter(b.set("B", {0}, {0}, {1}));
        addParameter(a.set("A", {0}, {0}, {1}));
        
        addParameter(output.set("Output", {ofPolyline()}));
    }
    
    void update(ofEventArgs &args){
        bool close = false;
        vector<glm::vec3> points;
        if(x->size() == y->size()){
            points.resize(x->size());
            for(int i = 0; i < points.size(); i++){
                points[i] = glm::vec3(x.get()[i], y.get()[i], 0);
            }
        }else if(x->size() > y->size()){
            points.resize(x->size());
            for(int i = 0; i < points.size(); i++){
                points[i] = glm::vec3(x.get()[i], y.get()[0], 0);
            }
        }else if(x->size() < y->size()){
            points.resize(y->size());
            for(int i = 0; i < points.size(); i++){
                points[i] = glm::vec3(x.get()[0], y.get()[i], 0);
            }
        }
        		
		ofPolyline line(points);
        output.set(vector<ofPolyline>(1, line));
    }
    
private:
    ofParameter<vector<float>> x;
    ofParameter<vector<float>> y;
    ofParameter<vector<float>> w;
    ofParameter<vector<float>> r;
    ofParameter<vector<float>> g;
    ofParameter<vector<float>> b;
    ofParameter<vector<float>> a;
    ofParameter<vector<ofPolyline>> output;
};

class polylineRenderer : public ofxOceanodeNodeModel{
public:
    polylineRenderer() : ofxOceanodeNodeModel("Poly Line Renderer"){};
    
    void setup(){
        addParameter(input.set("Input", {ofPolyline()}));
        addParameter(width.set("Width", 100, 1, 5000));
        addParameter(height.set("Height", 100, 1, 5000));
        addParameter(output.set("Output", nullptr));
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
        
        for(auto &l : input.get()){
			l.draw();//0, 0, width, height);
        }
        
        ofDisableAlphaBlending();
        ofPopMatrix();
        fbo.end();
        
        output = &fbo.getTexture();
    }
    
private:
    ofParameter<int> width, height;
    ofFbo fbo;
    ofParameter<vector<ofPolyline>> input;
    ofParameter<ofTexture*> output;
};

namespace ofxOceanodeVectorGraphics{
static void registerModels(ofxOceanode &o){
    o.registerModel<fatlineGenerator>("Vector Graphics");
    o.registerModel<fatlineRenderer>("Vector Graphics");
    o.registerModel<fatlineFourier>("Vector Graphics");
    o.registerModel<svgModulator>("Vector Graphics");
	o.registerModel<polylineGenerator>("Vector Graphics");
	o.registerModel<polylineRenderer>("Vector Graphics");
	o.registerModel<manualPoints>("Vector Graphics");
	o.registerModel<pointsResampler>("Vector Graphics");
	o.registerModel<dotMaker>("Vector Graphics");
	o.registerModel<segmentsMaker>("Vector Graphics");
	o.registerModel<multiSegments>("Vector Graphics");
	o.registerModel<shapeTransform>("Vector Graphics");
}
static void registerType(ofxOceanode &o){
    o.registerType<ofPolyline>();//"Polyline");
    o.registerType<vector<ofPolyline>>();
    o.registerType<ofxFatLine>();
    o.registerType<vector<ofxFatLine>>();
}
static void registerScope(ofxOceanode &o){
    o.registerScope<ofPolyline>([](ofxOceanodeAbstractParameter *p, ImVec2 size){
        auto polylineVertices = p->cast<ofPolyline>().getParameter().get().getVertices();
        
        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        if(polylineVertices.size() > 1){
            for (int n = 0; n < polylineVertices.size() - 1; n++)
                draw_list->AddLine(ImVec2(origin.x + (polylineVertices[n].x * size.x), origin.y + (polylineVertices[n].y * size.y)), ImVec2(origin.x + (polylineVertices[n + 1].x * size.x), origin.y + (polylineVertices[n + 1].y * size.y)), IM_COL32(255, 255, 0, 255), 2.0f);
        }
    });
    o.registerScope<vector<ofPolyline>>([](ofxOceanodeAbstractParameter *p, ImVec2 size){
        auto polylines = p->cast<vector<ofPolyline>>().getParameter().get();
        
        for(auto p : polylines){
            auto polylineVertices = p.getVertices();
            ImVec2 origin = ImGui::GetCursorScreenPos();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            if(polylineVertices.size() > 1){
                for (int n = 0; n < polylineVertices.size() - 1; n++)
                    draw_list->AddLine(ImVec2(origin.x + (polylineVertices[n].x * size.x), origin.y + (polylineVertices[n].y * size.y)), ImVec2(origin.x + (polylineVertices[n + 1].x * size.x), origin.y + (polylineVertices[n + 1].y * size.y)), IM_COL32(255, 255, 0, 255), 2.0f);
            }
        }
    });
	o.registerScope<ofxFatLine>([](ofxOceanodeAbstractParameter *p, ImVec2 size){
        auto polylineVertices = p->cast<ofxFatLine>().getParameter().get().getVertices();
        
        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        if(polylineVertices.size() > 1){
            for (int n = 0; n < polylineVertices.size() - 1; n++)
                draw_list->AddLine(ImVec2(origin.x + (polylineVertices[n].x * size.x), origin.y + (polylineVertices[n].y * size.y)), ImVec2(origin.x + (polylineVertices[n + 1].x * size.x), origin.y + (polylineVertices[n + 1].y * size.y)), IM_COL32(255, 255, 0, 255), 2.0f);
        }
    });
    o.registerScope<vector<ofxFatLine>>([](ofxOceanodeAbstractParameter *p, ImVec2 size){
        auto polylines = p->cast<vector<ofxFatLine>>().getParameter().get();
		
		auto size2 = ImGui::GetContentRegionAvail();
        
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
                for (int n = 0; n < polylineVertices.size() - 1; n++)
					draw_list->AddLine(ImVec2(origin.x + (ofMap(polylineVertices[n].x, minSize.x, maxSize.x, 0, size2.x)), origin.y + (ofMap(polylineVertices[n].y, minSize.y, maxSize.y, 0, size2.y))), ImVec2(origin.x + (ofMap(polylineVertices[n+1].x, minSize.x, maxSize.x, 0, size2.x)), origin.y + (ofMap(polylineVertices[n+1].y, minSize.y, maxSize.y, 0, size2.y))), IM_COL32(colors[n].r*255, colors[n].g*255, colors[n].b*255, colors[n].a*255), p.getWeight(n));
            }
        }
    });
    o.registerScope<ofTexture*>([](ofxOceanodeAbstractParameter *p, ImVec2 size){
        auto tex = p->cast<ofTexture*>().getParameter().get();
        auto size2 = ImGui::GetContentRegionAvail();

        if(tex != nullptr){
            ImTextureID textureID = (ImTextureID)(uintptr_t)tex->texData.textureID;
            ImGui::Image(textureID, size2);
        }
    });
}
static void registerCollection(ofxOceanode &o){
    registerModels(o);
    registerType(o);
    registerScope(o);
}
}

#endif /* ofxOceanodeVectorGraphics_h */
