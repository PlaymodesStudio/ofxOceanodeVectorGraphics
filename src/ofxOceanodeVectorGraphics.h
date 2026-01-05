//
//  ofxOceanodeVectorGraphics.h
//  example
//
//  Created by Eduard Frigola Bagué on 14/10/2020.
//

#ifndef ofxOceanodeVectorGraphics_h
#define ofxOceanodeVectorGraphics_h

#include "ofxOceanode.h"

#include "fourierEpicyles.h"
#include "svgTools.h"
#include "polylineGenerator.h"
#include "polylineRenderer.h"
#include "remove-1.h"
#include "fatlineGenerator.h"
#include "fatlineRenderer.h"
#include "manualPoints.h"
#include "pointsResamplerIndex.h"
#include "pointsResampler.h"
#include "dotMaker.h"
#include "segmentsMaker.h"
#include "multiSegments.h"
#include "shapeTransform.h"
#include "polygonCreator.h"
#include "pointMatrix.h"
#include "bezierCurve.h"
#include "radialModulation.h"
#include "shapeInterpolator.h"
#include "pointAtPercent.h"
#include "shapeFeather.h"
#include "shapeLength.h"
#include "borderMask.h"

namespace ofxOceanodeVectorGraphics{
static void registerModels(ofxOceanode &o){
	o.registerModel<fatlineGenerator>("Vector Graphics");
	o.registerModel<fatlineRenderer>("Vector Graphics");
	o.registerModel<fourierShape>("Vector Graphics");
	o.registerModel<inverseFourierShape>("Vector Graphics");
	o.registerModel<svgModulator>("Vector Graphics");
	o.registerModel<polylineGenerator>("Vector Graphics");
	o.registerModel<polylineRenderer>("Vector Graphics");
	o.registerModel<manualPoints>("Vector Graphics");
	o.registerModel<pointsResampler>("Vector Graphics");
	o.registerModel<pointsResamplerIndex>("Vector Graphics");
	o.registerModel<dotMaker>("Vector Graphics");
	o.registerModel<segmentsMaker>("Vector Graphics");
	o.registerModel<multiSegments>("Vector Graphics");
	o.registerModel<shapeTransform>("Vector Graphics");
	o.registerModel<polygonCreator>("Vector Graphics");
	o.registerModel<pointMatrix>("Vector Graphics");
	o.registerModel<removeZeros>("Vector Graphics");
	o.registerModel<bezierCurve>("Vector Graphics");
	o.registerModel<radialModulation>("Vector Graphics");
	o.registerModel<shapeInterpolator>("Vector Graphics");
	o.registerModel<pointAtPercent>("Vector Graphics");
	o.registerModel<shapeFeather>("Vector Graphics");
	o.registerModel<shapeLength>("Vector Graphics");
	o.registerModel<borderMask>("Border Mask");
}
static void registerType(ofxOceanode &o){
	o.registerType<ofPolyline>("Polyline");
	o.registerType<vector<ofPath>>("v_Poly");
	o.registerType<ofxFatLine>("Fatline");
	o.registerType<vector<ofxFatLine>>("v_Fatline");
#ifndef ofxOceanodeTextures_h
	o.registerType<ofTexture*>("Texture");
#endif
}
static void registerScope(ofxOceanode &o){
	//-------------------------------------------------------------------------------------------- polyline
	o.registerScope<ofPolyline>([](ofxOceanodeAbstractParameter *p, ImVec2 size){
		auto size2 = ImGui::GetContentRegionAvail();
		bool keepAspectRatio = (p->getFlags() & ofxOceanodeParameterFlags_ScopeKeepAspectRatio);
		if(keepAspectRatio)
		{
			if(size.y>size.x) size2.y = size2.x;
			else size2.x = size.y;
		}
		
		auto polylineVertices = p->cast<ofPolyline>().getParameter().get().getVertices();
		
		ImVec2 origin = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		if(polylineVertices.size() > 1){
			for (int n = 0; n < polylineVertices.size() - 1; n++)
				draw_list->AddLine(ImVec2(origin.x + (polylineVertices[n].x * size2.x), origin.y + (polylineVertices[n].y * size2.y)), ImVec2(origin.x + (polylineVertices[n + 1].x * size2.x), origin.y + (polylineVertices[n + 1].y * size2.y)), IM_COL32(255, 255, 0, 255), 2.0f);
		}
	});
	//-------------------------------------------------------------------------------------------- path
	o.registerScope<vector<ofPath>>([](ofxOceanodeAbstractParameter *p, ImVec2 size){
		auto polylines = p->cast<vector<ofPath>>().getParameter().get();
		auto size2 = ImGui::GetContentRegionAvail();
		bool keepAspectRatio = (p->getFlags() & ofxOceanodeParameterFlags_ScopeKeepAspectRatio);
		if(keepAspectRatio)
		{
			if(size.y>size.x) size2.y = size2.x;
			else size2.x = size.y;
		}
		
		for(auto p : polylines){
			auto polylineVertices = p.getOutline()[0].getVertices();
			ImVec2 origin = ImGui::GetCursorScreenPos();
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			if(polylineVertices.size() > 1){
				for (int n = 0; n < polylineVertices.size() - 1; n++)
					draw_list->AddLine(ImVec2(origin.x + (polylineVertices[n].x * size2.x), origin.y + (polylineVertices[n].y * size2.y)), ImVec2(origin.x + (polylineVertices[n + 1].x * size2.x), origin.y + (polylineVertices[n + 1].y * size2.y)), IM_COL32(p.getStrokeColor().r, p.getStrokeColor().g, p.getStrokeColor().b, p.getStrokeColor().a), p.getStrokeWidth());
			}
		}
	});
	//-------------------------------------------------------------------------------------------- fatline
	o.registerScope<ofxFatLine>([](ofxOceanodeAbstractParameter *p, ImVec2 size){
		auto polylineVertices = p->cast<ofxFatLine>().getParameter().get().getVertices();
		auto size2 = ImGui::GetContentRegionAvail();
		bool keepAspectRatio = (p->getFlags() & ofxOceanodeParameterFlags_ScopeKeepAspectRatio);
		if(keepAspectRatio)
		{
			if(size.y>size.x) size2.y = size2.x;
			else size2.x = size.y;
		}
		
		ImVec2 origin = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		if(polylineVertices.size() > 1){
			for (int n = 0; n < polylineVertices.size() - 1; n++)
				draw_list->AddLine(ImVec2(origin.x + (polylineVertices[n].x * size2.x), origin.y + (polylineVertices[n].y * size2.y)), ImVec2(origin.x + (polylineVertices[n + 1].x * size2.x), origin.y + (polylineVertices[n + 1].y * size2.y)), IM_COL32(255, 255, 0, 255), 2.0f);
		}
	});
	//-------------------------------------------------------------------------------------------- vector fatline
	o.registerScope<vector<ofxFatLine>>([](ofxOceanodeAbstractParameter *p, ImVec2 size){
		auto polylines = p->cast<vector<ofxFatLine>>().getParameter().get();
		auto size2 = ImGui::GetContentRegionAvail();
		bool keepAspectRatio = (p->getFlags() & ofxOceanodeParameterFlags_ScopeKeepAspectRatio);
		if(keepAspectRatio)
		{
			if(size.y>size.x) size2.y = size2.x;
			else size2.x = size.y;
		}
		for(auto p : polylines){
			auto polylineVertices = p.getVertices();
			auto colors = p.getColors();
			glm::vec2 minSize = glm::vec2(0,0);
			glm::vec2 maxSize = glm::vec2(800,800);
			
			
			//			glm::vec2 minSize = glm::vec2(10000,10000);
			//			glm::vec2 maxSize = glm::vec2(0,0);
			//			for(auto p_ : polylineVertices){
			//				if(p_.x < minSize.x) minSize.x = p_.x;
			//				if(p_.y < minSize.y) minSize.y = p_.y;
			//
			//				if(p_.x > maxSize.x) maxSize.x = p_.x;
			//				if(p_.y > maxSize.y) maxSize.y = p_.y;
			//			}
			
			//			minSize.x *= size.x;
			//			minSize.y *= size.y;
			//			maxSize.x *= size.x;
			//			maxSize.y *= size.y;
			
			ImVec2 origin = ImGui::GetCursorScreenPos();
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			if(polylineVertices.size() > 1){
				for (int n = 0; n < polylineVertices.size() - 1; n++){
					glm::vec2 middlePoint = glm::mix(glm::vec2(polylineVertices[n].x, polylineVertices[n].y), glm::vec2(polylineVertices[n+1].x, polylineVertices[n+1].y), 0.5f);
					draw_list->AddLine(ImVec2(origin.x + (ofMap(polylineVertices[n].x, minSize.x, maxSize.x, 0, size2.x)), origin.y + (ofMap(polylineVertices[n].y, minSize.y, maxSize.y, 0, size2.y))), ImVec2(origin.x + (ofMap(middlePoint.x, minSize.x, maxSize.x, 0, size2.x)), origin.y + (ofMap(middlePoint.y, minSize.y, maxSize.y, 0, size2.y))), IM_COL32(colors[n].r*255, colors[n].g*255, colors[n].b*255, colors[n].a*255), p.getWeight(n));
					draw_list->AddLine(ImVec2(origin.x + (ofMap(middlePoint.x, minSize.x, maxSize.x, 0, size2.x)), origin.y + (ofMap(middlePoint.y, minSize.y, maxSize.y, 0, size2.y))), ImVec2(origin.x + (ofMap(polylineVertices[n+1].x, minSize.x, maxSize.x, 0, size2.x)), origin.y + (ofMap(polylineVertices[n+1].y, minSize.y, maxSize.y, 0, size2.y))), IM_COL32(colors[n+1].r*255, colors[n+1].g*255, colors[n+1].b*255, colors[n+1].a*255), p.getWeight(n+1));
					
					// draw circle for each vertex
					draw_list->AddCircleFilled(ImVec2(origin.x + (ofMap(polylineVertices[n].x, minSize.x, maxSize.x, 0, size2.x)),
													  origin.y + (ofMap(polylineVertices[n].y, minSize.y, maxSize.y, 0, size2.y))),
											   5,
											   IM_COL32(colors[n].r*255, colors[n].g*255, colors[n].b*255, colors[n].a*255), 4);
				}
				// draw circle for last vertex
				int lastVertex = polylineVertices.size()-1;
				draw_list->AddCircleFilled(ImVec2(origin.x + (ofMap(polylineVertices[lastVertex].x, minSize.x, maxSize.x, 0, size2.x)),
												  origin.y + (ofMap(polylineVertices[lastVertex].y, minSize.y, maxSize.y, 0, size2.y))),
										   5,
										   IM_COL32(colors[lastVertex].r*255, colors[lastVertex].g*255, colors[lastVertex].b*255, colors[lastVertex].a*255), 4);
			}
			else if(polylineVertices.size()==1)
			{
				//draw_list->AddCircle(ImVec2( X , Y,  RADIUS , ImGui::GetColorU32(ImGuiCol_Text));
				draw_list->AddCircleFilled(ImVec2(origin.x + (ofMap(polylineVertices[0].x, minSize.x, maxSize.x, 0, size2.x)),
												  origin.y + (ofMap(polylineVertices[0].y, minSize.y, maxSize.y, 0, size2.y))),
										   5,
										   IM_COL32(colors[0].r*255, colors[0].g*255, colors[0].b*255, colors[0].a*255), 4);
			}
			
		}
	});
	
#ifndef ofxOceanodeTextures_h
	//-------------------------------------------------------------------------------------------- texture *
	o.registerScope<ofTexture*>([](ofxOceanodeAbstractParameter *p, ImVec2 size){
		auto tex = p->cast<ofTexture*>().getParameter().get();
		auto size2 = ImGui::GetContentRegionAvail();
		bool keepAspectRatio = (p->getFlags() & ofxOceanodeParameterFlags_ScopeKeepAspectRatio);
		float sizeAspectRatio=size.x/size.y;
		float texAspectRatio;
		if(tex != nullptr){
			texAspectRatio = tex->getWidth() / tex->getHeight();
		}
		if(keepAspectRatio)
		{
			if(sizeAspectRatio<texAspectRatio)
			{
				size2.y = size2.x / texAspectRatio;
				size2.x = size.x;
			}
			else
			{
				size2.x = size2.y * texAspectRatio;
				size2.y = size.y;
			}
		}
		
		if(tex != nullptr){
			ImTextureID textureID = (ImTextureID)(uintptr_t)tex->texData.textureID;
			ImGui::Image(textureID, size2);
		}
	});
#endif
}

static void registerCollection(ofxOceanode &o){
	registerModels(o);
	registerType(o);
	registerScope(o);
}
}

#endif /* ofxOceanodeVectorGraphics_h */
