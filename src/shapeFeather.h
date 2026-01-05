//
//  shapeFeather.h
//  example
//
//  Created by Eduard Frigola Bagué on 11/02/2021.
//

#ifndef shapeFeather_h
#define shapeFeather_h

#include "ofxOceanodeNodeModel.h"

class shapeFeather : public ofxOceanodeNodeModel{
public:
    shapeFeather() : ofxOceanodeNodeModel("Shape Feather"){}
    
    void setup(){
        addParameter(x_in.set("X.In", {0.5}, {0}, {1}));
        addParameter(y_in.set("Y.In", {0.5}, {0}, {1}));
        addParameter(amount.set("Amount", 0, 0, 1));
        addOutputParameter(output.set("Feather", {0}, {0}, {1}));
        
        listener = x_in.newListener([this](vector<float> &vf){
            if(x_in->size() == y_in->size() && x_in->size() > 1) {
                auto iterbegin = x_in->begin();
                auto iterend = std::find(x_in->begin(), x_in->end(), -1);
                int i = 0;
                vector<float> out_tmp;
                while (iterend != x_in->end()){
                    int beginIndex = iterbegin - x_in->begin();
                    for(int j = 0; j < iterend - iterbegin; j++){
                        float x = x_in.get()[beginIndex+j];
                        float y = y_in.get()[beginIndex+j];
                        // Calculate the distance from the point to each of the four edges
                        double distToLeft = x;
                        double distToRight = 1 - x;
                        double distToTop = y;
                        double distToBottom = 1 - y;
                        
                        // Determine the minimum distance to the edge
                        double minDistToEdge = std::min({distToLeft, distToRight, distToTop, distToBottom});
                        
                        // If the point is within the feathering distance to an edge,
                        // calculate the feathering amount (output)
                        double output;
                        if (minDistToEdge <= amount) {
                            out_tmp.push_back(minDistToEdge / amount);
                        } else {
                            out_tmp.push_back(1.0);  // The point is not within the feathering distance to an edge
                        }
                        
                    }
                    iterbegin = iterend+1;
                    iterend = std::find(iterbegin, x_in->end(), -1);
                    i++;
                }
                int beginIndex = iterbegin - x_in->begin();
                for(int j = 0; j < iterend - iterbegin; j++){
                    float x = x_in.get()[beginIndex+j];
                    float y = y_in.get()[beginIndex+j];
                    // Calculate the distance from the point to each of the four edges
                    double distToLeft = x;
                    double distToRight = 1 - x;
                    double distToTop = y;
                    double distToBottom = 1 - y;
                    
                    // Determine the minimum distance to the edge
                    double minDistToEdge = std::min({distToLeft, distToRight, distToTop, distToBottom});
                    
                    // If the point is within the feathering distance to an edge,
                    // calculate the feathering amount (output)
                    double output;
                    if (minDistToEdge <= amount) {
                        out_tmp.push_back(minDistToEdge / amount);
                    } else {
                        out_tmp.push_back(1.0);  // The point is not within the feathering distance to an edge
                    }
                }
            
                output = out_tmp;
            }
        });
    }
    
private:
    ofParameter<vector<float>> x_in, y_in;
    ofParameter<float> amount;
    ofParameter<vector<float>> output;
    
    ofEventListener listener;
};

#endif /* shapeFeather_h */
