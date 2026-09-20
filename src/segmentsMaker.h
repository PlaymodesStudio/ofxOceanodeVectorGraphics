//
//  segmentsMaker.h
//  example
//
//  Created by Eduard Frigola Bagué on 11/02/2021.
//

#ifndef segmentsMaker_h
#define segmentsMaker_h

#include "ofxOceanodeNodeModel.h"

class segmentsMaker : public ofxOceanodeNodeModel {
public:
	segmentsMaker() : ofxOceanodeNodeModel("Segments Maker"){}
	
	void setup(){
		addParameter(x.set("X", {0.5}, {0}, {1}));
        addParameter(y.set("Y", {0.5}, {0}, {1}));
		addParameter(allSegments.set("All", false));
		addParameter(numVertex.set("Vertex", 2, 2, INT_MAX));
		addParameter(numSegments.set("Num", 1, 1, INT_MAX));
		addParameter(x_out.set("X_out", {0.5}, {0}, {1}));
        addParameter(y_out.set("Y_out", {0.5}, {0}, {1}));
		
		listeners.push(x.newListener([this](vector<float> &vf){
			if(x->size() == y->size()){
				if(x->size() < 2) return;
				if(allSegments){
					numSegments = 0.5 * x->size() * (x->size()-1);
				}else{
					numSegments = x->size()-1;
				}
				x_tmp.resize(numSegments*(numVertex+1));
				y_tmp.resize(numSegments*(numVertex+1));
				
				if(allSegments){
					int k = 0;
					for(int i = 0; i < x->size()-1; i++){
						for(int j = i+1; j < x->size(); j++){
							for(int l = 0; l < numVertex ; l++){
								x_tmp[(k*(numVertex+1))+l] = ofLerp(x->at(i), x->at(j), float(l)/(float)(numVertex-1));
								y_tmp[(k*(numVertex+1))+l] = ofLerp(y->at(i), y->at(j), float(l)/(float)(numVertex-1));
							}
//							x_tmp[(k*(numVertex+1))] = x->at(i);
//							y_tmp[(k*(numVertex+1))] = y->at(i);
//							x_tmp[(k*(numVertex+1))+1] = x->at(j);
//							y_tmp[(k*(numVertex+1))+1] = y->at(j);
							
							x_tmp[(k*(numVertex+1))+numVertex] = -1;
							y_tmp[(k*(numVertex+1))+numVertex] = -1;
							k++;
						}
					}
				}else{
					for(int i = 0; i < x->size()-1; i++){
						for(int l = 0; l < numVertex ; l++){
							x_tmp[(i*(numVertex+1))+l] = ofLerp(x->at(i), x->at(i+1), float(l)/(float)(numVertex-1));
							y_tmp[(i*(numVertex+1))+l] = ofLerp(y->at(i), y->at(i+1), float(l)/(float)(numVertex-1));
						}
//						x_tmp[(i*(numVertex+1))] = x->at(i);
//						y_tmp[(i*(numVertex+1))] = y->at(i);
//						x_tmp[(i*(numVertex+1))+1] = x->at(i+1);
//						y_tmp[(i*(numVertex+1))+1] = y->at(i+1);
						
						x_tmp[(i*(numVertex+1))+numVertex] = -1;
						y_tmp[(i*(numVertex+1))+numVertex] = -1;
					}
				}
                y_out = y_tmp;
				x_out = x_tmp;
			}
		}));
		
	}
	
private:
	ofParameter<vector<float>> x, y;
	vector<float> x_tmp, y_tmp;
	ofParameter<vector<float>> x_out, y_out;
	ofParameter<int> numVertex;
	ofParameter<int> numSegments;
	ofParameter<bool> allSegments;

	ofEventListeners listeners;
};

#endif /* segmentsMaker_h */
