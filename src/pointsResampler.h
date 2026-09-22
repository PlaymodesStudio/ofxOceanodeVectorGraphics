//
//  pointsResampler.h
//  example
//
//  Created by Eduard Frigola Bagué on 11/02/2021.
//

#ifndef pointsResampler_h
#define pointsResampler_h

#include "ofxOceanodeNodeModel.h"

class pointsResampler : public ofxOceanodeNodeModel {
public:
    pointsResampler() : ofxOceanodeNodeModel("Points Resampler"){}
    
    void setup(){
        addParameter(x.set("X", {0.5}, {0}, {1}));
        addParameter(y.set("Y", {0.5}, {0}, {1}));
        addParameter(numPoints.set("Num", {100}, {1}, {INT_MAX}));
        addParameter(withSpacing.set("Use Spa", false));
        addParameter(spacing.set("Spacing", 0.1, 0, 1));
        addParameter(x_out.set("X_out", {0.5}, {0}, {1}));
        addParameter(y_out.set("Y_out", {0.5}, {0}, {1}));
        
        listeners.push(x.newListener([this](vector<float> &vf){
            if(vf.size() == 0)
            {
                x_out.set(vector<float>());
                y_out.set(vector<float>());
                return; // Exit
            }

            if(x->size() == y->size() && x->size() > 0){
                int numShapes = 1;
                for(int i = 0; i < x->size()-1; i++){ //Skip last in case is a -1
                    if(x->at(i) == -1){
                        numShapes++;
                    }
                }
                
                x_tmp.clear();
                y_tmp.clear();
                x_tmp.reserve((numPoints+1) * numShapes); //Add one for the -1
                y_tmp.reserve((numPoints+1) * numShapes);
                
                ofPolyline poly;
                for(int i = 0; i < x->size(); i++){
                    if(x->at(i) == -1){
                        if(withSpacing){
                            poly = poly.getResampledBySpacing(spacing);
                            for(int j = 0; j < poly.size(); j++){
                                x_tmp.push_back(poly[j].x);
                                y_tmp.push_back(poly[j].y);
                            }
                        }else{
                            //From: https://github.com/openframeworks/openFrameworks/issues/3557
                            float jump = 1.0f/ (numPoints-1);
                            for (int j = 0; j < numPoints; j++) {
                                auto p = poly.getPointAtPercent(min(j*jump, 0.99999f));
                                x_tmp.push_back(p.x);
                                y_tmp.push_back(p.y);
                            }
                        }
                        x_tmp.push_back(-1);
                        y_tmp.push_back(-1);
                        poly.clear();
                    }else{
                        poly.lineTo(x->at(i), y->at(i));
                    }
                }
                if(poly.size() != 0){
                    if(withSpacing){
                        poly = poly.getResampledBySpacing(spacing);
                        for(int j = 0; j < poly.size(); j++){
                            x_tmp.push_back(poly[j].x);
                            y_tmp.push_back(poly[j].y);
                        }
                    }else{
                        float jump = 1.0f/ (numPoints-1);
                        for (int j = 0; j < numPoints; j++) {
                            auto p = poly.getPointAtPercent(min(j*jump, 0.99999f));
                            x_tmp.push_back(p.x);
                            y_tmp.push_back(p.y);
                        }
                    }
                    x_tmp.push_back(-1);
                    y_tmp.push_back(-1);
                }
                y_out = y_tmp;
                x_out = x_tmp;
            }
        }));
        
        listeners.push(numPoints.newListener([this](int &i){
//            x_tmp.resize(i);
//            y_tmp.resize(i);
        }));
        
        x_tmp.resize(100);
        y_tmp.resize(100);
    }
    
private:
    ofParameter<vector<float>> x, y;
    vector<float> x_tmp, y_tmp;
    ofParameter<vector<float>> x_out, y_out;
    ofParameter<bool> withSpacing;
    ofParameter<float> spacing;
    ofParameter<int> numPoints;

    ofEventListeners listeners;
};

#endif /* pointsResampler_h */
