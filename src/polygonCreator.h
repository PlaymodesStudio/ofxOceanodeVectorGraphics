//
//  polygonCreator.h
//  example
//
//  Created by Eduard Frigola Bagué on 11/02/2021.
//

#ifndef polygonCreator_h
#define polygonCreator_h

#define M_PI 3.14159265358979323846264338327950288
#define PI M_PI

#include "ofxOceanodeNodeModel.h"

class polygonCreator : public ofxOceanodeNodeModel{
public:
	polygonCreator() : ofxOceanodeNodeModel("Polygon Creator"){};
	~polygonCreator(){};
	
	void setup(){
		addParameter(sides.set("Sides", {1}, {1}, {INT_MAX}));
        addParameter(closed.set("Closed", true));
		addParameter(toCenterFigure.set("To Center", {0}, {-1}, {1}));
		addParameter(translateX.set("X.Trans", {0.5}, {0}, {1}));
		addParameter(translateY.set("Y.Trans", {0.5}, {0}, {1}));
		addParameter(scaleX.set("X.Scale", {1}, {0}, {2}));
		addParameter(scaleY.set("Y.Scale", {1}, {0}, {2}));
		addParameter(rotate.set("Rot", {0}, {0}, {1}));
		addParameter(xOut.set("X.Out", {0.5}, {0}, {1}));
        addParameter(yOut.set("Y.Out", {0.5}, {0}, {1}));
        
        addInspectorParameter(overlap.set("Overlap", 1, 0, FLT_MAX));
	}
	
	void update(ofEventArgs &a){
		if(translateX.get().size()!=0 && translateY.get().size()!=0 && scaleX.get().size()!=0 && scaleY.get().size()!=0 && rotate.get().size()!=0 )
		{
			int numElements = max({sides->size(),
									toCenterFigure->size(),
									translateX->size(),
									translateY->size(),
									scaleX->size(),
									scaleY->size(),
									rotate->size()
			});
			
			vector<float> x_temp;
			vector<float> y_temp;
			for(int i = 0 ; i < numElements; i++){
				if(i != 0){
					x_temp.push_back(-1);
					y_temp.push_back(-1);
				}
				
				ofPoint position;
				position.x = getParameterValueForPosition(translateX, i);
				position.y = getParameterValueForPosition(translateY, i);
				ofPoint firstCreatedPoint = ofPoint(-100, -100);
				ofPoint lastCreatedVertex = ofPoint(-100, -100);
				ofPoint newVertex;
				int counter = 0;
				for(int j = 0 ; j < (getParameterValueForPosition(sides, i) + closed) ; j++){
					float jj = ((j * overlap) / (float)getParameterValueForPosition(sides, i)) + getParameterValueForPosition(rotate, i);
					
					newVertex.x = (sin(jj*2*PI)*getParameterValueForPosition(scaleX, i)/2)+position.x;
					newVertex.y = (cos(jj*2*PI)*getParameterValueForPosition(scaleY, i)/2)+position.y;
					if(j != 0 && getParameterValueForPosition(toCenterFigure, i) != 0){
						ofPoint middleVertex = (newVertex+lastCreatedVertex) / 2;
						ofPoint toCenterPoint = (middleVertex * (1 - getParameterValueForPosition(toCenterFigure, i))) + (position * getParameterValueForPosition(toCenterFigure, i));
						//ADD Point
						x_temp.push_back(toCenterPoint.x);
						y_temp.push_back(toCenterPoint.y);
					}
					//ADD Point
					x_temp.push_back(newVertex.x);
					y_temp.push_back(newVertex.y);
					
					lastCreatedVertex = newVertex;
					if(j == 0){
						firstCreatedPoint = newVertex;
					}
				}
			}
			yOut = y_temp;
			xOut = x_temp;

		}
	}
	
private:
	ofParameter<vector<int>> sides;
	ofParameter<vector<float>> toCenterFigure;
	ofParameter<vector<float>> translateX, translateY;
	ofParameter<vector<float>> scaleX, scaleY;
	ofParameter<vector<float>> rotate;
    ofParameter<float> overlap;
    
    ofParameter<bool> closed;
	
	template <typename T>
    T getParameterValueForPosition(ofParameter<vector<T>> &param, int index){
        if(param.get().size() == 1 || param.get().size() <= index){
            return param.get()[0];
        }
        else{
            return param.get()[index];
        }
    }
	
	
	vector<float> x_tmp, y_tmp;
	ofParameter<vector<float>> xOut, yOut;

	ofEventListeners listeners;
};

#endif /* polygonCreator_h */
