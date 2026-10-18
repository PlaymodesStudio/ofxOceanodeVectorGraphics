#include "MacroMixerFatline.h"
#include <cmath>
#include <stdexcept>

namespace ofxOceanodeVectorGraphics {
namespace {
using Lines=std::vector<ofxFatLine>;
void validate(const Lines& lines) {
    if(lines.size()>4096) throw std::runtime_error("Fatline Vector exceeds 4096 shapes");
    size_t points=0;
    for(auto line:lines) {
        points+=line.size();
        if(points>1048576) throw std::runtime_error("Fatline Vector exceeds 1048576 vertices");
        const auto colors=line.getColors();
        if(colors.size()!=line.size()) throw std::runtime_error("Fatline vertex/color counts differ");
        for(size_t i=0;i<line.size();++i) {
            const auto& p=line.getVertices()[i];
            if(!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) || !std::isfinite(line.getWeight(i)))
                throw std::runtime_error("Fatline contains non-finite geometry/width");
            for(size_t c=0;c<4;++c) if(!std::isfinite(colors[i][c])) throw std::runtime_error("Fatline contains non-finite color/opacity");
        }
    }
}
ofFloatColor faded(ofFloatColor color,float opacity) {
    // Fat Line and ILDA Shape use premultiplied RGB. Alpha-only attenuation
    // would leave physical laser brightness unchanged (ILDA reads RGB).
    color.r*=opacity; color.g*=opacity; color.b*=opacity; color.a*=opacity;
    return color;
}
void append(Lines& result,const Lines& source,float opacity) {
    for(auto line:source) {
        const auto colors=line.getColors();
        for(size_t i=0;i<colors.size();++i) { auto color=faded(colors[i],opacity); line.updateColor(i,color); }
        // Preserve the existing mesh, including feathering/caps, without rebuilding
        // or changing any vertices. Renderers that rebuild use the scaled raw colors.
        auto& mesh=line.getMesh();
        for(size_t i=0;i<mesh.getNumColors();++i) mesh.setColor(i,faded(mesh.getColor(i),opacity));
        result.push_back(std::move(line));
    }
}
}
MacroMixerMixing::Definition<Lines> fatlineMixingDefinition() {
    MacroMixerMixing::Definition<Lines> definition;
    definition.type="Fatline Vector"; definition.policy="opacity"; definition.label="Opacity crossfade";
    definition.allowSwitch=true;
    definition.validate=validate;
    definition.mix=[](const Lines& a,const Lines& b,float progress,const ofJson&) {
        Lines result; result.reserve(a.size()+b.size());
        append(result,a,1-progress); append(result,b,progress);
        return result;
    };
    return definition;
}
}
