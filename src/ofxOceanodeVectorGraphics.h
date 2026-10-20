//
//  ofxOceanodeVectorGraphics.h
//  example
//
//  Created by Eduard Frigola Bagué on 14/10/2020.
//


#ifndef ofxOceanodeVectorGraphics_h
#define ofxOceanodeVectorGraphics_h

// Declarations only: the node headers and the bodies live in
// ofxOceanodeVectorGraphics.cpp, so including this header does not compile
// every node of the addon again.

class ofxOceanode;

namespace ofxOceanodeVectorGraphics{

// The Texture type and scope are registered here only when ofxOceanodeTextures.h
// was not included before this header (ofxOceanodeTextures registers its own).
// Decided where this header is included, as when the bodies lived here.
#ifdef ofxOceanodeTextures_h
static constexpr bool registersTexture = false;
#else
static constexpr bool registersTexture = true;
#endif

void registerModels(ofxOceanode &o);
void registerType(ofxOceanode &o, bool withTexture);
void registerScope(ofxOceanode &o, bool withTexture);

static inline void registerType(ofxOceanode &o){ registerType(o, registersTexture); }
static inline void registerScope(ofxOceanode &o){ registerScope(o, registersTexture); }

static inline void registerCollection(ofxOceanode &o){
	registerModels(o);
	registerType(o);
	registerScope(o);
}

}

#endif /* ofxOceanodeVectorGraphics_h */
