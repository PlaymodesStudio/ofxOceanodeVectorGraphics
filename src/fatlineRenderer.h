//
//  fatlineRenderer.h
//  example
//
//  Created by Eduard Frigola Bagué on 11/02/2021.
//

#ifndef fatlineRenderer_h
#define fatlineRenderer_h

#include "ofxOceanodeNodeModel.h"
#include "ofxFatLine.h"

class fatlineRenderer : public ofxOceanodeNodeModel{
public:
    fatlineRenderer() : ofxOceanodeNodeModel("Fat Line Renderer"){};
    
    void setup(){
        addParameter(input.set("Input", {ofxFatLine()}));
        addParameter(width.set("Width", 100, 1, 5000));
        addParameter(height.set("Height", 100, 1, 5000));
        addParameter(output.set("Output", nullptr));
        //screenSize = glm::vec2(100, 100);
    }
    
    void update(ofEventArgs &a){
        if(!fbo.isAllocated() || fbo.getWidth() != width || fbo.getHeight() != height){
            fbo.allocate(width, height, GL_RGB);
        }
    
        //ofxFatLine copy = input.get();
//        copy.scale(screenSize.x, screenSize.y);
//        copy.update();
        fbo.begin();
        ofEnableAlphaBlending();
        ofPushMatrix();
        //ofScale(screenSize.x, screenSize.y);
        ofClear(0);
//        ofSetColor(255, 0, 0);
//        ofDrawRectangle( 100, 100, 200, 200);
        
        for(auto l : input.get()){
            l.update();
            l.draw();
        }
        
       // copy.draw();
        ofDisableAlphaBlending();
        ofPopMatrix();
        fbo.end();
        
        output = &fbo.getTexture();
    }
    
    void draw(ofEventArgs &a){
//        if(ImGui::Begin(("Fat Line Renderer Window " + ofToString(getNumIdentifier())).c_str())){
//            screenSize = ImGui::GetContentRegionAvail();
//            ImTextureID textureID = (ImTextureID)(uintptr_t)fbo.getTexture().texData.textureID;//       GetImTextureID(fbo.getTexture());
//            ImGui::Image(textureID, screenSize);
//        }
//        ImGui::End();
    }
    
private:
    ofParameter<int> width, height;
    //glm::vec2 screenSize;
    ofFbo fbo;
    ofParameter<vector<ofxFatLine>> input;
    ofParameter<ofTexture*> output;
};

#endif /* fatlineRenderer_h */
