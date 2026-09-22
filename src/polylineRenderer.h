//
//  polylineRenderer.h
//  example
//
//  Created by Eduard Frigola Bagué on 14/10/2020.
//

#ifndef polylineRenderer_h
#define polylineRenderer_h

#include "ofxOceanode.h"

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

#endif /* polylineRenderer_h */
