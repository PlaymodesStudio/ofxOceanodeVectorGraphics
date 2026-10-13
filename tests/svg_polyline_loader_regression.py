#!/usr/bin/env python3
"""Real OF parameters, ofPath and ofxSvg tests; only node registration/UI are stubbed.

macOS: requires compiled sibling openFrameworks, ofxSvg and addon dependencies.
OCEANODE_TEST_OF_ROOT can override the openFrameworks directory.
Also syntax-checks the complete addon against real Oceanode/ImGui headers.
"""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
STUB = r'''
#pragma once
#include "ofMain.h"
ofFileDialogResult svgTestLoadDialog(std::string, bool, std::string);
#define ofSystemLoadDialog svgTestLoadDialog
class ofxOceanodeNodeModel {
public:
    using customGuiRegion = ofParameter<std::function<void()>>;
    ofxOceanodeNodeModel(std::string) {}
    virtual ~ofxOceanodeNodeModel() = default;
    virtual void setup() {}
    virtual void presetRecallAfterSettingParameters(ofJson &) {}
    template<class T> void addParameter(ofParameter<T> &p) { parameters.add(p); }
    template<class T> void addOutputParameter(ofParameter<T> &p) { parameters.add(p); }
    void addCustomRegion(customGuiRegion &p, std::function<void()>) { parameters.add(p); }
    ofParameterGroup &getParameterGroup() { return parameters; }
    void addInspectorParameter(ofAbstractParameter &p) { inspector.add(p); }
    ofParameterGroup &getInspectorParameterGroup() { return inspector; }
protected:
    std::string description;
    ofParameterGroup parameters, inspector;
};
'''
UI = r'''
#pragma once
namespace ImGui {
inline bool pressLoad = false;
inline bool Button(const char*) { bool clicked = pressLoad; pressLoad = false; return clicked; }
inline void TextWrapped(const char*, ...) {}
}
'''

def main():
    of = Path(os.environ.get("OCEANODE_TEST_OF_ROOT", ROOT.parent.parent))
    includes = sorted({p.parent for p in (of / "libs/openFrameworks").rglob("*.h")})
    includes += [p for p in (of / "libs").glob("*/include") if p.is_dir()]
    includes += [of / "libs/freetype/include/freetype", of / "libs/cairo/include/cairo"]
    for addon in ("ofxOceanode", "ofxImGui", "ofxMidi", "ofxOsc", "ofxFatLines", "ofxSvg"):
        base = of / "addons" / addon
        includes += sorted({p.parent for p in (base / "src").rglob("*.h")})
        includes += sorted({p.parent for p in (base / "libs").rglob("*.h")})
    # During development, ROOT may be a staging directory.
    includes += [of / "addons/ofxOceanodeVectorGraphics/src", ROOT / "src"]
    flags = [os.environ.get("CXX", "clang++"), "-std=c++17", "-w", "-DOF_NO_FMOD"]
    incflags = ["-I" + str(p) for p in dict.fromkeys(includes)]
    libs = of / "libs/openFrameworksCompiled/lib/macos"
    frameworks = ["Accelerate", "AppKit", "ApplicationServices", "AudioToolbox",
                  "AVFoundation", "Cocoa", "CoreAudio", "CoreFoundation", "CoreMedia",
                  "CoreServices", "CoreVideo", "IOKit", "OpenGL", "QuartzCore", "Security"]
    with tempfile.TemporaryDirectory(prefix="svg-polyline-test-") as folder:
        folder = Path(folder)
        syntax = folder / "syntax.cpp"
        syntax.write_text('#include "' + str(ROOT / "src/ofxOceanodeVectorGraphics.h") + '"\n')
        subprocess.run(flags + incflags + ["-fsyntax-only", str(syntax)], check=True)
        print("Addon registration/header syntax passed", flush=True)
        (folder / "ofxOceanodeNodeModel.h").write_text(STUB)
        (folder / "imgui.h").write_text(UI)
        command = flags + ["-O0", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                           "-I" + str(folder), "-I" + str(ROOT / "src")] + incflags
        command += [str(ROOT / "tests/svg_polyline_loader_regression.cpp"),
                    str(of / "addons/ofxSvg/src/ofxSvg.cpp"),
                    str(of / "libs/openFrameworks/types/ofParameter.cpp"),
                    str(of / "libs/openFrameworks/types/ofParameterGroup.cpp"),
                    str(libs / "openFrameworks.a")]
        command += [str(p) for p in sorted(libs.glob("*.a")) if "openframeworks" not in p.name.lower()]
        command += [str(of / "addons/ofxSvg/libs/svgtiny/lib/macos/svgtiny.xcframework/macos-arm64_x86_64/libsvgtiny.a"),
                    str(of / "addons/ofxSvg/libs/libxml2/lib/macos/libxml2.xcframework/macos-arm64_x86_64/libxml2.a"),
                    "-liconv", "-Wl,-dead_strip"]
        for fw in frameworks:
            command += ["-framework", fw]
        executable = folder / "regression"
        subprocess.run(command + ["-o", str(executable)], check=True)
        subprocess.run([str(executable), str(folder)], check=True)

if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        raise SystemExit(error.returncode)
