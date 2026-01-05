//
//  pointAtPercent.h
//  example
//
//  Created by Eduard Frigola Bagué on 11/02/2021.
//

#ifndef pointAtPercent_h
#define pointAtPercent_h

#include "ofxOceanodeNodeModel.h"

class pointAtPercent : public ofxOceanodeNodeModel{
public:
    pointAtPercent() : ofxOceanodeNodeModel("Point At Pct"){}
    
    void setup(){
        addParameter(x_in.set("X.In", {0.5}, {0}, {1}));
        addParameter(y_in.set("Y.In", {0.5}, {0}, {1}));
        addParameter(pcts.set("Pcts", {0}, {0}, {1}));
        addOutputParameter(x_out.set("X.Out", {0.5}, {0}, {1}));
        addOutputParameter(y_out.set("Y_Out", {0.5}, {0}, {1}));
        
        listener = pcts.newListener([this](vector<float> &vf){
            if(x_in->size() == y_in->size() && x_in->size() > 1) {
                auto iterbegin = x_in->begin();
                auto iterend = std::find(x_in->begin(), x_in->end(), -1);
                int i = 0;
                vector<float> x_tmp, y_tmp;
                while (iterend != x_in->end()){
                    ofPolyline poly;
                    int beginIndex = iterbegin - x_in->begin();
                    for(int j = 0; j < iterend - iterbegin; j++){
                        poly.addVertex(glm::vec3(x_in.get()[beginIndex+j], y_in.get()[beginIndex+j], 0));
                    }
                    iterbegin = iterend+1;
                    iterend = std::find(iterbegin, x_in->end(), -1);
                    
                    if(vf.size() > i && vf[i] >= 0 && vf[i] <= 1){
                        auto p = poly.getPointAtPercent(vf[i]);
                        x_tmp.push_back(p.x);
                        y_tmp.push_back(p.y);
                    }
                    i++;
                }
                ofPolyline poly;
                int beginIndex = iterbegin - x_in->begin();
                for(int j = 0; j < iterend - iterbegin; j++){
                    poly.addVertex(glm::vec3(x_in.get()[beginIndex+j], y_in.get()[beginIndex+j], 0));
                }
                if(vf.size() > i && vf[i] >= 0 && vf[i] <= 1){
                    auto p = poly.getPointAtPercent(vf[i]);
                    x_tmp.push_back(p.x);
                    y_tmp.push_back(p.y);
                }
                
                y_out = y_tmp;
                x_out = x_tmp;
            }
        });
    }
    
private:
    ofParameter<vector<float>> x_in, y_in, pcts, x_out, y_out;
    
    ofEventListener listener;
};

#endif /* pointAtPercent_h */
