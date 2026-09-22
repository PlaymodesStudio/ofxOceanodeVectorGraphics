//
//  removeZeros.h
//  example
//
//  Created by Eduard Frigola Bagué on 14/10/2020.
//

#ifndef removeZeros_h
#define removeZeros_h

#include "ofxOceanode.h"

class removeZeros : public ofxOceanodeNodeModel{
public:
    removeZeros() : ofxOceanodeNodeModel("Remove -1"){}
    
    void setup(){
        addParameter(input.set("Input", {0}, {0}, {1}));
        addOutputParameter(output.set("Output", {0}, {0}, {1}));
        
        listener = input.newListener([this](vector<float> &vf){
            tempOut = vf;
            tempOut.erase(std::remove(tempOut.begin(), tempOut.end(), -1), tempOut.end());
            output = tempOut;
        });
    }
    
private:
    ofParameter<vector<float>> input;
    ofParameter<vector<float>> output;
    ofEventListener listener;
    
    vector<float> tempOut;
};

#endif /* removeZeros_h */
