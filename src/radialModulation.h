//
//  radialModulation.h
//  example
//
//  Created by Eduard Frigola Bagué on 11/02/2021.
//

#ifndef radialModulation_h
#define radialModulation_h

#define M_PI 3.14159265358979323846264338327950288

#include "ofxOceanodeNodeModel.h"

class radialModulation : public ofxOceanodeNodeModel{
public:
    radialModulation() : ofxOceanodeNodeModel("Radial Modulation"){}
    
    void setup(){
        addParameter(xIn.set("X.In", {0.5}, {0}, {1}));
        addParameter(yIn.set("Y.In", {0.5}, {0}, {1}));
        addParameter(modulator.set("Modulator", {0}, {0}, {1}));
        addParameter(amount.set("Amnt", {0}, {0}, {1}));
        addParameter(xCenter.set("X.Center", {0.5}, {0}, {1}));
        addParameter(yCenter.set("Y.Center", {0.5}, {0}, {1}));
        addParameter(xOut.set("X.Out", {0.5}, {0}, {1}));
        addParameter(yOut.set("Y.Out", {0.5}, {0}, {1}));
        
        addInspectorParameter(modulateType.set("Type ? radial : rotational", false));
        
        listeners.push(xIn.newListener([this](vector<float> &vf){
            auto getValueForIndex = [](const vector<float> &vf, int i, int currentShape, int numShapes) -> float{
                if(vf.size() == numShapes){
                    return vf[currentShape];
                }else if(i < vf.size()){
                    return vf[i];
                }else{
                    return vf[0];
                }
            };
            
            
            if(xIn->size() == yIn->size()){
                int numShapes = 1;
                for(int i = 0; i < xIn->size()-1; i++){ //Skip last in case is a -1
                    if(xIn->at(i) == -1){
                        numShapes++;
                    }
                }

                x_tmp = xIn;
                y_tmp = yIn;
                int currentShape = 0;
                int realI = 0;
                for(int i = 0; i < xIn->size(); i++){
                    if(xIn->at(i) != -1){
                        float modulationValue = getValueForIndex(modulator, realI, currentShape, numShapes);
                        float amountValue = getValueForIndex(amount, realI, currentShape, numShapes);
                        glm::vec2 center(getValueForIndex(xCenter.get(), realI, currentShape, numShapes), getValueForIndex(yCenter.get(), realI, currentShape, numShapes));
                        if(modulateType){
                            x_tmp[i] -= center.x;
                            y_tmp[i] -= center.y;
                            float x_copy = x_tmp[i];
                            x_tmp[i] = x_copy * cos((modulationValue-0.5) * amountValue * M_PI*2) - y_tmp[i] * sin((modulationValue-0.5) * amountValue * M_PI*2);
                            y_tmp[i] = x_copy * sin((modulationValue-0.5) * amountValue * M_PI*2) + y_tmp[i] * cos((modulationValue-0.5) * amountValue * M_PI*2);
                            x_tmp[i] += center.x;
                            y_tmp[i] += center.y;
                        }else{
                            glm::vec2 point(x_tmp[i], y_tmp[i]);
                            if(!(point.x == center.x && point.y == center.y)){
                                point = point + (glm::normalize(point - center)) * (modulationValue-0.5) * amountValue;
                            }
                            x_tmp[i] = point.x;
                            y_tmp[i] = point.y;
                        }
                        realI++;
                    }else{
                        currentShape++;
                    }
                }
                yOut = y_tmp;
                xOut = x_tmp;
            }
        }));
    }
    
    void presetHasLoaded(){
        xIn = xIn;
    }
    
private:
    ofEventListeners listeners;
    ofParameter<vector<float>> xIn, yIn, xOut, yOut, amount, xCenter, yCenter, modulator;
    ofParameter<bool> modulateType;
    vector<float> x_tmp, y_tmp;
};

#endif /* radialModulation_h */
