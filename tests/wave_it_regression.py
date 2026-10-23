#!/usr/bin/env python3
"""Wave It geometry and real OF event regressions with ASan/UBSan; addon syntax.

Use --core-only on machines without compiled macOS openFrameworks libraries.
OF_ROOT overrides the usual sibling openFrameworks checkout.
Only node registration is substituted in the adapter runtime test.
"""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    of = Path(os.environ.get("OF_ROOT", ROOT.parent.parent))
    compiler = os.environ.get("CXX", "clang++")
    flags = [compiler, "-std=c++17", "-O0", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    with tempfile.TemporaryDirectory(prefix="wave-it-") as temp:
        temp = Path(temp)
        core = temp / "core-tests"
        subprocess.run(flags + ["-Wall", "-Wextra", "-pedantic", str(ROOT / "tests/waveItCoreTests.cpp"), "-o", str(core)], check=True)
        subprocess.run([str(core)], check=True)
        if "--core-only" in sys.argv:
            return
        includes = sorted({p.parent for p in (of / "libs/openFrameworks").rglob("*.h")})
        includes += [p for p in (of / "libs").glob("*/include") if p.is_dir()]
        includes += [of / "libs/freetype/include/freetype", of / "libs/cairo/include/cairo"]
        for addon in ("ofxOceanode", "ofxImGui", "ofxMidi", "ofxOsc", "ofxFatLines", "ofxSvg"):
            for directory in ("src", "libs"):
                includes += sorted({p.parent for p in (of / "addons" / addon / directory).rglob("*.h")})
        includes += [ROOT / "src"]
        incflags = ["-I" + str(p) for p in dict.fromkeys(includes)]
        hostflags = ["-DOF_NO_FMOD", "-Wno-deprecated-declarations", "-Wno-c++20-extensions"]
        subprocess.run([compiler, "-std=c++17", "-fsyntax-only"] + hostflags + incflags + [str(ROOT / "src/ofxOceanodeVectorGraphics.cpp")], check=True)
        print("Complete addon registration/header syntax passed", flush=True)
        imgui = of / "addons/ofxImGui/libs/imgui/src"
        libs = of / "libs/openFrameworksCompiled/lib/macos"
        command = flags + hostflags + ["-I" + str(ROOT / "tests/support")] + incflags
        command += [str(ROOT / "tests/waveItNodeTests.cpp"),
                    str(of / "libs/openFrameworks/types/ofParameter.cpp"),
                    str(of / "libs/openFrameworks/types/ofParameterGroup.cpp")]
        command += [str(imgui / name) for name in ("imgui.cpp", "imgui_draw.cpp", "imgui_tables.cpp", "imgui_widgets.cpp")]
        command += [str(libs / "openFrameworks.a")]
        command += [str(p) for p in sorted(libs.glob("*.a")) if "openframeworks" not in p.name.lower()]
        command += ["-liconv", "-Wl,-dead_strip"]
        for framework in ("Accelerate", "AppKit", "ApplicationServices", "AudioToolbox", "AVFoundation",
                          "Cocoa", "CoreAudio", "CoreFoundation", "CoreMedia", "CoreServices", "CoreVideo",
                          "IOKit", "OpenGL", "QuartzCore", "Security"):
            command += ["-framework", framework]
        executable = temp / "node-tests"
        subprocess.run(command + ["-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True, cwd=temp)


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        raise SystemExit(error.returncode)
