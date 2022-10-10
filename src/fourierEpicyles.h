//
//  fourierEpicyles.h
//  example
//
//  Created by Eduard Frigola Bagué on 14/10/2020.
//

#ifndef fourierEpicyles_h
#define fourierEpicyles_h

#include "ofxOceanodeNodeModel.h"


class inverseFourierShape : public ofxOceanodeNodeModel{
public:
    inverseFourierShape() : ofxOceanodeNodeModel("Inv Fourirer Shape"){};
    
    void setup(){
        addParameter(freq.set("Freq", {1}, {-10000}, {10000}));
        addParameter(size.set("Size", {0.5}, {0}, {1}));
        addParameter(phase.set("Phase", {0}, {0}, {1}));
        addParameter(xOutput.set("X", {0}, {0}, {1}));
        addParameter(yOutput.set("Y", {0}, {0}, {1}));
    }
    
    void update(ofEventArgs &a){
		float numSteps = freq->size();
        int minSize = min(min(size->size(), phase->size()), freq->size());
        xTemp.resize(numSteps);
        yTemp.resize(numSteps);
        int k = 0;
        for(float i = 0; i < numSteps; i++){
            glm::vec3 newVertex(0, 0, 0);
            for(int j = 0; j < minSize; j++){
                newVertex.x = (cos((((i/numSteps)*(freq->at(j)))+phase->at(j))*2*PI)*size->at(j)) + newVertex.x;//+position.x;
                newVertex.y = (sin((((i/numSteps)*(freq->at(j)))+phase->at(j))*2*PI)*size->at(j)) + newVertex.y;//+position.y;
                
            }
            xTemp[k] = (newVertex.x+1)*0.5;
            yTemp[k] = (newVertex.y+1)*0.5;
            k++;
        }
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
};

class fourierShape : public ofxOceanodeNodeModel{
public:
	fourierShape() : ofxOceanodeNodeModel("Fourier Shape"){}
	
	void setup(){
		addParameter(x.set("X", {0}, {0}, {1}));
        addParameter(y.set("Y", {0}, {0}, {1}));
		addParameter(freq.set("Freq", {1}, {-10000}, {10000}));
        addParameter(amp.set("Size", {0.5}, {0}, {1}));
        addParameter(phase.set("Phase", {0}, {0}, {1}));
	}
	
	void update(ofEventArgs &a){
		if(x->size() == y->size()){
			int size = x->size();
			int halfSize = size/2;
			vector<float> tempFreq(size, 0);
			vector<float> tempAmp(size, 0);
			vector<float> tempPhase(size, 0);
			for(int i = 0; i < size; i++){
				float re = 0;
				float im = 0;
				for(int j = 0; j < size; j++){
					float phi = (TWO_PI * i * j) / size;
					float circleReal = cos(phi);
					float circleImaginary = -sin(phi);
					float newReal = (x->at(j)*2)-1;
					float newImaginary = (y->at(j)*2)-1;
					re += ((newReal*circleReal) - (newImaginary*circleImaginary));
					im += ((newReal*circleImaginary) + (newImaginary*circleReal));
					
				}
				re /= size;
				im /= size;
				
				tempFreq[i] = i;
				tempAmp[i] = sqrt((re * re) + (im * im));
				tempPhase[i] = atan2(im, re)/TWO_PI;
			}
			freq = tempFreq;
			amp = tempAmp;
			phase = tempPhase;
		}
	}
	
private:
	ofParameter<vector<float>> x;
    ofParameter<vector<float>> y;
	ofParameter<vector<float>> freq;
    ofParameter<vector<float>> amp;
    ofParameter<vector<float>> phase;
};

#endif /* fourierEpicyles_h */
