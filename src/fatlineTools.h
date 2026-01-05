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
    manualPoints() : ofxOceanodeNodeModel("Manual Points"){
        description = "Interactive window opens where you can create points and drag them around a [0,0] to [1,1] space.\nRight-Click to create or delete points.\nLeft-click to drag points.\nKeyboard arrows move the selected point. (+alt moves very slowly).\nTAB key switches between selected points.\n\nInput is a texture to apply as background to the window area.";
    }
    
    void updatePoints()
    {
        if(x->size() == y->size() && !outputingValues){
            points.clear();
            points.resize(x->size());
            for(int i = 0; i < points.size(); i++){
                    points[i] == glm::vec2(x->at(i), y->at(i));
            }
        }
    }
    
    void setup(){
        addParameter(showWindow.set("Show",false));
        addParameter(originLow.set("OrigPos",false));
        addParameter(input.set("Background", nullptr));
        addOutputParameter(mouseX.set("M.x", 0.5, 0, 1));
        addOutputParameter(mouseY.set("M.y", 0.5, 0, 1));
        addOutputParameter(numPoints.set("Num", {1}, {1}, {INT_MAX}));
        addOutputParameter(x.set("X", {0.5}, {0}, {1}));
        addOutputParameter(y.set("Y", {0.5}, {0}, {1}));

        listeners.push(x.newListener([this](vector<float> &vf){
            updatePoints();
        }));
        
        listeners.push(y.newListener([this](vector<float> &vf){
            //updatePoints();
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
    
    void draw(ofEventArgs &a) override{
        if(showWindow)
        {
            if(ImGui::Begin(("Manual Points " + ofToString(getNumIdentifier())).c_str())){
                if(ImGui::Button("[Reset Points]"))
                {
                    for(int i=0;i<x.get().size();i++)
                    {
                        points[i]=glm::vec2(0.5,0.5);
                    }
                }
                ImGui::SameLine();
                if(keepAspectRatio)ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0,0.5,0.0,1.0));
                else ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55,0.55,0.55,1.0));
                if(ImGui::Button("[AR]"))
                {
                    if(ImGui::IsMouseReleased(0)) keepAspectRatio=!keepAspectRatio;
                }
                ImGui::PopStyleColor();
                
                auto screenSize = ImGui::GetContentRegionAvail();
                if(keepAspectRatio)
                {
                    ImVec2 sizeAux = screenSize;
                    if(screenSize.y>screenSize.x) sizeAux.y = sizeAux.x;
                    else sizeAux.x = screenSize.y;
                    screenSize=sizeAux;
                }
                
                auto screenPos = ImGui::GetCursorScreenPos();
                //ImGui::Vec2 size;
                
                ImTextureID textureID = (ImTextureID)(uintptr_t)black.texData.textureID;
                if(input.get() != nullptr){
                    textureID = (ImTextureID)(uintptr_t)input.get()->texData.textureID;
                }
                ImGui::Image(textureID, screenSize);
                if(ImGui::IsWindowHovered()){
                    glm::vec2 normPos = (ImGui::GetMousePos() - screenPos) / screenSize;
                    if(normPos.x>1.0) normPos.x = 1.0;
                    if(normPos.x<0.0) normPos.x = 0.0;
                    if(normPos.y>1.0) normPos.y = 1.0;
                    if(normPos.y<0.0) normPos.y = 0.0;

                    bool addRemove = false;
                    bool mouseClicked = false;
                    if(ImGui::IsMouseClicked(0)){
                        mouseClicked = true;
                    }
                    else if(ImGui::IsMouseClicked(1)){
                        mouseClicked = true;
                        addRemove = true;
                    }
                    if(mouseClicked){
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
                                pointDraggingIndex = points.size()-1;
                            }
                        }else if(!foundPoint){
                            pointDraggingIndex = -1;
                        }
                    }
                    else if(ImGui::IsMouseDragging(0)){
                        if(pointDraggingIndex != -1){
                            if(ImGui::GetIO().KeyAlt){
                                points[pointDraggingIndex] += ImGui::GetIO().MouseDelta / (screenSize * ImVec2(100, 100));
                            }else{
                                points[pointDraggingIndex] += ImGui::GetIO().MouseDelta / screenSize;
                            }
                        }else if(ImGui::GetIO().KeyShift){
                            points.push_back((ImGui::GetMousePos() - screenPos) / screenSize);
                        }
                    }else if(pointDraggingIndex != -1){
                        float moveAmt = ImGui::GetIO().KeyAlt ? 0.00001 : 0.001;
                        if(ImGui::IsKeyDown(ImGuiKey_LeftArrow)){
                            points[pointDraggingIndex] += glm::vec2(-moveAmt, 0);
                        }else if(ImGui::IsKeyDown(ImGuiKey_RightArrow)){
                            points[pointDraggingIndex] += glm::vec2(moveAmt, 0);
                        }if(ImGui::IsKeyDown(ImGuiKey_UpArrow)){
                            points[pointDraggingIndex] += glm::vec2(0, -moveAmt);
                        }else if(ImGui::IsKeyDown(ImGuiKey_DownArrow)){
                            points[pointDraggingIndex] += glm::vec2(0, moveAmt);
                        }
                    }
                    if(ImGui::IsKeyPressed(ImGuiKey_Tab)){
                        pointDraggingIndex = (pointDraggingIndex + 1) % points.size();
                    }
                    if(pointDraggingIndex!=-1)
					{
						if(points[pointDraggingIndex].x>1.0) points[pointDraggingIndex].x=1.0;
						if(points[pointDraggingIndex].x<0.0) points[pointDraggingIndex].x=0.0;
						if(points[pointDraggingIndex].y>1.0) points[pointDraggingIndex].y=1.0;
						if(points[pointDraggingIndex].y<0.0) points[pointDraggingIndex].y=0.0;
						
						mouseY = normPos.y;
						mouseX = normPos.x;
					}
                }
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                vector<float> x_t(points.size());
                vector<float> y_t(points.size());
                for(int i = 0; i < points.size(); i++){
                    if(pointDraggingIndex == i) draw_list->AddCircle((points[i] * screenSize) + screenPos, 10, IM_COL32(255, 255, 0, 255));
                    else draw_list->AddCircle((points[i] * screenSize) + screenPos, 10, IM_COL32(255, 128, 0, 255));
                    string numString = ofToString(i);
                    draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), (points[i] * screenSize) + screenPos - glm::vec2(5, 5), IM_COL32(255,255,255,255), numString.c_str(), numString.c_str()+ (i < 10 ? 1 : 2));
                    x_t[i] = points[i].x;
                    if(!originLow)
                    {
                        y_t[i] = points[i].y;
                    }
                    else
                    {
                        y_t[i] = -1*(points[i].y-1.0);
                    }
                }
                if(!originLow)
                {
                    draw_list->AddCircleFilled(screenPos,5, IM_COL32(255, 128, 0, 128));
                }
                else
                {
                    draw_list->AddCircleFilled(screenPos+ImVec2(0,screenSize.y),5, IM_COL32(255, 128, 0, 128));
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
    ofParameter<float> mouseX, mouseY;
	ofParameter<int> numPoints;
    ofParameter<bool> originLow;
    ofParameter<bool> showWindow;
	vector<glm::vec2> points;
    
    ofEventListeners listeners;
	
	int pointDraggingIndex = -1;
    bool outputingValues = false;
    bool keepAspectRatio = true;
};

