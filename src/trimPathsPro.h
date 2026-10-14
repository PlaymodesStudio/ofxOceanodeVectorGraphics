#pragma once

#include "ofxOceanodeNodeModel.h"
#include "imgui.h"
#include "trimPathsProCore.h"
#include <climits>

class trimPathsPro : public ofxOceanodeNodeModel {
public:
    trimPathsPro() : ofxOceanodeNodeModel("Trim Paths Pro") {}

    void setup() override {
        description = "Trim edges, individual paths, all paths, or selected groups. Start, End and Origin map independently; "
            "scalar values broadcast in Auto. Origin shifts the trim with wrapping. Progress uses edge count or distance. Changes are batched until the next "
            "node update. Open Show GUI for groups, counts and diagnostics. -1 separators must match in X and Y.";
        addParameter(pointsX.set("In.X", {0.5f}, {-FLT_MAX}, {FLT_MAX}));
        addParameter(pointsY.set("In.Y", {0.5f}, {-FLT_MAX}, {FLT_MAX}));
        addParameterDropdown(scope, "Scope", 1, {"Edges", "Paths", "All Paths", "Groups"});
        addParameterDropdown(metric, "Progress", 1, {"Edge Count", "Distance"});
        addParameter(start.set("Start", {0}, {0}, {1}));
        addParameter(end.set("End", {1}, {0}, {1}));
        addParameter(origin.set("Origin", {0}, {0}, {1}));
        const vector<string> mappings{"Auto", "Scalar", "Per Path", "Per Edge", "Per Group"};
        addParameterDropdown(startMapping, "Start Map", 0, mappings);
        addParameterDropdown(endMapping, "End Map", 0, mappings);
        addParameterDropdown(originMapping, "Origin Map", 0, mappings);
        addParameter(keepOrder.set("Keep Order", false));
        addParameterDropdown(topology, "Output", 0, {"Connected Paths", "Separate Edges"});
        addParameter(endpointDots.set("Endpoint Dots", false));
        addParameter(showWindow.set("Show GUI", false));
        addInspectorParameter(numGroups.set("Num Groups", 1, 1, 100));
        addOutputParameter(outX.set("Out.X", {}, {-FLT_MAX}, {FLT_MAX}));
        addOutputParameter(outY.set("Out.Y", {}, {-FLT_MAX}, {FLT_MAX}));
        addOutputParameter(completeness.set("Completeness", {}, {0}, {1}));
        addOutputParameter(fullX.set("FullSegment.X", {}, {-FLT_MAX}, {FLT_MAX}));
        addOutputParameter(fullY.set("FullSegment.Y", {}, {-FLT_MAX}, {FLT_MAX}));
        addOutputParameter(wrappedX.set("WrappedSegment.X", {}, {-FLT_MAX}, {FLT_MAX}));
        addOutputParameter(wrappedY.set("WrappedSegment.Y", {}, {-FLT_MAX}, {FLT_MAX}));
        addOutputParameter(sourcePath.set("Source.Path", {}, {-1}, {INT_MAX}));
        addOutputParameter(sourceEdge.set("Source.Edge", {}, {-1}, {INT_MAX}));
        addOutputParameter(sourceGroup.set("Source.Group", {}, {-1}, {INT_MAX}));
        addOutputParameter(pathCount.set("Path Count", 0, 0, INT_MAX));
        addOutputParameter(edgeCount.set("Edge Count", 0, 0, INT_MAX));
        addOutputParameter(groupCount.set("Group Count", 1, 1, 100));
        addOutputParameter(valid.set("Valid", true));
        addOutputParameter(status.set("Status", "Waiting for input"));

        watch(pointsX); watch(pointsY); watch(scope); watch(metric); watch(start); watch(end); watch(origin);
        watch(startMapping); watch(endMapping); watch(originMapping); watch(keepOrder); watch(topology); watch(endpointDots);
        listeners.push(numGroups.newListener([this](int& n) {
            const int count = std::max(1, std::min(100, n));
            if(count != n) numGroups.setWithoutEventNotifications(count);
            groups.resize(count);
            focusedGroup = std::max(0, std::min(focusedGroup, count-1));
            dirty = true;
        }));
    }

