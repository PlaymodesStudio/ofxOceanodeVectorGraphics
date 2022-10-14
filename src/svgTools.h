//
//  svgTools.h
//  example
//
//  Created by Eduard Frigola Bagué on 15/10/2020.
//

#ifndef svgTools_h
#define svgTools_h

#include "ofxOceanodeNodeModel.h"
#include "ofxFatLine.h"
#include "ofxSvg.h"

class svgModulator : public ofxOceanodeNodeModel{
public:
    svgModulator() : ofxOceanodeNodeModel("SVG Modulator"){};
    
    void setup(){
        addParameter(filename.set("File", ""));
        addParameter(width.set("Width", 100, 1, 5000), ofxOceanodeParameterFlags_DisableSavePreset);
        addParameter(height.set("Height", 100, 1, 5000), ofxOceanodeParameterFlags_DisableSavePreset);
        addParameter(sizes.set("Sizes", {0}, {0}, {INT_MAX}));
        addOutputParameter(x_out.set("X.Out", {0.5}, {0}, {1}));
        addOutputParameter(y_out.set("Y.Out", {0.5}, {0}, {1}));

        
        listener = filename.newListener([this](string &path){
            svg.load(path);
            int numPaths = svg.getNumPath();
            
            glm::vec2 size(svg.getWidth(), svg.getHeight());
            width = size.x;
            height = size.y;
            
            vector<float> x_tmp;
            vector<float> y_tmp;
            vector<int> sizes_tmp;
            for(int i = 0; i < numPaths; i++){
//                ofPolyline p;
//                for(auto &c : svg.getPathAt(i).getCommands()){
//                    if(c.type == ofPath::Command::moveTo){
//                        p.addVertex(c.to);
//                    }
//                    else if(c.type == ofPath::Command::lineTo){
//                        if(c.to != glm::vec3(0, 0, 0))
//                        p.lineTo(c.to);
//                    }
//                    else if(c.type == ofPath::Command::curveTo){
//                        if(c.to != glm::vec3(0, 0, 0))
//                        p.curveTo(c.to);
//                    }
//                    else if(c.type == ofPath::Command::bezierTo){
//                        if(c.to != glm::vec3(0, 0, 0))
//                        p.bezierTo(c.to, c.cp1, c.cp2);
//                    }
//                    else if(c.type == ofPath::Command::quadBezierTo){
//                        if(c.to != glm::vec3(0, 0, 0))
//                        p.quadBezierTo(c.to, c.cp1, c.cp2);
//                    }
//                    else if(c.type == ofPath::Command::arc){
//                        if(c.to != glm::vec3(0, 0, 0))
//                        p.arc(c.to, c.);
//                    }
//                    else if(c.type == ofPath::Command::arcNegative){
//                        if(c.to != glm::vec3(0, 0, 0))
//                        p.addVertex(c.to);
//                    }
//                    else if(c.type == ofPath::Command::close){
//                        p.addVertex(p.getVertices()[0]);
//                    }
//                    else{
//                        ofLog() << "unprocessed command";
//                        p.addVertex(c.to);
//                    }
//                }
//                if(svg.getPathAt(i).isClosed())
//                    tempOut[i].addVertex(tempOut[i].getVertices()[0]);
                
                
                for(auto &v : svg.getPathAt(i).getOutline()){
                    for(auto &p : v){
                        x_tmp.push_back(p.x);
                        y_tmp.push_back(p.y);
                    }
                    sizes_tmp.push_back(v.size());
                    x_tmp.push_back(-1);
                    y_tmp.push_back(-1);
                }
            }
            sizes = sizes_tmp;
            y_out = y_tmp;
            x_out = x_tmp;
        });
    }
    
private:
    ofEventListener listener;
    
    ofxSVG svg;
    ofParameter<int> width, height;
    ofParameter<string> filename;
    ofParameter<vector<float>> x_out, y_out;
    ofParameter<vector<int>> sizes;
};

#endif /* svgTools_h */
