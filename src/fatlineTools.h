//
//  fatlineTools.h
//  example
//
//  Created by Eduard Frigola Bagué on 11/02/2021.
//

#ifndef fatlineTools_h
#define fatlineTools_h

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
        addParameter(w.set("W", {0}, {0}, {20}));
        addParameter(r.set("R", {0}, {0}, {1}));
        addParameter(g.set("G", {0}, {0}, {1}));
        addParameter(b.set("B", {0}, {0}, {1}));
        addParameter(a.set("A", {0}, {0}, {1}));
        
        addParameter(output.set("Output", {ofxFatLine()}));
    }
    
    void update(ofEventArgs &args){
        bool close = false;
        vector<vector<glm::vec3>> points;
		int index = 0;
        int maxSize = 0;
        if(x->size() == y->size() && x->size() > 1) {
			auto iterbegin = x->begin();
			auto iterend = std::find(x->begin(), x->end(), -1);
			int i = 0;
			while (iterend != x->end()){
				points.emplace_back();
				points[i].resize(iterend - iterbegin);
				int beginIndex = iterbegin - x->begin();
				for(int j = 0; j < points[i].size(); j++){
					points[i][j] = glm::vec3(x.get()[beginIndex+j] * width, y.get()[beginIndex+j] * height, 0);
				}
				iterbegin = iterend+1;
				iterend = std::find(iterbegin, x->end(), -1);
                if(points[i].size() > maxSize) maxSize = points[i].size();
				i++;
			}
			points.emplace_back();
			points[i].resize(iterend - iterbegin);
			int beginIndex = iterbegin - x->begin();
			for(int j = 0; j < points[i].size(); j++){
				points[i][j] = glm::vec3(x.get()[beginIndex+j] * width, y.get()[beginIndex+j] * height, 0);
			}
			
//            points.resize(close ? x->size() + 1 : x->size());
//            for(int i = 0; i < points.size(); i++){
//                points[i] = glm::vec3(x.get()[i] * width, y.get()[i] * height, 0);
//            }
//        }else if(x->size() > y->size()){
//			points.resize(1);
//            points[0].resize(close ? x->size() + 1 : x->size());
//            for(int i = 0; i < points[0].size(); i++){
//                points[0][i] = glm::vec3(x.get()[i] * width, y.get()[0] * height, 0);
//            }
//        }else if(x->size() < y->size()){
//			points.resize(1);
//            points[0].resize(close ? y->size() + 1 : y->size());
//            for(int i = 0; i < points[0].size(); i++){
//                points[0][i] = glm::vec3(x.get()[0] * width, y.get()[i] * height, 0);
//            }
        }
		
//		vector<double> doubleWeights;
//        if(w->size() != points.size()){
//            doubleWeights = (vector<double>(points.size(), w->at(0)));
//        }else{
//            doubleWeights = vector<double>(w->begin(), w->end());
//        }
		//if(points.back().size() == 0) points.pop_back();
        
        auto getFromVec = [](const vector<float> vf, int index) -> float{
            if(index < vf.size()) return vf[index];
            return vf[0];
        };
        
		vector<ofxFatLine> fatlines(points.size());
		
		int accumPosition = 0;
		for(int i = 0 ; i < points.size(); i++){
			vector<ofFloatColor> colors;
			colors.resize(points[i].size());
			for(int j = 0; j < points[i].size(); j++){
				colors[j].set(getFromVec(r, accumPosition+j), getFromVec(g, accumPosition+j), getFromVec(b, accumPosition+j), getFromVec(a, accumPosition+j));
			}
			vector<double> doubleWeights;
			doubleWeights.resize(points[i].size());
			for(int j = 0; j < points[i].size(); j++){
				doubleWeights[j] = getFromVec(w, accumPosition+j);
			}
			
			fatlines[i].add(points[i], colors, doubleWeights);
			accumPosition += maxSize;
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
    ofParameter<int> width, height;
    ofParameter<vector<ofxFatLine>> output;
    
//    ofxFatLine fatline;
};


class fatlineRenderer : public ofxOceanodeNodeModel{
public:
    fatlineRenderer() : ofxOceanodeNodeModel("Fat Line Renderer"){};
    
    void setup(){
        addParameter(input.set("Input", {ofxFatLine()}));
        addParameter(width.set("Width", 100, 1, 5000));
        addParameter(height.set("Height", 100, 1, 5000));
        addParameter(output.set("Output", nullptr));
        //screenSize = glm::vec2(100, 100);
    }
    
    void update(ofEventArgs &a){
        if(!fbo.isAllocated() || fbo.getWidth() != width || fbo.getHeight() != height){
            fbo.allocate(width, height, GL_RGB);
        }
    
        //ofxFatLine copy = input.get();
//        copy.scale(screenSize.x, screenSize.y);
//        copy.update();
        fbo.begin();
        ofEnableAlphaBlending();
        ofPushMatrix();
        //ofScale(screenSize.x, screenSize.y);
        ofClear(0);
//        ofSetColor(255, 0, 0);
//        ofDrawRectangle( 100, 100, 200, 200);
        
        for(auto l : input.get()){
            l.update();
            l.draw();
        }
        
       // copy.draw();
        ofDisableAlphaBlending();
        ofPopMatrix();
        fbo.end();
        
        output = &fbo.getTexture();
    }
    
    void draw(ofEventArgs &a){
//        if(ImGui::Begin(("Fat Line Renderer Window " + ofToString(getNumIdentifier())).c_str())){
//            screenSize = ImGui::GetContentRegionAvail();
//            ImTextureID textureID = (ImTextureID)(uintptr_t)fbo.getTexture().texData.textureID;//       GetImTextureID(fbo.getTexture());
//            ImGui::Image(textureID, screenSize);
//        }
//        ImGui::End();
    }
    
private:
    ofParameter<int> width, height;
    //glm::vec2 screenSize;
    ofFbo fbo;
    ofParameter<vector<ofxFatLine>> input;
    ofParameter<ofTexture*> output;
};

class manualPoints : public ofxOceanodeNodeModel{
public:
    manualPoints() : ofxOceanodeNodeModel("Manual Points"){}
    
    void setup(){
        addParameter(input.set("Input", nullptr));
		addParameter(x.set("X", {0.5}, {0}, {1}));
        addParameter(y.set("Y", {0.5}, {0}, {1}));
		addParameter(numPoints.set("Num", {1}, {1}, {INT_MAX}));
        
        listeners.push(x.newListener([this](vector<float> &vf){
            if(x->size() == y->size() && !outputingValues){
                points.clear();
                points.resize(x->size());
                for(int i = 0; i < points.size(); i++){
                    points[i] == glm::vec2(x->at(i), y->at(i));
                }
            }
        }));
        
        listeners.push(y.newListener([this](vector<float> &vf){
            if(x->size() == y->size() && !outputingValues){
                points.clear();
                points.resize(x->size());
                for(int i = 0; i < points.size(); i++){
                    points[i] == glm::vec2(x->at(i), y->at(i));
                }
            }
        }));
		
		points.emplace_back(0.5, 0.5);
        
		ofPixels pix;
		pix.allocate(4, 4, OF_PIXELS_RGB);
		size_t i = 0;
		while( i < pix.size()) {
			pix[i] = 0;
			i++;
		}
		
        black.allocate(pix);
		black.loadData(pix);
    }
    
    void draw(ofEventArgs &a){
        if(ImGui::Begin(("Manual Points " + ofToString(getNumIdentifier())).c_str())){
            auto screenSize = ImGui::GetContentRegionAvail();
            auto screenPos = ImGui::GetCursorScreenPos();
            ImTextureID textureID = (ImTextureID)(uintptr_t)black.texData.textureID;
            if(input.get() != nullptr){
                textureID = (ImTextureID)(uintptr_t)input.get()->texData.textureID;
            }
            ImGui::Image(textureID, screenSize);
			if(ImGui::IsWindowHovered()){
				glm::vec2 normPos = glm::vec2(-1, -1);
				bool addRemove = false;
                if(ImGui::IsMouseClicked(0)){
                    normPos = (ImGui::GetMousePos() - screenPos) / screenSize;
                }
                else if(ImGui::IsMouseClicked(1)){
                    normPos = (ImGui::GetMousePos() - screenPos) / screenSize;
					addRemove = true;
                }
				if(normPos != glm::vec2(-1, -1)){
					bool foundPoint = false;
					for(int i = points.size()-1; i >= 0 && !foundPoint ; i--){
						auto point = (points[i] * screenSize) + screenPos;
						if(glm::distance(glm::vec2(ImGui::GetMousePos()), point) < 10){
							pointDraggingIndex = i;
							foundPoint = true;
						}
					}
					if(ImGui::GetIO().KeyShift){
						points.clear();
						points.push_back(normPos);
					}
					else if(addRemove){
						if(foundPoint){
							if(points.size() > 1){
								points.erase(points.begin() + pointDraggingIndex);
							}
							pointDraggingIndex = -1;
						}else{
							points.push_back(normPos);
						}
					}
				}
				else if(ImGui::IsMouseDragging(0)){
					if(pointDraggingIndex != -1){
						if(ImGui::GetIO().KeyAlt){
							glm::vec2 oldPos = points[pointDraggingIndex];
							glm::vec2 step = oldPos - ((ImGui::GetMousePos() - screenPos) / screenSize);
							points[pointDraggingIndex] = oldPos - (step/100.0);
						}else{
							points[pointDraggingIndex] = (ImGui::GetMousePos() - screenPos) / screenSize;
						}
					}else if(ImGui::GetIO().KeyShift){
						points.push_back((ImGui::GetMousePos() - screenPos) / screenSize);
					}
				}else if(ImGui::IsMouseReleased(0) || ImGui::IsMouseReleased(1)){
					pointDraggingIndex = -1;
				}
            }
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			vector<float> x_t(points.size());
			vector<float> y_t(points.size());
			for(int i = 0; i < points.size(); i++){
				draw_list->AddCircleFilled((points[i] * screenSize) + screenPos, 10, IM_COL32(255, 255, 255, 255));
				string numString = ofToString(i);
				draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), (points[i] * screenSize) + screenPos - glm::vec2(5, 5), IM_COL32(0,0,0,255), numString.c_str(), numString.c_str()+ (i < 10 ? 1 : 2));
				x_t[i] = points[i].x;
				y_t[i] = points[i].y;
			}
			draw_list->ChannelsMerge();
            outputingValues = true;
            y = y_t;
			x = x_t;
            outputingValues = false;
			numPoints = points.size();
        }
        ImGui::End();
    }
	
	void presetSave(ofJson &json) override{
        string pointsStr;
        for(auto &p : points){
            pointsStr += ofToString(p) + "_|_";
        }
        pointsStr.erase(pointsStr.end()-3, pointsStr.end());
        json["Points"] = pointsStr;
    }
    
    void presetRecallBeforeSettingParameters(ofJson &json) override{
        if(json.count("Points") == 1){
            vector<string> pointsStrVec = ofSplitString(json["Points"], "_|_");
            vector<glm::vec2> newPoints;
            newPoints.resize(pointsStrVec.size());
            for(int i = 0; i < newPoints.size(); i++){
                newPoints[i] = ofFromString<glm::vec2>(pointsStrVec[i]);
            }
            points = newPoints;
        }
    }
	
