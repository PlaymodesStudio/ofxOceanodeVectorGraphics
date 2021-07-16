//
//  svgTools.h
//  example
//
//  Created by Eduard Frigola Bagué on 15/10/2020.
//

#ifndef svgTools_h
#define svgTools_h

#include "ofxOceanodeNodeModel.h"
//#include "ofxFatLine.h"
#include "ofxSvg.h"

class svgModulator : public ofxOceanodeNodeModel{
public:
    svgModulator() : ofxOceanodeNodeModel("SVG Modulator"){};
    
    void setup(){
        addParameter(filename.set("File", ""));
        addParameter(width.set("Width", 100, 1, 5000), ofxOceanodeParameterFlags_DisableSavePreset);
        addParameter(height.set("Height", 100, 1, 5000), ofxOceanodeParameterFlags_DisableSavePreset);
        addParameter(resample.set("Resample", 100, 2, 1000));
        addParameter(texModulator.set("TMod", nullptr));
        addParameter(output.set("Output", {ofxFatLine()}));
        
        listener = filename.newListener([this](string &path){
            svg.load(path);
            int numPaths = svg.getNumPath();
            ofLog() << numPaths;
            tempOut.resize(numPaths);
            
            glm::vec2 size(svg.getWidth(), svg.getHeight());
            width = size.x;
            height = size.y;
            
            for(int i = 0; i < numPaths; i++){
                ofPolyline p;
                for(auto &c : svg.getPathAt(i).getCommands()){
                    if(c.type == ofPath::Command::moveTo){
                        p.addVertex(c.to);
                    }
                    else if(c.type == ofPath::Command::lineTo){
                        if(c.to != glm::vec3(0, 0, 0))
                        p.addVertex(c.to);
                    }
                    else if(c.type == ofPath::Command::close){
                        p.addVertex(p.getVertices()[0]);
                    }
                    else{
                        ofLog() << "unprocessed command";
                        p.addVertex(c.to);
                    }
                }
//                if(svg.getPathAt(i).isClosed())
//                    tempOut[i].addVertex(tempOut[i].getVertices()[0]);
                //p = p.getResampledByCount(100);
                //while(p.size() > 100) p.removeVertex(p.size()-1);
                tempOut[i].setGlobalWidth(10);
                tempOut[i].setFromPolyline(p);
            }
            output = tempOut;
            
//            ofPolyline p;
//            for(auto &c : svg.getPathAt(0).getCommands()){
//                if(c.to != glm::vec3(0, 0, 0))
//                    p.addVertex(c.to / size);
//            }
//            p.addVertex(p.getVertices()[0]);
//            resample = p.getVertices().size();
////            p = p.getResampledByCount(100);
////            while(p.size() > 100) p.removeVertex(p.size()-1);
//            output = p;
        });
        
        
        listener2 = resample.newListener([this](int &i){
//            ofPolyline p;
//            p = output->getResampledByCount(i);
//            while(p.size() > i) p.removeVertex(p.size()-1);
//            output = p;
        });
    }
    
    void update(ofEventArgs &a){
    //        if(modulator->size() == 100){
    //            for (int i = 0; i < 100 ; i++){
    //                auto c = ofFloatColor(modulator->at(i));
    //                for (int j = 0; j < svg.getNumPath() ; j++)
    //                    tempOut[j].updateColor(i, c);
    //            }
    //        }
            
            if(texModulator.get() != nullptr){
                ofFloatPixels pixels;
                texModulator.get()->readToPixels(pixels);
                int texWidth = texModulator.get()->getWidth();
                size_t texHeight = texModulator.get()->getHeight();
                float *p = pixels.begin();
                int pixelSize = pixels.getPixelFormat();
    //            for(int i = 0; i < texWidth; i++){
    //                for(int j = 0; j < texHeigh; j++){
    //                    int index = ((j*texWidth)+(i))*pixelSize;
    //                    ofFloatColor c;
    //                    c.r = p[index];
    //                    c.g = p[index+1];
    //                    c.b = p[index+2];
    //                    tempOut[j].updateColor(i, c);
    //                }
    //            }
                for(int j = 0; j < min(tempOut.size(), texHeight); j++){
                    int lineWidth = tempOut[j].size();//sizes.get()[j];
                    ofFloatColor c;
                    for(int i = 0; i < lineWidth; i++){
                        int index = ((j*texWidth)+(i))*pixelSize;
                        c.r = p[index];
                        c.g = p[index+1];
                        c.b = p[index+2];
                        tempOut[j].updateColor(i, c);
                    }
                    //tempOut[j].setColor(vc);
                }
            }
            output = tempOut;
        }
    
private:
    ofEventListener listener;
    ofEventListener listener2;
    
    ofxSVG svg;
    vector<ofxFatLine> tempOut;
    ofParameter<int> width, height;
    ofParameter<string> filename;
    ofParameter<int>    resample;
    ofParameter<ofTexture*> texModulator;
    //ofParameter<vector<float>> modulator;
    ofParameter<vector<ofxFatLine>> output;
};

#endif /* svgTools_h */
