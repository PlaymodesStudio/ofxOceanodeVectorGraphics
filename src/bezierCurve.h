//
//  bezierCurve.h
//  example
//
//  Created by Eduard Frigola Bagué on 11/02/2021.
//

#ifndef bezierCurve_h
#define bezierCurve_h

#include "ofxOceanodeNodeModel.h"

class bezierCurve : public ofxOceanodeNodeModel{
public:
    bezierCurve() : ofxOceanodeNodeModel("Bezier Curve"){}
    
    void setup(){
        addParameter(xIn.set("X.In", {0.5}, {0}, {1}));
        addParameter(yIn.set("Y.In", {0.5}, {0}, {1}));
        addParameter(resolution.set("Res", 15, 1, INT_MAX));
        addParameter(close.set("Close", {1}, {0}, {1}));
        addParameter(xOut.set("X.Out", {0.5}, {0}, {1}));
        addParameter(yOut.set("Y.Out", {0.5}, {0}, {1}));
        
        listeners.push(xIn.newListener([this](vector<float> &vf){
            if(xIn->size() == yIn->size()){
                int numShapes = 1;
                for(int i = 0; i < xIn->size()-1; i++){ //Skip last in case is a -1
                    if(xIn->at(i) == -1){
                        numShapes++;
                    }
                }

                x_tmp.clear();
                y_tmp.clear();
                x_tmp.reserve((xIn->size()) * resolution); //Add one for the -1
                y_tmp.reserve((xIn->size()) * resolution);

                ofPolyline poly;
                vector<glm::vec3> points;
                int currentShape = 0;
                for(int i = 0; i < xIn->size(); i++){
                    if(xIn->at(i) == -1){
                        if(points.size() > 1){
                            //Code from Roy Macdonald found at: https://forum.openframeworks.cc/t/connecting-path-curveto-first-and-last-points/39309/3
                            
                            bool closed = close->size() == numShapes ? close->at(currentShape) : close->at(0);
                            poly.addVertex(points[0]);
                            if(closed){
                                poly.curveTo(points[points.size()-1], resolution); // add the last point first
                            }else{
                                poly.curveTo(points[0], resolution);
                            }
                            for (int j = 0; j < points.size(); j++)
                            {
                                poly.curveTo(points[j], resolution);
                            }
                            if(closed){
                                poly.curveTo(points[0], resolution);
                                poly.curveTo(points[1], resolution);
                            }else{
                                poly.curveTo(points.back(), resolution);
                            }
                            
                            
                            for(int j = 0; j < poly.size(); j++){
                                x_tmp.push_back(poly[j].x);
                                y_tmp.push_back(poly[j].y);
                            }
                            x_tmp.push_back(-1);
                            y_tmp.push_back(-1);
                            poly.clear();
                            points.clear();
                            currentShape++;
                        }else{
                            x_tmp.push_back(points[0].x);
                            y_tmp.push_back(points[0].y);
                            x_tmp.push_back(-1);
                            y_tmp.push_back(-1);
                        }
                    }else{
                        points.emplace_back(xIn->at(i), yIn->at(i), 0);
                    }
                }
                if(points.size() > 1){
                    bool closed = close->size() == numShapes ? close->at(currentShape) : close->at(0);
                    poly.addVertex(points[0]);
                    if(closed){
                        poly.curveTo(points[points.size()-1], resolution); // add the last point first
                    }else{
                        poly.curveTo(points[0], resolution);
                    }
                    for (int j = 0; j < points.size(); j++)
                    {
                        poly.curveTo(points[j], resolution);
                    }
                    if(closed){
                        poly.curveTo(points[0], resolution);
                        poly.curveTo(points[1], resolution);
                    }else{
                        poly.curveTo(points.back(), resolution);
                    }
                    
                    
                    for(int j = 0; j < poly.size(); j++){
                        x_tmp.push_back(poly[j].x);
                        y_tmp.push_back(poly[j].y);
                    }
                    x_tmp.push_back(-1);
                    y_tmp.push_back(-1);
                }
                yOut = y_tmp;
                xOut = x_tmp;
            }
        }));
    }
    
private:
    ofEventListeners listeners;
    
    ofParameter<vector<float>> xIn, yIn, xOut, yOut;
    vector<float> x_tmp, y_tmp;
    ofParameter<int> resolution;
    ofParameter<vector<int>> close;
};

#endif /* bezierCurve_h */
