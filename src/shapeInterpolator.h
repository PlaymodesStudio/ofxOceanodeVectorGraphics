//
//  shapeInterpolator.h
//  example
//
//  Created by Eduard Frigola Bagué on 11/02/2021.
//

#ifndef shapeInterpolator_h
#define shapeInterpolator_h

#include "ofxOceanodeNodeModel.h"

class shapeInterpolator : public ofxOceanodeNodeModel{
public:
    shapeInterpolator() : ofxOceanodeNodeModel("Shape Interp"){}
    
    void setup(){
        addParameter(xA.set("X.a", {0.5}, {0}, {1}));
        addParameter(yA.set("Y.a", {0.5}, {0}, {1}));
        addParameter(xB.set("X.b", {0.5}, {0}, {1}));
        addParameter(yB.set("Y.b", {0.5}, {0}, {1}));
        addParameter(interp.set("Interp", 0, 0, 1));
        addParameter(x_out.set("X_out", {0.5}, {0}, {1}));
        addParameter(y_out.set("Y_out", {0.5}, {0}, {1}));
        
        listeners.push(xA.newListener([this](vector<float> &vf){
            if(xA->size() > 1 && xB->size() > 1 && xA->size() == yA->size() && xB->size() == yB->size()){
//                int numShapes = 1;
//                for(int i = 0; i < x->size()-1; i++){ //Skip last in case is a -1
//                    if(x->at(i) == -1){
//                        numShapes++;
//                    }
//                }
//
//                x_tmp.clear();
//                y_tmp.clear();
//                x_tmp.reserve((numPoints+1) * numShapes); //Add one for the -1
//                y_tmp.reserve((numPoints+1) * numShapes);
//
//                ofPolyline poly;
//                for(int i = 0; i < x->size(); i++){
//                    if(x->at(i) == -1){
//                        if(withSpacing){
//                            poly = poly.getResampledBySpacing(spacing);
//                        }else{
//                            poly = poly.getResampledByCount(numPoints);
//                        }
//                        for(int j = 0; j < poly.size(); j++){
//                            x_tmp.push_back(poly[j].x);
//                            y_tmp.push_back(poly[j].y);
//                        }
//                        x_tmp.push_back(-1);
//                        y_tmp.push_back(-1);
//                        poly.clear();
//                    }else{
//                        poly.lineTo(x->at(i), y->at(i));
//                    }
//                }
//                if(poly.size() != 0){
//                    if(withSpacing){
//                        poly = poly.getResampledBySpacing(spacing);
//                    }else{
//                        poly = poly.getResampledByCount(numPoints);
//                    }
//                    for(int j = 0; j < poly.size(); j++){
//                        x_tmp.push_back(poly[j].x);
//                        y_tmp.push_back(poly[j].y);
//                    }
//                }
//                y_out = y_tmp;
//                x_out = x_tmp;
                //https://github.com/veltman/flubber/blob/0cadadf3eb15cd5b2ec7c45c70601ff88b76a6c3/build/flubber.js#L2504
                auto addPoint = [](vector<float> &x, vector<float> &y, int iter){
                    glm::vec2 p1(x[iter*2], y[iter*2]);
                    glm::vec2 p2(x[(iter*2) + 1], y[(iter*2) + 1]);
                    glm::vec2 pout = glm::mix(p1, p2, 0.5);
                    x.insert(x.begin() + (iter*2) + 1, pout.x);
                    y.insert(y.begin() + (iter*2) + 1, pout.y);
                };
                
                x_tmp.resize(max(xA->size(), xB->size()));
                y_tmp.resize(max(xA->size(), xB->size()));
                
                xA_tmp = xA;
                yA_tmp = yA;
                xB_tmp = xB;
                yB_tmp = yB;
                if(xA_tmp.size() < xB_tmp.size()){
                    int iter = 0;
                    while(xA_tmp.size() != xB_tmp.size()){
                        addPoint(xA_tmp, yA_tmp, iter);
                        iter++;
                    }
                }else if(xA_tmp.size() > xB_tmp.size()){
                    int iter = 0;
                    while(xA_tmp.size() != xB_tmp.size()){
                        addPoint(xB_tmp, yB_tmp, iter);
                        iter++;
                    }
                }
                
                if(xA_tmp.size() == xB_tmp.size()){
//                    ofPolyline poly;
//                    for(int i = 0; i < xB->size(); i++){
//                        poly.lineTo(xB->at(i), yB->at(i));
//                    }
//                    for(int i = 0; i < xA->size(); i++){
//                        glm::vec3 pA = glm::vec3(xA->at(i), yA->at(i), 0);
//                        glm::vec3 pB = *std::max_element(poly.getVertices().begin(), poly.getVertices().end(), [pA](glm::vec3 &first, glm::vec3 &second){
//                            return glm::distance(pA, first) > glm::distance(pA, second);
//                        });
//
//                        x_tmp[i] = ofLerp(pA.x, pB.x, interp);
//                        y_tmp[i] = ofLerp(pA.y, pB.y, interp);
//                    }
                    
                    // Find the best rotation of A to go to B
                    // Code from https://github.com/veltman/flubber/blob/0cadadf3eb15cd5b2ec7c45c70601ff88b76a6c3/build/flubber.js#L2600
                    float sumOfSquares = 0;
                    float min = FLT_MAX;
                    int bestOffset = 0;
                    bool aClosed = xA_tmp.front() == xA_tmp.back() && yA_tmp.front() == yA_tmp.back();
                    bool bClosed = xB_tmp.front() == xB_tmp.back() && yB_tmp.front() == yB_tmp.back();
                    bool bothClosed = aClosed && bClosed;
                    
                    int xA_size = xA_tmp.size() - (bothClosed ? 1 : 0);
                    int xB_size = xB_tmp.size() - (bothClosed ? 1 : 0);
                    if(aClosed || bClosed){
                        for(int i = 0; i < xA_size; i++){
                            sumOfSquares = 0;
                            for(int j = 0; j < xB_size; j++){
                                glm::vec3 pA = glm::vec3(xA_tmp[(i + j) % xA_size], yA_tmp[(i + j) % xA_size], 0);
                                glm::vec3 pB = glm::vec3(xB_tmp[j], yB_tmp[j], 0);
                                float d = glm::distance(pA, pB);
                                sumOfSquares += (d * d);
                            }
                            
                            if (sumOfSquares < min) {
                                min = sumOfSquares;
                                bestOffset = i;
                            }
                        }
                    }

                    for(int i = 0; i < xA_size; i++){
                        int ii = (i + bestOffset) % xA_size;
                        glm::vec3 pA = glm::vec3(xA_tmp[ii], yA_tmp[ii], 0);
                        glm::vec3 pB = glm::vec3(xB_tmp[i], yB_tmp[i], 0);
                        if(bClosed){
                            x_tmp[i] = ofLerp(pA.x, pB.x, interp);
                            y_tmp[i] = ofLerp(pA.y, pB.y, interp);
                        }else{
                            x_tmp[ii] = ofLerp(pA.x, pB.x, interp);
                            y_tmp[ii] = ofLerp(pA.y, pB.y, interp);
                        }
                    }
                    if(bothClosed){
                        x_tmp.back() = x_tmp.front();
                        y_tmp.back() = y_tmp.front();
                    }
                }
                y_out = y_tmp;
                x_out = x_tmp;
            }
        }));
        
//        x_tmp.resize(100);
//        y_tmp.resize(100);
        //        getClosestPoint();
    }
    
private:
    ofParameter<vector<float>> xA, yA, xB, yB;
    vector<float> xA_tmp, yA_tmp, xB_tmp, yB_tmp, x_tmp, y_tmp;
    ofParameter<vector<float>> x_out, y_out;
    ofParameter<float> interp;

    ofEventListeners listeners;
};

#endif /* shapeInterpolator_h */
