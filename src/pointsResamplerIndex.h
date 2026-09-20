//
//  pointsResamplerIndex.h
//  example
//
//  Created by Eduard Frigola Bagué on 11/02/2021.
//

#ifndef pointsResamplerIndex_h
#define pointsResamplerIndex_h

#include "ofxOceanodeNodeModel.h"

class pointsResamplerIndex : public ofxOceanodeNodeModel {
public:
    pointsResamplerIndex() : ofxOceanodeNodeModel("Points Resampler Index"){}
    
    void setup(){
        addParameter(x.set("X", {0.5}, {0}, {1}));
        addParameter(y.set("Y", {0.5}, {0}, {1}));
        addParameter(numPoints.set("Num", {100}, {1}, {INT_MAX}));
        addParameter(withSpacing.set("Use Spa", false));
        addParameter(spacing.set("Spacing", 0.1, 0, 1));
        addParameter(x_out.set("X_out", {0.5}, {0}, {1}));
        addParameter(y_out.set("Y_out", {0.5}, {0}, {1}));
        addParameter(index_out.set("Index_out", {0}, {0}, {INT_MAX}));
        
        listeners.push(x.newListener([this](vector<float> &vf){
            if(x->size() == y->size()){
                int numSegments = x->size()/2;

                // Compute the length of each segment
                vector<float> lengths(numSegments);
                float totalLength = 0;
                for(int i = 0; i < numSegments; i++){
                    float dx = x->at(2*i+1) - x->at(2*i);
                    float dy = y->at(2*i+1) - y->at(2*i);
                    lengths[i] = sqrt(dx*dx + dy*dy);
                    totalLength += lengths[i];
                }
                
                x_tmp.clear();
                y_tmp.clear();
                index_tmp.clear();
                
                ofPolyline poly;
                for(int i = 0; i < numSegments; i++){
                    poly.lineTo(x->at(2*i), y->at(2*i));
                    poly.lineTo(x->at(2*i+1), y->at(2*i+1));
                    
                    // Determine the number of points for this segment
                    int segmentPoints = round(numPoints * lengths[i] / totalLength);
                    if(withSpacing){
                        poly = poly.getResampledBySpacing(spacing);
                    }else{
                        poly = poly.getResampledByCount(segmentPoints);
                    }
                    
                    for(auto & point : poly){
                        x_tmp.push_back(point.x);
                        y_tmp.push_back(point.y);
                        index_tmp.push_back(i);
                    }
                    poly.clear();
                }
                x_out = x_tmp;
                y_out = y_tmp;
                index_out = index_tmp;
            }
        }));
        
        x_tmp.resize(100);
        y_tmp.resize(100);
        index_tmp.resize(100);
    }
    
private:
    ofParameter<vector<float>> x, y;
    vector<float> x_tmp, y_tmp, index_tmp;
    ofParameter<vector<float>> x_out, y_out, index_out;
    ofParameter<bool> withSpacing;
    ofParameter<float> spacing;
    ofParameter<int> numPoints;

    ofEventListeners listeners;
};

#endif /* pointsResamplerIndex_h */
