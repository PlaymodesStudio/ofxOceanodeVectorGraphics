//
//  polylineGenerator.h
//  example
//
//  Created by Eduard Frigola Bagué on 14/10/2020.
//

#ifndef polylineGenerator_h
#define polylineGenerator_h

#include "ofxOceanode.h"

class polylineGenerator : public ofxOceanodeNodeModel{
public:
    polylineGenerator() : ofxOceanodeNodeModel("Poly Line"){};
    
    void setup(){
        addParameter(x.set("X", {0}, {0}, {1}));
        addParameter(y.set("Y", {0}, {0}, {1}));
        addParameter(w.set("W", {0}, {0}, {20}));
        addParameter(r.set("R", {1}, {0}, {1}));
        addParameter(g.set("G", {1}, {0}, {1}));
        addParameter(b.set("B", {1}, {0}, {1}));
        addParameter(a.set("A", {1}, {0}, {1}));
		addParameter(fr.set("FillR", {0}, {0}, {1}));
        addParameter(fg.set("FillG", {0}, {0}, {1}));
        addParameter(fb.set("FillB", {0}, {0}, {1}));
        addParameter(fa.set("FillA", {0}, {0}, {1}));
        
        addParameter(output.set("Output", {ofPath()}));
    }
    
    void update(ofEventArgs &args){
        bool close = false;
        vector<vector<glm::vec3>> points;
		int index = 0;
		if(x->size() == y->size()){
			auto iterbegin = x->begin();
			auto iterend = std::find(x->begin(), x->end(), -1);
			int i = 0;
			while (iterend != x->end()){
				points.emplace_back();
				points[i].resize(iterend - iterbegin);
				int beginIndex = iterbegin - x->begin();
				for(int j = 0; j < points[i].size(); j++){
					points[i][j] = glm::vec3(x.get()[beginIndex+j], y.get()[beginIndex+j], 0);
				}
				iterbegin = iterend+1;
				iterend = std::find(iterbegin, x->end(), -1);
				i++;
			}
			points.emplace_back();
			points[i].resize(iterend - iterbegin);
			int beginIndex = iterbegin - x->begin();
			for(int j = 0; j < points[i].size(); j++){
				points[i][j] = glm::vec3(x.get()[beginIndex+j], y.get()[beginIndex+j], 0);
			}
		}else if(x->size() > y->size() && y->size() == 1){
			points.resize(1);
			points[0].resize(close ? x->size() + 1 : x->size());
			for(int i = 0; i < points[0].size(); i++){
				points[0][i] = glm::vec3(x.get()[i], y.get()[0], 0);
			}
		}else if(x->size() < y->size() && x->size() == 1){
			points.resize(1);
			points[0].resize(close ? y->size() + 1 : y->size());
			for(int i = 0; i < points[0].size(); i++){
				points[0][i] = glm::vec3(x.get()[0], y.get()[i], 0);
			}
        }else{
            return;
        }
		if(points.back().size() == 0) points.pop_back();
		
        auto getFromVec = [](const vector<float>& vf, int index) -> float{
            if (!vf.empty()) {
                if(index < vf.size()) return vf[index];
                return vf[0];
            }
            return 0.0f;
        };

		vector<ofPath> lines(points.size());
		
		for(int i = 0 ; i < points.size(); i++){
			lines[i].moveTo(points[i][0]);
			for(int j = 1; j < points[i].size(); j++){
				lines[i].lineTo(points[i][j]);
			}
			lines[i].setStrokeColor(ofFloatColor(getFromVec(r, i), getFromVec(g, i), getFromVec(b, i), getFromVec(a, i)));
			lines[i].setFillColor(ofFloatColor(getFromVec(fr, i), getFromVec(fg, i), getFromVec(fb, i), getFromVec(fa, i)));
			lines[i].setStrokeWidth(getFromVec(w, i));
			
			//lines[i].
		}
        output.set(lines);
    }
    
private:
    ofParameter<vector<float>> x;
    ofParameter<vector<float>> y;
    ofParameter<vector<float>> w;
    ofParameter<vector<float>> r;
    ofParameter<vector<float>> g;
    ofParameter<vector<float>> b;
    ofParameter<vector<float>> a;
    ofParameter<vector<float>> fr;
    ofParameter<vector<float>> fg;
    ofParameter<vector<float>> fb;
    ofParameter<vector<float>> fa;
    ofParameter<vector<ofPath>> output;
};

#endif /* polylineGenerator_h */