private:
    ofParameter<ofTexture*> input;
    ofTexture black;
    ofParameter<vector<float>> x, y;
	ofParameter<int> numPoints;
	vector<glm::vec2> points;
    
    ofEventListeners listeners;
	
	int pointDraggingIndex = -1;
    bool outputingValues = false;
};

class pointsResampler : public ofxOceanodeNodeModel {
public:
	pointsResampler() : ofxOceanodeNodeModel("Points Resampler"){}
	
	void setup(){
		addParameter(x.set("X", {0.5}, {0}, {1}));
        addParameter(y.set("Y", {0.5}, {0}, {1}));
		addParameter(numPoints.set("Num", {100}, {1}, {INT_MAX}));
        addParameter(withSpacing.set("Use Spa", false));
        addParameter(spacing.set("Spacing", 0.1, 0, 1));
		addParameter(x_out.set("X_out", {0.5}, {0}, {1}));
        addParameter(y_out.set("Y_out", {0.5}, {0}, {1}));
		
		listeners.push(x.newListener([this](vector<float> &vf){
//			if(x->size() == y->size()){
//				ofPolyline poly;
//				for(int i = 0; i < x->size(); i++){
//					poly.lineTo(x->at(i), y->at(i));
//				}
//				poly = poly.getResampledByCount(numPoints);
//				for(int i = 0; i < numPoints; i++){
//					auto point = poly[i];
//					x_tmp[i] = point.x;
//					y_tmp[i] = point.y;
//				}
//				x_out = x_tmp;
//				y_out = y_tmp;
//			}
            
            if(x->size() == y->size()){
                int numShapes = 1;
                for(int i = 0; i < x->size()-1; i++){ //Skip last in case is a -1
                    if(x->at(i) == -1){
                        numShapes++;
                    }
                }
                
                x_tmp.clear();
                y_tmp.clear();
                x_tmp.reserve((numPoints+1) * numShapes); //Add one for the -1
                y_tmp.reserve((numPoints+1) * numShapes);
                
                ofPolyline poly;
                for(int i = 0; i < x->size(); i++){
                    if(x->at(i) == -1){
                        if(withSpacing){
                            poly = poly.getResampledBySpacing(spacing);
                        }else{
                            poly = poly.getResampledByCount(numPoints);
                        }
                        for(int j = 0; j < poly.size(); j++){
                            x_tmp.push_back(poly[j].x);
                            y_tmp.push_back(poly[j].y);
                        }
                        x_tmp.push_back(-1);
                        y_tmp.push_back(-1);
                        poly.clear();
                    }else{
                        poly.lineTo(x->at(i), y->at(i));
                    }
                }
                if(poly.size() != 0){
                    if(withSpacing){
                        poly = poly.getResampledBySpacing(spacing);
                    }else{
                        poly = poly.getResampledByCount(numPoints);
                    }
                    for(int j = 0; j < poly.size(); j++){
                        x_tmp.push_back(poly[j].x);
                        y_tmp.push_back(poly[j].y);
                    }
                }
                y_out = y_tmp;
                x_out = x_tmp;
            }
		}));
		
		listeners.push(numPoints.newListener([this](int &i){
//			x_tmp.resize(i);
//			y_tmp.resize(i);
		}));
		
		x_tmp.resize(100);
		y_tmp.resize(100);
	}
	
private:
	ofParameter<vector<float>> x, y;
	vector<float> x_tmp, y_tmp;
	ofParameter<vector<float>> x_out, y_out;
    ofParameter<bool> withSpacing;
    ofParameter<float> spacing;
	ofParameter<int> numPoints;

