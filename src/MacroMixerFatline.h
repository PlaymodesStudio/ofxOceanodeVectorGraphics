#pragma once
#include "ofxFatLine.h"
#include "MacroMixerMixing.h"

namespace ofxOceanodeVectorGraphics {
// Registered at the existing v_Fatline type registration, not a second node list.
MacroMixerMixing::Definition<std::vector<ofxFatLine>> fatlineMixingDefinition();
}