class pointsResamplerIndex : public ofxOceanodeNodeModel {
public:
    pointsResamplerIndex() : ofxOceanodeNodeModel("Points Resampler Index"){}
    
    void setup(){
        addParameter(x.set("X", {0.5}, {0}, {1}));
        addParameter(y.set("Y", {0.5}, {0}, {1}));
        addParameter(numPoints.set("Num", {100}, {1}, {INT_MAX}));
        addParameter(withSpacing.set("Use Spa", false));
        addParameter(spacing.set("Spacing", 0.1, 0, 1));
        addParameter(x_out.set("X_out", {0.5}, {0}, {1}));
        addParameter(y_out.set("Y_out", {0.5}, {0}, {1}));
        addParameter(index_out.set("Index_out", {0}, {0}, {INT_MAX}));
        
        listeners.push(x.newListener([this](vector<float> &vf){
            if(x->size() == y->size()){
                int numSegments = x->size()/2;

                // Compute the length of each segment
                vector<float> lengths(numSegments);
                float totalLength = 0;
                for(int i = 0; i < numSegments; i++){
                    float dx = x->at(2*i+1) - x->at(2*i);
                    float dy = y->at(2*i+1) - y->at(2*i);
                    lengths[i] = sqrt(dx*dx + dy*dy);
                    totalLength += lengths[i];
                }
                
                x_tmp.clear();
                y_tmp.clear();
                index_tmp.clear();
                
                ofPolyline poly;
                for(int i = 0; i < numSegments; i++){
                    poly.lineTo(x->at(2*i), y->at(2*i));
                    poly.lineTo(x->at(2*i+1), y->at(2*i+1));
                    
                    // Determine the number of points for this segment
                    int segmentPoints = round(numPoints * lengths[i] / totalLength);
                    if(withSpacing){
                        poly = poly.getResampledBySpacing(spacing);
                    }else{
                        poly = poly.getResampledByCount(segmentPoints);
                    }
                    
                    for(auto & point : poly){
                        x_tmp.push_back(point.x);
                        y_tmp.push_back(point.y);
                        index_tmp.push_back(i);
                    }
                    poly.clear();
                }
                x_out = x_tmp;
                y_out = y_tmp;
                index_out = index_tmp;
            }
        }));
        
        x_tmp.resize(100);
        y_tmp.resize(100);
        index_tmp.resize(100);
    }
    
private:
    ofParameter<vector<float>> x, y;
    vector<float> x_tmp, y_tmp, index_tmp;
    ofParameter<vector<float>> x_out, y_out, index_out;
    ofParameter<bool> withSpacing;
    ofParameter<float> spacing;
    ofParameter<int> numPoints;

