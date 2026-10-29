#pragma once

// Headless node fixtures have no GUI scopes, scheduling backends or dropdown graphs, and
// connections are destroyed before their wrappers. Replace only those otherwise-unused
// core dependencies; OF parameters/events and Oceanode connections stay real.
#include "ofxOceanodeParameter.h"
#include "ofxOceanodeScheduling.h"
#include <cassert>

ofxOceanodeAbstractParameter::~ofxOceanodeAbstractParameter() {}

void ofxOceanodeAbstractParameter::removeAllConnections() {
    assert(!hasInConnection() && !hasOutConnections() && !isScoped());
}

bool ofxOceanodeAbstractParameter::refreshCombinedInput() { return false; }

bool ofxOceanodeScheduling::isScheduledSendActive() { return false; }