	ofEventListeners listeners;
};

class dotMaker : public ofxOceanodeNodeModel {
public:
	dotMaker() : ofxOceanodeNodeModel("Dot Maker"){}
	
	void setup(){
		addParameter(x.set("X", {0.5}, {0}, {1}));
        addParameter(y.set("Y", {0.5}, {0}, {1}));
		addParameter(x_out.set("X_out", {0.5}, {0}, {1}));
        addParameter(y_out.set("Y_out", {0.5}, {0}, {1}));
		
		listeners.push(x.newListener([this](vector<float> &vf){
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
		}));
		
	}
	
private:
	ofParameter<vector<float>> x, y;
	vector<float> x_tmp, y_tmp;
	ofParameter<vector<float>> x_out, y_out;

	ofEventListeners listeners;
};

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
		
		listeners.push(xIn.newListener([this](vector<float> &vf){
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
							x_tmp[i] = ofClamp(x_tmp[i], 0, 1);
							y_tmp[i] = ofClamp(y_tmp[i], 0, 1);
						}
					}else{
						x_tmp[i] = -1;
						y_tmp[i] = -1;
					}
				}
                yOut = y_tmp;
				xOut = x_tmp;
			}
		}));
		
	}
	
private:
	ofParameter<vector<float>> xIn, yIn;
	ofParameter<int> replicate;
	ofParameter<vector<float>> translateX, translateY;
	ofParameter<vector<float>> scaleX, scaleY;
	ofParameter<vector<float>> rotate;
	
	vector<float> x_tmp, y_tmp;
	ofParameter<vector<float>> xOut, yOut;

	ofEventListeners listeners;
};

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
	}
	
	void update(ofEventArgs &a){
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
			for(float j = 0 ; j < 1 ; j = j + (1.0/(float)getParameterValueForPosition(sides, i))){
				float jj = j + getParameterValueForPosition(rotate, i);
				
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
			if(getParameterValueForPosition(toCenterFigure, i) != 0){
				ofPoint middleVertex = (newVertex+firstCreatedPoint) / 2;
				ofPoint toCenterPoint = (middleVertex * (1 - getParameterValueForPosition(toCenterFigure, i))) + (position * getParameterValueForPosition(toCenterFigure, i));
				//ADD Point
				x_temp.push_back(toCenterPoint.x);
				y_temp.push_back(toCenterPoint.y);
			}
			//Close Shape
            if(getParameterValueForPosition(sides, i) > 2 && closed){
                x_temp.push_back(firstCreatedPoint.x);
                y_temp.push_back(firstCreatedPoint.y);
            }
		}
		yOut = y_temp;
        xOut = x_temp;
	}
	
