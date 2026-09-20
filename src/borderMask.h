//
//  borderMask.h
//  example
//
//  Created by Eduard Frigola Bagué on 11/02/2021.
//

#ifndef borderMask_h
#define borderMask_h

#include "ofxOceanodeNodeModel.h"
#include "ofxFatLine.h"

class borderMask : public ofxOceanodeNodeModel {
public:
    borderMask() : ofxOceanodeNodeModel("Border Mask"){}
    
    void setup(){
        addParameter(input.set("Input", {ofxFatLine()}));
        addParameter(affectAlpha.set("Alpha?", true));
        addParameter(maskPosition.set("Mask Pos", 0.0625, 0, 0.5)); // 0.0625 = 50/800 normalized
        addParameter(maskWidth.set("Mask Width", 0.025, 0, 0.5));  // 0.025 = 20/800 normalized
        addParameter(output.set("Output", {ofxFatLine()}));
        
        listener = input.newListener([this](vector<ofxFatLine> &lines){
            if(maskPosition!=0.0)
            {
                vector<ofxFatLine> outputLines;
                
                for(auto &line : lines){
                    ofxFatLine newLine;
                    const auto &vertices = line.getVertices();
                    vector<double> weights(vertices.size(), 1.0);
                    auto colors = line.getColors();
                    
                    for(size_t i = 0; i < vertices.size(); i++){
                        const glm::vec3 &vertex = vertices[i];
                        
                        // Keep everything in normalized space [0,1]
                        float normalizedX = vertex.x / 800.0f;
                        float normalizedY = vertex.y / 800.0f;
                        
                        // Calculate minimum distance to any border in normalized space
                        float distToLeft = normalizedX;
                        float distToRight = 1.0f - normalizedX;
                        float distToTop = normalizedY;
                        float distToBottom = 1.0f - normalizedY;
                        
                        float minDist = min(min(distToLeft, distToRight),
                                            min(distToTop, distToBottom));
                        
                        // Calculate mask factor using linear interpolation
                        float factor = 1.0f;
                        float innerEdge = maskPosition - maskWidth;
                        
                        if(minDist <= maskPosition) {
                            if(minDist <= innerEdge) {
                                factor = 0.0f; // Fully masked
                            } else {
                                // Linear interpolation between inner edge and mask position
                                factor = (minDist - innerEdge) / maskWidth;
                            }
                            
                            colors[i].r *= factor;
                            colors[i].g *= factor;
                            colors[i].b *= factor;
                            if(affectAlpha) {
                                colors[i].a *= factor;
                            }
                        }
                        
                        weights[i] = line.getWeight(i);
                    }
                    
                    newLine.add(vertices, colors, weights);
                    outputLines.push_back(newLine);
                }
                output = outputLines;
            }
        });
    }
    
private:
    ofParameter<vector<ofxFatLine>> input;
    ofParameter<float> maskPosition;
    ofParameter<float> maskWidth;
    ofParameter<bool> affectAlpha;
    ofParameter<vector<ofxFatLine>> output;
    
    ofEventListener listener;
};

#endif /* borderMask_h */
