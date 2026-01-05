//
//  pointMatrix.h
//  example
//
//  Created by Eduard Frigola Bagué on 11/02/2021.
//

#ifndef pointMatrix_h
#define pointMatrix_h

#include "ofxOceanodeNodeModel.h"

class pointMatrix : public ofxOceanodeNodeModel{
public:
    pointMatrix() : ofxOceanodeNodeModel("Point Matrix"){};
    ~pointMatrix(){};
    
    void setup() override{
        addParameter(sizeX.set("width", 10, 1, 1000));
        addParameter(sizeY.set("height", 10, 1, 1000));
		addParameter(x.set("X", {0.5}, {0}, {1}));
		addParameter(y.set("Y", {0.5}, {0}, {1}));
        
        listeners.push(sizeX.newListener([this](int &i){
            calcOutput();
        }));
        listeners.push(sizeY.newListener([this](int &i){
            calcOutput();
        }));
    }
    
private:
    ofEventListeners listeners;
    
    void calcOutput(){
        vector<float> tempX(sizeX*sizeY);
		vector<float> tempY(sizeX*sizeY);
        for(int i = 0; i < sizeY; i++){
            for(int j = 0 ; j < sizeX; j++){
                tempX[(i*sizeX) + j] = (j+0.5)/sizeX;
				tempY[(i*sizeX) + j] = (i+0.5)/sizeY;
            }
        }
        y = tempY;
        x = tempX;
    }
    
    ofParameter<int> sizeX;
    ofParameter<int> sizeY;
    ofParameter<vector<float>> x;
	ofParameter<vector<float>> y;
};

#endif /* pointMatrix_h */