private:
	ofParameter<vector<int>> sides;
	ofParameter<vector<float>> toCenterFigure;
	ofParameter<vector<float>> translateX, translateY;
	ofParameter<vector<float>> scaleX, scaleY;
	ofParameter<vector<float>> rotate;
    
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

class pointMatrix : public ofxOceanodeNodeModel{
public:
    pointMatrix() : ofxOceanodeNodeModel("Point Matrix"){};
    ~pointMatrix(){};
    
    void setup() override{
        addParameter(sizeX.set("width", 10, 1, 1000));
        addParameter(sizeY.set("height", 10, 1, 1000));
		addParameter(x.set("X", {0.5}, {0}, {1}));
		addParameter(y.set("Y", {0.5}, {0}, {1}));
        
        listeners.push(sizeX.newListener([this](int &i){
            calcOutput();
        }));
        listeners.push(sizeY.newListener([this](int &i){
            calcOutput();
        }));
    }
    
private:
    ofEventListeners listeners;
    
    void calcOutput(){
        vector<float> tempX(sizeX*sizeY);
		vector<float> tempY(sizeX*sizeY);
        for(int i = 0; i < sizeY; i++){
            for(int j = 0 ; j < sizeX; j++){
                tempX[(i*sizeX) + j] = (j+0.5)/sizeX;
				tempY[(i*sizeX) + j] = (i+0.5)/sizeY;
            }
        }
        y = tempY;
        x = tempX;
    }
    
