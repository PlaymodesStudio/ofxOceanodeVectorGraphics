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
        if(x->size() == y->size()){
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
        }else if(x->size() > y->size()){
			points.resize(1);
            points[0].resize(close ? x->size() + 1 : x->size());
            for(int i = 0; i < points[0].size(); i++){
                points[0][i] = glm::vec3(x.get()[i] * width, y.get()[0] * height, 0);
            }
        }else if(x->size() < y->size()){
			points.resize(1);
            points[0].resize(close ? y->size() + 1 : y->size());
            for(int i = 0; i < points[0].size(); i++){
                points[0][i] = glm::vec3(x.get()[0] * width, y.get()[i] * height, 0);
            }
        }
		
//		vector<double> doubleWeights;
//        if(w->size() != points.size()){
//            doubleWeights = (vector<double>(points.size(), w->at(0)));
//        }else{
//            doubleWeights = vector<double>(w->begin(), w->end());
//        }
		if(points.back().size() == 0) points.pop_back();
        
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
			accumPosition += points[i].size();
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
			x = x_t;
			y = y_t;
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
	
	int pointDraggingIndex = -1;
};

class pointsResampler : public ofxOceanodeNodeModel {
public:
	pointsResampler() : ofxOceanodeNodeModel("Points Resampler"){}
	
	void setup(){
		addParameter(x.set("X", {0.5}, {0}, {1}));
        addParameter(y.set("Y", {0.5}, {0}, {1}));
		addParameter(numPoints.set("Num", {100}, {1}, {INT_MAX}));
		addParameter(x_out.set("X_out", {0.5}, {0}, {1}));
        addParameter(y_out.set("Y_out", {0.5}, {0}, {1}));
		
		listeners.push(x.newListener([this](vector<float> &vf){
			if(x->size() == y->size()){
				ofPolyline poly;
				for(int i = 0; i < x->size(); i++){
					poly.lineTo(x->at(i), y->at(i));
				}
				poly = poly.getResampledByCount(numPoints);
				for(int i = 0; i < numPoints; i++){
					auto point = poly[i];
					x_tmp[i] = point.x;
					y_tmp[i] = point.y;
				}
				x_out = x_tmp;
				y_out = y_tmp;
			}
		}));
		
		listeners.push(numPoints.newListener([this](int &i){
			x_tmp.resize(i);
			y_tmp.resize(i);
		}));
		
		x_tmp.resize(100);
		y_tmp.resize(100);
		
//		listeners.push(y.newListener([this](vector<float> &vf){
//
//		}));
	}
	
private:
	ofParameter<vector<float>> x, y;
	vector<float> x_tmp, y_tmp;
	ofParameter<vector<float>> x_out, y_out;
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
				x_out = x_tmp;
				y_out = y_tmp;
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
				x_out = x_tmp;
				y_out = y_tmp;
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
				x_out = x_tmp;
				y_out = y_tmp;
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
				xOut = x_tmp;
				yOut = y_tmp;
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

#endif /* fatlineTools_h */
