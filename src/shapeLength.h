//
//  shapeLength.h
//  example
//
//  Created by Eduard Frigola Bagué on 11/02/2021.
//

#ifndef shapeLength_h
#define shapeLength_h

#include "ofxOceanodeNodeModel.h"

class shapeLength : public ofxOceanodeNodeModel{
public:
    shapeLength() : ofxOceanodeNodeModel("Shape Length"){}
    
    void setup(){
        addParameter(x_in.set("X.In", {0.5}, {0}, {1}));
        addParameter(y_in.set("Y.In", {0.5}, {0}, {1}));
        addOutputParameter(length.set("Length", {0}, {0}, {FLT_MAX}));
        
        listener = x_in.newListener([this](vector<float> &vf){
            if(x_in->size() == y_in->size() && x_in->size() > 1) {
                auto iterbegin = x_in->begin();
                auto iterend = std::find(x_in->begin(), x_in->end(), -1);
                int i = 0;
                vector<float> length_tmp;
                while (iterend != x_in->end()){
                    int beginIndex = iterbegin - x_in->begin();
                    float dist = 0;
                    for(int j = 0; j < iterend - iterbegin - 1; j++){
                        float x0 = x_in.get()[beginIndex+j];
                        float y0 = y_in.get()[beginIndex+j];
                        float x1 = x_in.get()[beginIndex+j+1];
                        float y1 = y_in.get()[beginIndex+j+1];
                        dist += glm::distance(glm::vec2(x0, y0), glm::vec2(x1, y1));
                    }
                    length_tmp.push_back(dist);
                    iterbegin = iterend+1;
                    iterend = std::find(iterbegin, x_in->end(), -1);
                    i++;
                }
                int beginIndex = iterbegin - x_in->begin();
                float dist = 0;
                for(int j = 0; j < iterend - iterbegin; j++){
                    float x0 = x_in.get()[beginIndex+j];
                    float y0 = y_in.get()[beginIndex+j];
                    float x1 = x_in.get()[beginIndex+j+1];
                    float y1 = y_in.get()[beginIndex+j+1];
                    dist += glm::distance(glm::vec2(x0, y0), glm::vec2(x1, y1));
                }
                if(dist > 0)
                    length_tmp.push_back(dist);
                length = length_tmp;
            }
        });
    }
    
private:
    ofParameter<vector<float>> x_in, y_in;
    ofParameter<vector<float>> length;
    
    ofEventListener listener;
};

#endif /* shapeLength_h */