    ofParameter<int> sizeX;
    ofParameter<int> sizeY;
    ofParameter<vector<float>> x;
	ofParameter<vector<float>> y;
};

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

class radialModulation : public ofxOceanodeNodeModel{
public:
    radialModulation() : ofxOceanodeNodeModel("Radial Modulation"){}
    
    void setup(){
        addParameter(xIn.set("X.In", {0.5}, {0}, {1}));
        addParameter(yIn.set("Y.In", {0.5}, {0}, {1}));
        addParameter(modulator.set("Modulator", {0}, {0}, {1}));
        addParameter(amount.set("Amnt", {0}, {0}, {1}));
        addParameter(xCenter.set("X.Center", {0.5}, {0}, {1}));
        addParameter(yCenter.set("Y.Center", {0.5}, {0}, {1}));
        addParameter(xOut.set("X.Out", {0.5}, {0}, {1}));
        addParameter(yOut.set("Y.Out", {0.5}, {0}, {1}));
        
        addInspectorParameter(modulateType.set("Type ? radial : rotational", false));
        
        listeners.push(xIn.newListener([this](vector<float> &vf){
            auto getValueForIndex = [](const vector<float> &vf, int i, int currentShape, int numShapes) -> float{
                if(vf.size() == numShapes){
                    return vf[currentShape];
                }else if(i < vf.size()){
                    return vf[i];
                }else{
                    return vf[0];
                }
            };
            
            
            if(xIn->size() == yIn->size()){
                int numShapes = 1;
                for(int i = 0; i < xIn->size()-1; i++){ //Skip last in case is a -1
                    if(xIn->at(i) == -1){
                        numShapes++;
                    }
                }

                x_tmp = xIn;
                y_tmp = yIn;
                int currentShape = 0;
                int realI = 0;
                for(int i = 0; i < xIn->size(); i++){
                    if(xIn->at(i) != -1){
                        float modulationValue = getValueForIndex(modulator, realI, currentShape, numShapes);
                        float amountValue = getValueForIndex(amount, realI, currentShape, numShapes);
                        glm::vec2 center(getValueForIndex(xCenter.get(), realI, currentShape, numShapes), getValueForIndex(yCenter.get(), realI, currentShape, numShapes));
                        if(modulateType){
                            x_tmp[i] -= center.x;
                            y_tmp[i] -= center.y;
                            float x_copy = x_tmp[i];
                            x_tmp[i] = x_copy * cos((modulationValue-0.5) * amountValue * M_PI*2) - y_tmp[i] * sin((modulationValue-0.5) * amountValue * M_PI*2);
                            y_tmp[i] = x_copy * sin((modulationValue-0.5) * amountValue * M_PI*2) + y_tmp[i] * cos((modulationValue-0.5) * amountValue * M_PI*2);
                            x_tmp[i] += center.x;
                            y_tmp[i] += center.y;
                        }else{
                            glm::vec2 point(x_tmp[i], y_tmp[i]);
                            if(!(point.x == center.x && point.y == center.y)){
                                point = point + (glm::normalize(point - center)) * (modulationValue-0.5) * amountValue;
                            }
                            x_tmp[i] = point.x;
                            y_tmp[i] = point.y;
                        }
                        realI++;
                    }else{
                        currentShape++;
                    }
                }
                yOut = y_tmp;
                xOut = x_tmp;
            }
        }));
    }
    