    void update(ofEventArgs&) override {
        if(!dirty) return;
        // Notifications raised by downstream feedback during publish remain pending for next update.
        dirty = false;
        geometry = trimPathsProCore::parse(pointsX.get(), pointsY.get());
        trimPathsProCore::Settings settings;
        settings.scope = static_cast<trimPathsProCore::Scope>(scope.get());
        settings.metric = static_cast<trimPathsProCore::Metric>(metric.get());
        settings.startMapping = static_cast<trimPathsProCore::Mapping>(startMapping.get());
        settings.endMapping = static_cast<trimPathsProCore::Mapping>(endMapping.get());
        settings.originMapping = static_cast<trimPathsProCore::Mapping>(originMapping.get());
        settings.start = start.get(); settings.end = end.get(); settings.origin = origin.get();
        settings.keepOrder = keepOrder.get(); settings.connected = topology.get() == 0;
        settings.endpointDots = endpointDots.get(); settings.groups = groups;
        const auto result = trimPathsProCore::trim(geometry, settings);

        // Stage the complete result before notifying ANY output, then notify X last for X-driven consumers.
        outX.setWithoutEventNotifications(result.x); outY.setWithoutEventNotifications(result.y);
        fullX.setWithoutEventNotifications(result.fullX); fullY.setWithoutEventNotifications(result.fullY);
        wrappedX.setWithoutEventNotifications(result.wrappedX); wrappedY.setWithoutEventNotifications(result.wrappedY);
        completeness.setWithoutEventNotifications(result.completeness);
        sourcePath.setWithoutEventNotifications(result.sourcePath); sourceEdge.setWithoutEventNotifications(result.sourceEdge);
        sourceGroup.setWithoutEventNotifications(result.sourceGroup);
        pathCount.setWithoutEventNotifications(static_cast<int>(geometry.paths.size()));
        edgeCount.setWithoutEventNotifications(static_cast<int>(geometry.edges.size()));
        groupCount.setWithoutEventNotifications(static_cast<int>(groups.size()));
        valid.setWithoutEventNotifications(result.valid); status.setWithoutEventNotifications(result.status);
        notify(valid); notify(status); notify(pathCount); notify(edgeCount); notify(groupCount);
        notify(sourcePath); notify(sourceEdge); notify(sourceGroup); notify(completeness);
        notify(wrappedY); notify(wrappedX); notify(fullY); notify(fullX); notify(outY); notify(outX);
    }

    void presetSave(ofJson& json) override { json["TrimPathsProGroups"] = groups; }

    void presetRecallBeforeSettingParameters(ofJson& json) override {
        // Replace membership even for empty or missing entries; never retain another preset's groups.
        groups.assign(1, {});
        auto it = json.find("TrimPathsProGroups");
        if(it != json.end() && it->is_array() && !it->empty()) {
            groups.resize(std::min<size_t>(100, it->size()));
            for(size_t i = 0; i < groups.size(); ++i) {
                const auto& saved = (*it)[i];
                if(!saved.is_array()) continue;
                for(const auto& value : saved) {
                    // Conversion only after range checking also handles unsigned JSON integers safely.
                    if(value.is_number_integer() && value >= 0 && value <= INT_MAX)
                        groups[i].push_back(value.get<int>());
                }
                std::sort(groups[i].begin(), groups[i].end());
                groups[i].erase(std::unique(groups[i].begin(), groups[i].end()), groups[i].end());
            }
        }
        numGroups = static_cast<int>(groups.size());
        focusedGroup = 0;
        dirty = true;
    }

    void presetRecallAfterSettingParameters(ofJson&) override { dirty = true; }

    void draw(ofEventArgs&) override {
        if(!showWindow.get()) return;
        if(ImGui::Begin(("Trim Paths Pro " + ofToString(getNumIdentifier())).c_str())) {
            ImGui::Text("Paths: %d   Edges: %d   Groups: %d", pathCount.get(), edgeCount.get(), groupCount.get());
            ImGui::TextWrapped("%s", status.get().c_str());
            ImGui::TextWrapped("Start, End and Origin use independent mappings. Auto broadcasts one value. Origin shifts the trim and wraps at the end.");
            ImGui::Separator();
            if(ImGui::Button("Add Group") && groups.size() < 100) numGroups = numGroups.get()+1;
            ImGui::SameLine();
            if(ImGui::Button("Remove Last Group") && groups.size() > 1) numGroups = numGroups.get()-1;
            for(size_t i = 0; i < groups.size(); ++i) {
                ImGui::PushID(static_cast<int>(i));
                if(ImGui::RadioButton(("Group " + ofToString(i)).c_str(), focusedGroup == static_cast<int>(i)))
                    focusedGroup = static_cast<int>(i);
                ImGui::SameLine();
                ImGui::Text("%d paths", static_cast<int>(groups[i].size()));
                ImGui::PopID();
            }
            auto& selected = groups[focusedGroup];
            if(ImGui::Button("Select All Paths")) {
                selected.clear();
                for(size_t i = 0; i < geometry.paths.size(); ++i) selected.push_back(static_cast<int>(i));
                dirty = true;
            }
            ImGui::SameLine();
            if(ImGui::Button("Clear Group")) { selected.clear(); dirty = true; }
            ImGui::TextWrapped("Click a path in the preview or its checkbox. A path may belong to multiple groups. Group traversal follows input path order.");
            drawPreview();
            if(ImGui::TreeNode("Path membership")) {
                for(size_t i = 0; i < geometry.paths.size(); ++i) {
                    bool member = std::find(selected.begin(), selected.end(), static_cast<int>(i)) != selected.end();
                    if(ImGui::Checkbox(("Path " + ofToString(i)).c_str(), &member)) togglePath(static_cast<int>(i));
                }
                ImGui::TreePop();
            }
        }
        ImGui::End();
    }

private:
    template<typename T> void watch(ofParameter<T>& parameter) {
        listeners.push(parameter.newListener([this](T&) { dirty = true; }));
    }
    template<typename T> void notify(ofParameter<T>& parameter) { parameter.set(parameter.get()); }

