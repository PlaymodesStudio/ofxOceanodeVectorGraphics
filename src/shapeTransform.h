//
//  shapeTransform.h
//  example
//
//  Created by Eduard Frigola Bagué on 11/02/2021.
//

#ifndef shapeTransform_h
#define shapeTransform_h

#define M_PI 3.14159265358979323846264338327950288

#include "ofxOceanodeNodeModel.h"

class shapeTransform : public ofxOceanodeNodeModel {
public:
	shapeTransform() : ofxOceanodeNodeModel("Shape Transform"){}
	
	void setup(){
        
		addParameter(xIn.set("X.In", {0.5}, {0}, {1}));
        addParameter(yIn.set("Y.In", {0.5}, {0}, {1}));
		addParameter(replicate.set("Replicate", 1, 1, INT_MAX));
		addParameter(translateX.set("X.Trans", {0.5}, {0}, {1}));
		addParameter(translateY.set("Y.Trans", {0.5}, {0}, {1}));
		addParameter(scaleX.set("X.Scale", {1}, {0}, {2}));
		addParameter(scaleY.set("Y.Scale", {1}, {0}, {2}));
		addParameter(rotate.set("Rot", {0}, {0}, {1}));
		addParameter(xOut.set("X.Out", {0.5}, {0}, {1}));
        addParameter(yOut.set("Y.Out", {0.5}, {0}, {1}));

        addInspectorParameter(clampOutput.set("Clamp output", false));

		listeners.push(xIn.newListener([this](vector<float> &vf){
            if(vf.size() > 0){
			auto getValueForIndex = [](const vector<float> &vf, int i, int replicate, int numShapes) -> float{
				if(vf.size() == replicate){
					return vf[floor(i/numShapes)];
				}else if(i < vf.size()){
					return vf[i];
				}else{
					return vf[0];
				}
			};
			
			
			if((xIn->size() == yIn->size())){
				bool addSplit = false;
				if(xIn->back() == -1){
					x_tmp.resize(xIn->size()*replicate);
					y_tmp.resize(xIn->size()*replicate);
				}else{
					addSplit = true;
					x_tmp.resize((xIn->size()+1)*replicate);
					y_tmp.resize((xIn->size()+1)*replicate);
				}
				
				int numShapes = 1;
				for(int i = 0; i < xIn->size()-1; i++){
					if(xIn->at(i) == -1) numShapes++;
				}
				
				int shapeIndex = 0;
				for(int i = 0; i < x_tmp.size(); i++){
					int ii = i % (xIn->size() + addSplit);
					if(addSplit && ii == xIn->size()){
						shapeIndex++;
						x_tmp[i] = -1;
						y_tmp[i] = -1;
					}
					if(xIn->size() > ii){
						if(xIn->at(ii) == -1){
							shapeIndex++;
							x_tmp[i] = -1;
							y_tmp[i] = -1;
						}else{
							x_tmp[i] = xIn->at(ii);
							y_tmp[i] = yIn->at(ii);
							x_tmp[i] -= 0.5;
							y_tmp[i] -= 0.5;
							float x_copy = x_tmp[i];
							x_tmp[i] = x_copy * cos(getValueForIndex(rotate, shapeIndex, replicate, numShapes)*M_PI*2) - y_tmp[i] * sin(getValueForIndex(rotate, shapeIndex, replicate, numShapes)*M_PI*2);
							y_tmp[i] = x_copy * sin(getValueForIndex(rotate, shapeIndex, replicate, numShapes)*M_PI*2) + y_tmp[i] * cos(getValueForIndex(rotate, shapeIndex, replicate, numShapes)*M_PI*2);
							x_tmp[i] *= getValueForIndex(scaleX, shapeIndex, replicate, numShapes);
							y_tmp[i] *= getValueForIndex(scaleY, shapeIndex, replicate, numShapes);
							x_tmp[i] += 0.5 + ((getValueForIndex(translateX, shapeIndex, replicate, numShapes)-0.5));
							y_tmp[i] += 0.5 + ((getValueForIndex(translateY, shapeIndex, replicate, numShapes)-0.5));
                            if(clampOutput){
                                x_tmp[i] = ofClamp(x_tmp[i], 0, 1);
                                y_tmp[i] = ofClamp(y_tmp[i], 0, 1);
                            }
						}
					}else{
						x_tmp[i] = -1;
						y_tmp[i] = -1;
					}
				}
                yOut = y_tmp;
				xOut = x_tmp;
			}
        }
    }));
	}
	
private:
	ofParameter<vector<float>> xIn, yIn;
	ofParameter<int> replicate;
	ofParameter<vector<float>> translateX, translateY;
	ofParameter<vector<float>> scaleX, scaleY;
	ofParameter<vector<float>> rotate;
    ofParameter<bool> clampOutput;

	vector<float> x_tmp, y_tmp;
	ofParameter<vector<float>> xOut, yOut;

	ofEventListeners listeners;
};

#endif /* shapeTransform_h */