    void presetHasLoaded(){
        xIn = xIn;
    }
    
private:
    ofEventListeners listeners;
    ofParameter<vector<float>> xIn, yIn, xOut, yOut, amount, xCenter, yCenter, modulator;
    ofParameter<bool> modulateType;
    vector<float> x_tmp, y_tmp;
};

class shapeInterpolator : public ofxOceanodeNodeModel{
public:
    shapeInterpolator() : ofxOceanodeNodeModel("Shape Interp"){}
    
    void setup(){
        addParameter(xA.set("X.a", {0.5}, {0}, {1}));
        addParameter(yA.set("Y.a", {0.5}, {0}, {1}));
        addParameter(xB.set("X.b", {0.5}, {0}, {1}));
        addParameter(yB.set("Y.b", {0.5}, {0}, {1}));
        addParameter(interp.set("Interp", 0, 0, 1));
        addParameter(x_out.set("X_out", {0.5}, {0}, {1}));
        addParameter(y_out.set("Y_out", {0.5}, {0}, {1}));
        
        listeners.push(xA.newListener([this](vector<float> &vf){
            if(xA->size() > 1 && xB->size() > 1 && xA->size() == yA->size() && xB->size() == yB->size()){
//                int numShapes = 1;
//                for(int i = 0; i < x->size()-1; i++){ //Skip last in case is a -1
//                    if(x->at(i) == -1){
//                        numShapes++;
//                    }
//                }
//
//                x_tmp.clear();
//                y_tmp.clear();
//                x_tmp.reserve((numPoints+1) * numShapes); //Add one for the -1
//                y_tmp.reserve((numPoints+1) * numShapes);
//
//                ofPolyline poly;
//                for(int i = 0; i < x->size(); i++){
//                    if(x->at(i) == -1){
//                        if(withSpacing){
//                            poly = poly.getResampledBySpacing(spacing);
//                        }else{
//                            poly = poly.getResampledByCount(numPoints);
//                        }
//                        for(int j = 0; j < poly.size(); j++){
//                            x_tmp.push_back(poly[j].x);
//                            y_tmp.push_back(poly[j].y);
//                        }
//                        x_tmp.push_back(-1);
//                        y_tmp.push_back(-1);
//                        poly.clear();
//                    }else{
//                        poly.lineTo(x->at(i), y->at(i));
//                    }
//                }
//                if(poly.size() != 0){
//                    if(withSpacing){
//                        poly = poly.getResampledBySpacing(spacing);
//                    }else{
//                        poly = poly.getResampledByCount(numPoints);
//                    }
//                    for(int j = 0; j < poly.size(); j++){
//                        x_tmp.push_back(poly[j].x);
//                        y_tmp.push_back(poly[j].y);
//                    }
//                }
//                y_out = y_tmp;
//                x_out = x_tmp;
                //https://github.com/veltman/flubber/blob/0cadadf3eb15cd5b2ec7c45c70601ff88b76a6c3/build/flubber.js#L2504
                auto addPoint = [](vector<float> &x, vector<float> &y, int iter){
                    glm::vec2 p1(x[iter*2], y[iter*2]);
                    glm::vec2 p2(x[(iter*2) + 1], y[(iter*2) + 1]);
                    glm::vec2 pout = glm::mix(p1, p2, 0.5);
                    x.insert(x.begin() + (iter*2) + 1, pout.x);
                    y.insert(y.begin() + (iter*2) + 1, pout.y);
                };
                
                x_tmp.resize(max(xA->size(), xB->size()));
                y_tmp.resize(max(xA->size(), xB->size()));
                
                xA_tmp = xA;
                yA_tmp = yA;
                xB_tmp = xB;
                yB_tmp = yB;
                if(xA_tmp.size() < xB_tmp.size()){
                    int iter = 0;
                    while(xA_tmp.size() != xB_tmp.size()){
                        addPoint(xA_tmp, yA_tmp, iter);
                        iter++;
                    }
                }else if(xA_tmp.size() > xB_tmp.size()){
                    int iter = 0;
                    while(xA_tmp.size() != xB_tmp.size()){
                        addPoint(xB_tmp, yB_tmp, iter);
                        iter++;
                    }
                }
                
                if(xA_tmp.size() == xB_tmp.size()){
//                    ofPolyline poly;
//                    for(int i = 0; i < xB->size(); i++){
//                        poly.lineTo(xB->at(i), yB->at(i));
//                    }
//                    for(int i = 0; i < xA->size(); i++){
//                        glm::vec3 pA = glm::vec3(xA->at(i), yA->at(i), 0);
//                        glm::vec3 pB = *std::max_element(poly.getVertices().begin(), poly.getVertices().end(), [pA](glm::vec3 &first, glm::vec3 &second){
//                            return glm::distance(pA, first) > glm::distance(pA, second);
//                        });
//
//                        x_tmp[i] = ofLerp(pA.x, pB.x, interp);
//                        y_tmp[i] = ofLerp(pA.y, pB.y, interp);
//                    }
                    
                    // Find the best rotation of A to go to B
                    // Code from https://github.com/veltman/flubber/blob/0cadadf3eb15cd5b2ec7c45c70601ff88b76a6c3/build/flubber.js#L2600
                    float sumOfSquares = 0;
                    float min = FLT_MAX;
                    int bestOffset = 0;
                    bool aClosed = xA_tmp.front() == xA_tmp.back() && yA_tmp.front() == yA_tmp.back();
                    bool bClosed = xB_tmp.front() == xB_tmp.back() && yB_tmp.front() == yB_tmp.back();
                    bool bothClosed = aClosed && bClosed;
                    
                    int xA_size = xA_tmp.size() - (bothClosed ? 1 : 0);
                    int xB_size = xB_tmp.size() - (bothClosed ? 1 : 0);
                    if(aClosed || bClosed){
                        for(int i = 0; i < xA_size; i++){
                            sumOfSquares = 0;
                            for(int j = 0; j < xB_size; j++){
                                glm::vec3 pA = glm::vec3(xA_tmp[(i + j) % xA_size], yA_tmp[(i + j) % xA_size], 0);
                                glm::vec3 pB = glm::vec3(xB_tmp[j], yB_tmp[j], 0);
                                float d = glm::distance(pA, pB);
                                sumOfSquares += (d * d);
                            }
                            
                            if (sumOfSquares < min) {
                                min = sumOfSquares;
                                bestOffset = i;
                            }
                        }
                    }

                    for(int i = 0; i < xA_size; i++){
                        int ii = (i + bestOffset) % xA_size;
                        glm::vec3 pA = glm::vec3(xA_tmp[ii], yA_tmp[ii], 0);
                        glm::vec3 pB = glm::vec3(xB_tmp[i], yB_tmp[i], 0);
                        if(bClosed){
                            x_tmp[i] = ofLerp(pA.x, pB.x, interp);
                            y_tmp[i] = ofLerp(pA.y, pB.y, interp);
                        }else{
                            x_tmp[ii] = ofLerp(pA.x, pB.x, interp);
                            y_tmp[ii] = ofLerp(pA.y, pB.y, interp);
                        }
                    }
                    if(bothClosed){
                        x_tmp.back() = x_tmp.front();
                        y_tmp.back() = y_tmp.front();
                    }
                }
                y_out = y_tmp;
                x_out = x_tmp;
            }
        }));
        
//        x_tmp.resize(100);
//        y_tmp.resize(100);
        //        getClosestPoint();
    }
    
private:
    ofParameter<vector<float>> xA, yA, xB, yB;
    vector<float> xA_tmp, yA_tmp, xB_tmp, yB_tmp, x_tmp, y_tmp;
    ofParameter<vector<float>> x_out, y_out;
    ofParameter<float> interp;