    void togglePath(int path) {
        auto& selected = groups[focusedGroup];
        auto it = std::find(selected.begin(), selected.end(), path);
        if(it != selected.end()) selected.erase(it);
        else { selected.push_back(path); std::sort(selected.begin(), selected.end()); }
        dirty = true;
    }

    void drawPreview() {
        const ImVec2 size(std::max(100.0f, ImGui::GetContentRegionAvail().x), 260.0f);
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("PathsPreview", size);
        auto* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(origin, ImVec2(origin.x+size.x, origin.y+size.y), IM_COL32(24,24,24,255));
        if(geometry.paths.empty()) return;
        double minX = geometry.paths.front().front().x, maxX = minX;
        double minY = geometry.paths.front().front().y, maxY = minY;
        for(const auto& path : geometry.paths) for(const auto& p : path) {
            minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
            minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
        }
        const double span = std::max(maxX-minX, maxY-minY);
        const double scale = std::min(size.x-24, size.y-24) / (span > 0 ? span : 1);
        auto screen = [&](trimPathsProCore::Point p) {
            return ImVec2(origin.x+size.x*0.5f+static_cast<float>((p.x-(minX+maxX)*0.5)*scale),
                          origin.y+size.y*0.5f+static_cast<float>((p.y-(minY+maxY)*0.5)*scale));
        };
        const ImVec2 mouse = ImGui::GetMousePos();
        int hovered = -1;
        float nearest = 8*8;
        if(ImGui::IsItemHovered()) for(size_t i = 0; i < geometry.paths.size(); ++i) {
            const auto& path = geometry.paths[i];
            for(size_t j = 0; j < path.size(); ++j) {
                const auto a = screen(path[j]), b = screen(path[std::min(j+1, path.size()-1)]);
                const float dx = b.x-a.x, dy = b.y-a.y, lengthSquared = dx*dx+dy*dy;
                const float t = lengthSquared > 0 ? std::max(0.0f, std::min(1.0f,
                    ((mouse.x-a.x)*dx+(mouse.y-a.y)*dy)/lengthSquared)) : 0;
                const float px = mouse.x-a.x-t*dx, py = mouse.y-a.y-t*dy;
                if(px*px+py*py < nearest) { nearest = px*px+py*py; hovered = static_cast<int>(i); }
            }
        }
        if(hovered >= 0 && ImGui::IsMouseClicked(0)) togglePath(hovered);
        for(size_t i = 0; i < geometry.paths.size(); ++i) {
            const auto& path = geometry.paths[i];
            const auto& selected = groups[focusedGroup];
            const bool member = std::find(selected.begin(), selected.end(), static_cast<int>(i)) != selected.end();
            const ImU32 color = member ? IM_COL32(70,210,230,255) : IM_COL32(130,130,130,255);
            for(size_t j = 1; j < path.size(); ++j)
                draw->AddLine(screen(path[j-1]), screen(path[j]), color, hovered == static_cast<int>(i) ? 4.0f : 2.0f);
            draw->AddCircleFilled(screen(path.front()), 3, color);
            draw->AddText(screen(path.front()), IM_COL32(255,255,255,255), ofToString(i).c_str());
        }
    }

    ofParameter<vector<float>> pointsX, pointsY, start, end, origin;
    ofParameter<vector<float>> outX, outY, completeness, fullX, fullY, wrappedX, wrappedY;
    ofParameter<vector<int>> sourcePath, sourceEdge, sourceGroup;
    ofParameter<int> scope, metric, startMapping, endMapping, originMapping, topology, numGroups, pathCount, edgeCount, groupCount;
    ofParameter<bool> keepOrder, endpointDots, showWindow, valid;
    ofParameter<string> status;
    std::vector<std::vector<int>> groups{{}};
    trimPathsProCore::Geometry geometry;
    int focusedGroup = 0;
    bool dirty = true;
    ofEventListeners listeners;
};
