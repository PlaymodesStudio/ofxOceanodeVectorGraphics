//
//  fourierEpicyles.h
//  example
//
//  Created by Eduard Frigola Bagué on 14/10/2020.
//

#ifndef fourierEpicyles_h
#define fourierEpicyles_h

#include "ofxOceanodeNodeModel.h"
#include "ofxFatLine.h"


class fatlineFourier : public ofxOceanodeNodeModel{
public:
    fatlineFourier() : ofxOceanodeNodeModel("Fat Line Fourier"){};
    
    void setup(){
        addParameter(freq.set("Freq", {1}, {-10000}, {10000}));
        addParameter(size.set("Size", {0.5}, {0}, {1}));
        addParameter(phase.set("Phase", {0}, {0}, {1}));
        addParameter(output.set("Output", {ofxFatLine()}));
        addParameter(xOutput.set("X", {0}, {0}, {1}));
        addParameter(yOutput.set("Y", {0}, {0}, {1}));
        addParameter(polyOutput.set("Poly", ofPolyline()));
    }
    
    void update(ofEventArgs &a){
        ofPolyline line;
        ofSetLineWidth(4);
        int numSteps = 1.0f / 0.001;
        int minSize = min(min(size->size(), phase->size()), freq->size());
        xTemp.resize(numSteps);
        yTemp.resize(numSteps);
        int k = 0;
        for(float i = 0; i < 1; i=i+0.001){
            glm::vec3 newVertex(0.5, 0.5, 0);
            for(int j = 0; j < minSize; j++){
                newVertex.x = (sin(((i * (freq->at(j)))+phase->at(j))*2*PI)*size->at(j)/2) + newVertex.x;//+position.x;
                newVertex.y = (cos(((i * (freq->at(j)))+phase->at(j))*2*PI)*size->at(j)/2) + newVertex.y;//+position.y;
                
            }
            xTemp[k] = newVertex.x;
            yTemp[k] = newVertex.y;
            k++;
            line.lineTo(newVertex);
        }
        polyOutput = line;
        ofxFatLine fatline;
        fatline.setFromPolyline(line);
        output.set(vector<ofxFatLine>(1, fatline));
//        output.set(ofxVaserPolyline(line));
        xOutput = xTemp;
        yOutput = yTemp;
    }
    
private:
    ofParameter<vector<float>> freq;
    ofParameter<vector<float>> size;
    ofParameter<vector<float>> phase;
    ofParameter<vector<float>> xOutput;
    ofParameter<vector<float>> yOutput;
    vector<float> xTemp;
    vector<float> yTemp;
    ofParameter<ofPolyline> polyOutput;
    
    ofParameter<vector<ofxFatLine>> output;
};

class fourierTransform : public ofxOceanodeNodeModel{
public:
	fourierTransform() : ofxOceanodeNodeModel("Fourier Transform"){}
	
	void setup(){
		addParameter(x.set("X", {0}, {0}, {1}));
        addParameter(y.set("Y", {0}, {0}, {1}));
		addParameter(freq.set("Freq", {1}, {-10000}, {10000}));
        addParameter(size.set("Size", {0.5}, {0}, {1}));
        addParameter(phase.set("Phase", {0}, {0}, {1}));
	}
	
	void update(ofEventArgs &a){
		
	}
	
private:
	ofParameter<vector<float>> x;
    ofParameter<vector<float>> y;
	ofParameter<vector<float>> freq;
    ofParameter<vector<float>> size;
    ofParameter<vector<float>> phase;
};

#endif /* fourierEpicyles_h */
