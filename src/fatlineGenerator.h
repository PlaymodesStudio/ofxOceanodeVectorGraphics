//
//  fatlineGenerator.h
//  example
//
//  Created by Eduard Frigola Bagué on 11/02/2021.
//

#ifndef fatlineGenerator_h
#define fatlineGenerator_h

#define M_PI 3.14159265358979323846264338327950288

#include "ofxOceanodeNodeModel.h"
#include "ofxFatLine.h"

class fatlineGenerator : public ofxOceanodeNodeModel{
public:
    fatlineGenerator() : ofxOceanodeNodeModel("Fat Line"){};
    
    void setup(){
        addParameter(width.set("Width", 800, 1, 5000));
        addParameter(height.set("Height", 800, 1, 5000));
        addParameter(x.set("X", {0}, {0}, {1}));
        addParameter(y.set("Y", {0}, {0}, {1}));
        addParameter(w.set("W", {1}, {0}, {20}));
        addParameter(r.set("R", {1}, {0}, {1}));
        addParameter(g.set("G", {1}, {0}, {1}));
        addParameter(b.set("B", {1}, {0}, {1}));
        addParameter(a.set("A", {1}, {0}, {1}));
		addParameter(opacity.set("Opacity",1.0,0.0,1.0));
        addParameter(output.set("Output", {ofxFatLine()}));
    }
    
    void update(ofEventArgs &args) {
        if(x->size() != y->size() || x->size() <= 1) return;
        
        // Helper function to safely get values from vectors
        auto getFromVec = [](const vector<float>& vf, int index) -> float {
            if(vf.size() == 0) return -1;
            if(index < vf.size()) return vf[index];
            return vf[0];
        };
        
        // Pre-calculate sizes
        const size_t inputSize = x->size();
        const auto& xData = x.get();
        const auto& yData = y.get();
        
        // Count number of shapes and max size
        size_t numShapes = 1;
        size_t maxShapeSize = 0;
        size_t currentShapeSize = 0;
        
        for(size_t i = 0; i < inputSize; ++i) {
            if(xData[i] == -1) {
                numShapes++;
                maxShapeSize = std::max(maxShapeSize, currentShapeSize);
                currentShapeSize = 0;
            } else {
                currentShapeSize++;
            }
        }
        maxShapeSize = std::max(maxShapeSize, currentShapeSize);
        
        // Pre-allocate vectors
        vector<ofxFatLine> fatlines(numShapes);
        vector<glm::vec3> points;
        points.reserve(maxShapeSize);
        
        // Process shapes
        size_t shapeIndex = 0;
        size_t accumPosition = 0;
        
        auto processShape = [&](size_t start, size_t end) {
            points.clear();
            points.reserve(end - start +1);
            
            // Build points for current shape
            for(size_t j = start; j <= end; ++j) {
                points.emplace_back(xData[j] * width, yData[j] * height, 0);
            }
            
            // Process colors and weights once per shape
            vector<ofFloatColor> colors(points.size());
            vector<double> weights(points.size());
            
            for(size_t j = 0; j < points.size(); ++j) {
                colors[j].set(
                              getFromVec(r.get(), accumPosition + j)*opacity,
                              getFromVec(g.get(), accumPosition + j)*opacity,
                              getFromVec(b.get(), accumPosition + j)*opacity,
                              getFromVec(a.get(), accumPosition + j)*opacity
                              );
                weights[j] = getFromVec(w.get(), accumPosition + j);
            }
            
            fatlines[shapeIndex].add(points, colors, weights);
            accumPosition += maxShapeSize;
            shapeIndex++;
        };
        
        // Process all shapes
        size_t start = 0;
        for(size_t i = 0; i < inputSize; ++i) {
            if((xData[i] == -1)&&(i!=0)){
                processShape(start, i-1);
                start = i + 1;
            }
            if(i == inputSize - 1) {
                processShape(start, i);
                start = i + 1;
            }
        }
        
        output.set(fatlines);
    }
    
private:
    
    ofParameter<vector<float>> x;
    ofParameter<vector<float>> y;
    ofParameter<vector<float>> w;
    ofParameter<vector<float>> r;
    ofParameter<vector<float>> g;
    ofParameter<vector<float>> b;
    ofParameter<vector<float>> a;
	ofParameter<float> opacity;
    ofParameter<int> width, height;
    ofParameter<vector<ofxFatLine>> output;
    
//    ofxFatLine fatline;
};

#endif /* fatlineGenerator_h */