    ofEventListeners listeners;
};

class pointAtPercent : public ofxOceanodeNodeModel{
public:
    pointAtPercent() : ofxOceanodeNodeModel("Point At Pct"){}
    
    void setup(){
        addParameter(x_in.set("X.In", {0.5}, {0}, {1}));
        addParameter(y_in.set("Y.In", {0.5}, {0}, {1}));
        addParameter(pcts.set("Pcts", {0}, {0}, {1}));
        addOutputParameter(x_out.set("X.Out", {0.5}, {0}, {1}));
        addOutputParameter(y_out.set("Y_Out", {0.5}, {0}, {1}));
        
        listener = pcts.newListener([this](vector<float> &vf){
            if(x_in->size() == y_in->size() && x_in->size() > 1) {
                auto iterbegin = x_in->begin();
                auto iterend = std::find(x_in->begin(), x_in->end(), -1);
                int i = 0;
                vector<float> x_tmp, y_tmp;
                while (iterend != x_in->end()){
                    ofPolyline poly;
                    int beginIndex = iterbegin - x_in->begin();
                    for(int j = 0; j < iterend - iterbegin; j++){
                        poly.addVertex(glm::vec3(x_in.get()[beginIndex+j], y_in.get()[beginIndex+j], 0));
                    }
                    iterbegin = iterend+1;
                    iterend = std::find(iterbegin, x_in->end(), -1);
                    
                    if(vf.size() > i && vf[i] >= 0 && vf[i] <= 1){
                        auto p = poly.getPointAtPercent(vf[i]);
                        x_tmp.push_back(p.x);
                        y_tmp.push_back(p.y);
                    }
                    i++;
                }
                ofPolyline poly;
                int beginIndex = iterbegin - x_in->begin();
                for(int j = 0; j < iterend - iterbegin; j++){
                    poly.addVertex(glm::vec3(x_in.get()[beginIndex+j], y_in.get()[beginIndex+j], 0));
                }
                if(vf.size() > i && vf[i] >= 0 && vf[i] <= 1){
                    auto p = poly.getPointAtPercent(vf[i]);
                    x_tmp.push_back(p.x);
                    y_tmp.push_back(p.y);
                }
                
                y_out = y_tmp;
                x_out = x_tmp;
            }
        });
    }
    
private:
    ofParameter<vector<float>> x_in, y_in, pcts, x_out, y_out;
    
    ofEventListener listener;
};

#endif /* fatlineTools_h */
