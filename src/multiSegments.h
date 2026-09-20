//
//  multiSegments.h
//  example
//
//  Created by Eduard Frigola Bagué on 11/02/2021.
//

#ifndef multiSegments_h
#define multiSegments_h

#include "ofxOceanodeNodeModel.h"

class multiSegments : public ofxOceanodeNodeModel {
public:
	multiSegments() : ofxOceanodeNodeModel("Multi Segments"){}
	
	void setup(){
		addParameter(xStart.set("X.Start", {0.5}, {0}, {1}));
        addParameter(yStart.set("Y.Start", {0.5}, {0}, {1}));
		addParameter(xEnd.set("X.End", {0.5}, {0}, {1}));
        addParameter(yEnd.set("Y.End", {0.5}, {0}, {1}));
		addParameter(x_out.set("X.Out", {0.5}, {0}, {1}));
        addParameter(y_out.set("Y.Out", {0.5}, {0}, {1}));
		
		listeners.push(xStart.newListener([this](vector<float> &vf){
			if((xStart->size() == yStart->size()) &&  (xStart->size() == xEnd->size()) && (xStart->size() == yEnd->size())){
				x_tmp.resize((xStart->size()*3));
				y_tmp.resize((xStart->size()*3));
				
				for(int i = 0; i < xStart->size(); i++){
					x_tmp[(i*3)] = xStart->at(i);
					y_tmp[(i*3)] = yStart->at(i);
					x_tmp[(i*3)+1] = xEnd->at(i);
					y_tmp[(i*3)+1] = yEnd->at(i);
					
					x_tmp[(i*3)+2] = -1;
					y_tmp[(i*3)+2] = -1;
				}
                y_out = y_tmp;
				x_out = x_tmp;
			}
		}));
		
	}
	
private:
	ofParameter<vector<float>> xStart, yStart, xEnd, yEnd;
	vector<float> x_tmp, y_tmp;
	ofParameter<vector<float>> x_out, y_out;

	ofEventListeners listeners;
};

#endif /* multiSegments_h */
