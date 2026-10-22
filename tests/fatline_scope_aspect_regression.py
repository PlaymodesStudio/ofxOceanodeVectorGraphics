#!/usr/bin/env python3
"""Test the shared Fatline Scope size/toggle workflow with real ImGui."""
from pathlib import Path
import os
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
IMGUI=ROOT.parent/'ofxImGui/libs/imgui/src'
CASES=r'''
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
constexpr int ofxOceanodeParameterFlags_ScopeKeepAspectRatio=1<<5;
struct ofxOceanodeAbstractParameter {
    int flags=4; int getFlags(){return flags;} void setFlags(int f){flags=f;}
};
HELPERS
ofxOceanodeAbstractParameter parameter;
ImVec2 available, fitted, button;
void frame(){
    ImGui::NewFrame();ImGui::SetNextWindowPos(ImVec2(20,20));ImGui::SetNextWindowSize(ImVec2(800,700));
    ImGui::Begin("Fatline Scope");
    fitted=fatlineScopeSize(&parameter,available);
    const ImVec2 origin=ImGui::GetCursorScreenPos();
    button=origin+ImVec2(std::max(0.f,fitted.x-20)+8,13);
    auto* drawList=ImGui::GetWindowDrawList();const int vertices=drawList->VtxBuffer.Size;
    drawScopeBorder(origin,origin+fitted);assert(drawList->VtxBuffer.Size>vertices);
    drawFatlineScopeAspectToggle(&parameter,fitted);
    assert(ImGui::GetCursorScreenPos().y>=origin.y+fitted.y);
    ImGui::End();ImGui::Render();
}
void idle(int n=3){while(n--)frame();}
void click(){auto& io=ImGui::GetIO();io.AddMousePosEvent(button.x,button.y);idle();io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);idle();}
int main(){
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.DisplaySize={1000,900};io.DeltaTime=1.f/60;io.IniFilename=nullptr;
    unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
    for(ImVec2 size:{ImVec2(500,200),ImVec2(200,500),ImVec2(200,200)}){
        available=size;parameter.flags=4;idle();assert(fitted.x==size.x&&fitted.y==size.y);
        click();assert(parameter.flags==(4|ofxOceanodeParameterFlags_ScopeKeepAspectRatio));
        assert(fitted.x==std::min(size.x,size.y)&&fitted.y==fitted.x);
        click();assert(parameter.flags==4&&fitted.x==size.x&&fitted.y==size.y);
    }
    parameter.flags=ofxOceanodeParameterFlags_ScopeKeepAspectRatio;
    auto empty=fatlineScopeSize(&parameter,ImVec2(0,-1));assert(empty.x==0&&empty.y==0);
    ImGui::DestroyContext();puts("Fatline Scope passed: portrait/landscape/square fit, toggle clicks, flag preservation and zero-size layout.");
}
'''
def main():
    source=(ROOT/'src/ofxOceanodeVectorGraphics.cpp').read_text()
    helpers=source[source.index('static ImVec2 fatlineScopeSize('):source.index('void registerScope(ofxOceanode &o, bool withTexture)')]
    for registration in ('o.registerScope<ofxFatLine>','o.registerScope<vector<ofxFatLine>>'):
        start=source.index(registration);end=source.index('\n\t});',start)
        assert 'fatlineScopeSize(p, size)' in source[start:end]
        assert 'drawScopeBorder(' in source[start:end]
        assert 'drawFatlineScopeAspectToggle(p, size2)' in source[start:end]
    with tempfile.TemporaryDirectory(prefix='fatline-scope-') as directory:
        work=Path(directory);(work/'imconfig.h').write_text('')
        cpp=work/'test.cpp';cpp.write_text(CASES.replace('HELPERS',helpers));binary=work/'test'
        subprocess.run([os.environ.get('CXX','clang++'),'-std=c++17','-O1','-g','-fsanitize=address,undefined',
                        '-I',str(IMGUI),'-I',str(work),str(cpp),*[str(IMGUI/name) for name in ('imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp','imgui_tables.cpp')],'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
if __name__=='__main__':main()
