//
//  manualPoints.h
//  example
//
//  Created by Eduard Frigola Bagué on 11/02/2021.
//

#ifndef manualPoints_h
#define manualPoints_h

#include "ofxOceanodeNodeModel.h"

class manualPoints : public ofxOceanodeNodeModel{
public:
    manualPoints() : ofxOceanodeNodeModel("Manual Points"){
        description = "Interactive window opens where you can create points and drag them around a [0,0] to [1,1] space.\nRight-Click to create or delete points.\nLeft-click to drag points.\nKeyboard arrows move the selected point. (+alt moves very slowly).\nTAB key switches between selected points.\n\nInput is a texture to apply as background to the window area.";
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

#endif /* manualPoints_h */