    ofEventListeners listeners;
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
            if(vf.size() == 0)
            {
                x_out.set(vector<float>());
                y_out.set(vector<float>());
                return; // Exit
            }

            if(x->size() == y->size() && x->size() > 0){
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
                            for(int j = 0; j < poly.size(); j++){
                                x_tmp.push_back(poly[j].x);
                                y_tmp.push_back(poly[j].y);
                            }
                        }else{
                            //From: https://github.com/openframeworks/openFrameworks/issues/3557
                            float jump = 1.0f/ (numPoints-1);
                            for (int j = 0; j < numPoints; j++) {
                                auto p = poly.getPointAtPercent(min(j*jump, 0.99999f));
                                x_tmp.push_back(p.x);
                                y_tmp.push_back(p.y);
                            }
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
                        for(int j = 0; j < poly.size(); j++){
                            x_tmp.push_back(poly[j].x);
                            y_tmp.push_back(poly[j].y);
                        }
                    }else{
                        float jump = 1.0f/ (numPoints-1);
                        for (int j = 0; j < numPoints; j++) {
                            auto p = poly.getPointAtPercent(min(j*jump, 0.99999f));
                            x_tmp.push_back(p.x);
                            y_tmp.push_back(p.y);
                        }
                    }
                    x_tmp.push_back(-1);
                    y_tmp.push_back(-1);
                }
                y_out = y_tmp;
                x_out = x_tmp;
            }
        }));
        
        listeners.push(numPoints.newListener([this](int &i){
//            x_tmp.resize(i);
//            y_tmp.resize(i);
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

class shapeFeather : public ofxOceanodeNodeModel{
public:
    shapeFeather() : ofxOceanodeNodeModel("Shape Feather"){}
    
    void setup(){
        addParameter(x_in.set("X.In", {0.5}, {0}, {1}));
        addParameter(y_in.set("Y.In", {0.5}, {0}, {1}));
        addParameter(amount.set("Amount", 0, 0, 1));
        addOutputParameter(output.set("Feather", {0}, {0}, {1}));
        
        listener = x_in.newListener([this](vector<float> &vf){
            if(x_in->size() == y_in->size() && x_in->size() > 1) {
                auto iterbegin = x_in->begin();
                auto iterend = std::find(x_in->begin(), x_in->end(), -1);
                int i = 0;
                vector<float> out_tmp;
                while (iterend != x_in->end()){
                    int beginIndex = iterbegin - x_in->begin();
                    for(int j = 0; j < iterend - iterbegin; j++){
                        float x = x_in.get()[beginIndex+j];
                        float y = y_in.get()[beginIndex+j];
                        // Calculate the distance from the point to each of the four edges
                        double distToLeft = x;
                        double distToRight = 1 - x;
                        double distToTop = y;
                        double distToBottom = 1 - y;
                        
                        // Determine the minimum distance to the edge
                        double minDistToEdge = std::min({distToLeft, distToRight, distToTop, distToBottom});
                        
                        // If the point is within the feathering distance to an edge,
                        // calculate the feathering amount (output)
                        double output;
                        if (minDistToEdge <= amount) {
                            out_tmp.push_back(minDistToEdge / amount);
                        } else {
                            out_tmp.push_back(1.0);  // The point is not within the feathering distance to an edge
                        }
                        
                    }
                    iterbegin = iterend+1;
                    iterend = std::find(iterbegin, x_in->end(), -1);
                    i++;
                }
                int beginIndex = iterbegin - x_in->begin();
                for(int j = 0; j < iterend - iterbegin; j++){
                    float x = x_in.get()[beginIndex+j];
                    float y = y_in.get()[beginIndex+j];
                    // Calculate the distance from the point to each of the four edges
                    double distToLeft = x;
                    double distToRight = 1 - x;
                    double distToTop = y;
                    double distToBottom = 1 - y;
                    
                    // Determine the minimum distance to the edge
                    double minDistToEdge = std::min({distToLeft, distToRight, distToTop, distToBottom});
                    
                    // If the point is within the feathering distance to an edge,
                    // calculate the feathering amount (output)
                    double output;
                    if (minDistToEdge <= amount) {
                        out_tmp.push_back(minDistToEdge / amount);
                    } else {
                        out_tmp.push_back(1.0);  // The point is not within the feathering distance to an edge
                    }
                }
            
                output = out_tmp;
            }
        });
    }
    
private:
    ofParameter<vector<float>> x_in, y_in;
    ofParameter<float> amount;
    ofParameter<vector<float>> output;
    
    ofEventListener listener;
};

class shapeLength : public ofxOceanodeNodeModel{
public:
    shapeLength() : ofxOceanodeNodeModel("Shape Length"){}
    
    void setup(){
        addParameter(x_in.set("X.In", {0.5}, {0}, {1}));
        addParameter(y_in.set("Y.In", {0.5}, {0}, {1}));
        addOutputParameter(length.set("Length", {0}, {0}, {FLT_MAX}));
        
        listener = x_in.newListener([this](vector<float> &vf){
            if(x_in->size() == y_in->size() && x_in->size() > 1) {
                auto iterbegin = x_in->begin();
                auto iterend = std::find(x_in->begin(), x_in->end(), -1);
                int i = 0;
                vector<float> length_tmp;
                while (iterend != x_in->end()){
                    int beginIndex = iterbegin - x_in->begin();
                    float dist = 0;
                    for(int j = 0; j < iterend - iterbegin - 1; j++){
                        float x0 = x_in.get()[beginIndex+j];
                        float y0 = y_in.get()[beginIndex+j];
                        float x1 = x_in.get()[beginIndex+j+1];
                        float y1 = y_in.get()[beginIndex+j+1];
                        dist += glm::distance(glm::vec2(x0, y0), glm::vec2(x1, y1));
                    }
                    length_tmp.push_back(dist);
                    iterbegin = iterend+1;
                    iterend = std::find(iterbegin, x_in->end(), -1);
                    i++;
                }
                int beginIndex = iterbegin - x_in->begin();
                float dist = 0;
                for(int j = 0; j < iterend - iterbegin; j++){
                    float x0 = x_in.get()[beginIndex+j];
                    float y0 = y_in.get()[beginIndex+j];
                    float x1 = x_in.get()[beginIndex+j+1];
                    float y1 = y_in.get()[beginIndex+j+1];
                    dist += glm::distance(glm::vec2(x0, y0), glm::vec2(x1, y1));
                }
                if(dist > 0)
                    length_tmp.push_back(dist);
                length = length_tmp;
            }
        });
    }
    
private:
    ofParameter<vector<float>> x_in, y_in;
    ofParameter<vector<float>> length;
    
    ofEventListener listener;
};


class borderMask : public ofxOceanodeNodeModel {
public:
    borderMask() : ofxOceanodeNodeModel("Border Mask"){}
    
    void setup(){
        addParameter(input.set("Input", {ofxFatLine()}));
        addParameter(affectAlpha.set("Alpha?", true));
        addParameter(maskPosition.set("Mask Pos", 0.0625, 0, 0.5)); // 0.0625 = 50/800 normalized
        addParameter(maskWidth.set("Mask Width", 0.025, 0, 0.5));  // 0.025 = 20/800 normalized
        addParameter(output.set("Output", {ofxFatLine()}));
        
        listener = input.newListener([this](vector<ofxFatLine> &lines){
            if(maskPosition!=0.0)
            {
                vector<ofxFatLine> outputLines;
                
                for(auto &line : lines){
                    ofxFatLine newLine;
                    const auto &vertices = line.getVertices();
                    vector<double> weights(vertices.size(), 1.0);
                    auto colors = line.getColors();
                    
                    for(size_t i = 0; i < vertices.size(); i++){
                        const glm::vec3 &vertex = vertices[i];
                        
                        // Keep everything in normalized space [0,1]
                        float normalizedX = vertex.x / 800.0f;
                        float normalizedY = vertex.y / 800.0f;
                        
                        // Calculate minimum distance to any border in normalized space
                        float distToLeft = normalizedX;
                        float distToRight = 1.0f - normalizedX;
                        float distToTop = normalizedY;
                        float distToBottom = 1.0f - normalizedY;
                        
                        float minDist = min(min(distToLeft, distToRight),
                                            min(distToTop, distToBottom));
                        
                        // Calculate mask factor using linear interpolation
                        float factor = 1.0f;
                        float innerEdge = maskPosition - maskWidth;
                        
                        if(minDist <= maskPosition) {
                            if(minDist <= innerEdge) {
                                factor = 0.0f; // Fully masked
                            } else {
                                // Linear interpolation between inner edge and mask position
                                factor = (minDist - innerEdge) / maskWidth;
                            }
                            
                            colors[i].r *= factor;
                            colors[i].g *= factor;
                            colors[i].b *= factor;
                            if(affectAlpha) {
                                colors[i].a *= factor;
                            }
                        }
                        
                        weights[i] = line.getWeight(i);
                    }
                    
                    newLine.add(vertices, colors, weights);
                    outputLines.push_back(newLine);
                }
                output = outputLines;
            }
        });
    }
    
private:
    ofParameter<vector<ofxFatLine>> input;
    ofParameter<float> maskPosition;
    ofParameter<float> maskWidth;
    ofParameter<bool> affectAlpha;
    ofParameter<vector<ofxFatLine>> output;
    
    ofEventListener listener;
};

#endif /* fatlineTools_h */

