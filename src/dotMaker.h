//
//  dotMaker.h
//  example
//
//  Created by Eduard Frigola Bagué on 11/02/2021.
//

#ifndef dotMaker_h
#define dotMaker_h

#include "ofxOceanodeNodeModel.h"

class dotMaker : public ofxOceanodeNodeModel {
public:
	dotMaker() : ofxOceanodeNodeModel("Dot Maker"){}
	
	void setup(){
		addParameter(x.set("X", {0.5}, {0}, {1}));
        addParameter(y.set("Y", {0.5}, {0}, {1}));
		addParameter(x_out.set("X_out", {0.5}, {0}, {1}));
        addParameter(y_out.set("Y_out", {0.5}, {0}, {1}));
		
		listeners.push(x.newListener([this](vector<float> &vf){
			//generateDots();
		}));
		
	}
	void generateDots()
	{
		if(x->size() == y->size()){
			x_tmp.clear();
			y_tmp.clear();
			x_tmp.reserve((x->size()*2));
			y_tmp.reserve((x->size()*2));
			
			for(int i = 0; i < x->size(); i++){
//					x_tmp[(i*2)] = x->at(i);
//					y_tmp[(i*2)] = y->at(i);
//
//					x_tmp[(i*2)+1] = -1;
//					y_tmp[(i*2)+1] = -1;
				if(x->at(i) != -1){
					x_tmp.push_back(x->at(i));
					y_tmp.push_back(y->at(i));
					
					x_tmp.push_back(-1);
					y_tmp.push_back(-1);
				}
			}
			y_out = y_tmp;
			x_out = x_tmp;
		}
	}
	
	void update(ofEventArgs &a) override {
		generateDots();
	}
	
private:
	ofParameter<vector<float>> x, y;
	vector<float> x_tmp, y_tmp;
	ofParameter<vector<float>> x_out, y_out;

	
	ofEventListeners listeners;
};

#endif /* dotMaker_h */
