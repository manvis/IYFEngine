// The IYFEngine
//
// Copyright (C) 2015-2018, Manvydas Šliamka
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
// conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
// of conditions and the following disclaimer in the documentation and/or other materials
// provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
// used to endorse or promote products derived from this software without specific prior
// written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
// SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
// WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef IYF_LOGIC_GRAPH_EDITOR_HPP
#define IYF_LOGIC_GRAPH_EDITOR_HPP

#include "LogicGraph.hpp"
#include "logging/Logger.hpp"
#include "utilities/hashing/HashCombine.hpp"
#include "utilities/Flags.hpp"
#include "localization/TextLocalization.hpp"

#include <set>
#include <memory>
#include "imgui.h"

namespace iyf {
enum class LogicGraphEditorButtonFlagBits : std::uint32_t {
    Load   = 0x01,
    Save   = 0x02,
    SaveAs = 0x04,
};
using LogicGraphEditorButtonFlags = Flags<LogicGraphEditorButtonFlagBits>;
IYF_DEFINE_ENUM_FLAG_OPS(LogicGraphEditorButtonFlagBits)

struct NodeEditorSettings {
    NodeEditorSettings() 
        : canvasSize(2000.0f, 1500.0f), lineDensity(50.0f, 50.0f), zoomMultiplier(0.1f), nodeWidth(150.0f), scrollMultipliers(25.0f, 25.0f),
        lineThickness(2.0f), showDebugOptions(false), showNewAsClear(false), shownButtons(LogicGraphEditorButtonFlagBits::Load | LogicGraphEditorButtonFlagBits::Save | LogicGraphEditorButtonFlagBits::SaveAs) {}
    
    ImVec2 canvasSize;
    ImVec2 lineDensity;
    float zoomMultiplier;
    float nodeWidth;
    ImVec2 scrollMultipliers;
    float lineThickness;
    bool showDebugOptions;
    bool showNewAsClear;
    LogicGraphEditorButtonFlags shownButtons;
};

class NewGraphSettings {
public:
    virtual ~NewGraphSettings() {}
};

enum class DragMode : std::uint8_t {
    NoDrag,
    Node,
    Connector,
    Background
};

template <typename T>
class LogicGraphEditor {
public:
    using NodeKey = typename T::NodeKey;
    using NodeTypeEnum = typename T::NodeTypeEnum;
    
    static constexpr const char* LocalizationNamespace = "logic_graph_editor";
    
    LogicGraphEditor(NodeEditorSettings settings = NodeEditorSettings()) 
        : scale(1.0f), nodeInfoWidth(200.0f), canvasPosition(0.0f, 0.0f), nodeCreationMenuLocation(0.0f, 0.0f), settings(std::move(settings)), hoveredNodeKey(T::InvalidKey),
          contextMenuNodeKey(T::InvalidKey), dragMode(DragMode::NoDrag), wasMouse1DraggingLastFrame(false) {
        name.resize(128, '\0');
    }
    virtual ~LogicGraphEditor() {}
    
    void show(bool* open);
    
    virtual std::string getWindowName() const = 0;
    
    /// Serializes the currently active graph and extra parameters from this editor to a JSON string. Should be used when handling the save button.
    std::string serializeJSON() const {
        if (graph == nullptr) {
            return "";
        } else {
            rj::StringBuffer sb;
            PrettyStringWriter pw(sb);
            
            serializeJSON(pw);
            
            const char* jsonString = sb.GetString();
            const std::size_t jsonByteCount = sb.GetSize();
            
            return std::string(jsonString, jsonByteCount);
        }
    }
    
    /// Serializes the currently active graph (if any) and extra parameters from this editor into a PrettyStringWriter.
    void serializeJSON(PrettyStringWriter& pw) const {
        if (graph == nullptr) {
            return;
        }
        
        pw.SetIndent('\t', 1);
            
        assert(!graph->makesJSONRoot());
        
        pw.StartObject();
        
        StoreEditorValues(scale, canvasPosition, pw);
        
        graph->serializeJSON(pw);
        
        pw.EndObject();
    }
    
    /// Destroys the current graph and deserializes a new one from a JSON string. Should be used when handling the load button.
    void deserializeJSON(const std::string& json) {
        deserializeJSON(json.data(), json.length());
    }
    
    /// Destroys the current graph and deserializes a new one from a JSON char array. Should be used when handling the load button.
    void deserializeJSON(const char* data, std::int64_t length) {
        graph = makeNewGraph(NewGraphSettings());
        clear();
        
        rj::Document document;
        document.Parse(data, static_cast<std::size_t>(length));
        
        LoadEditorValues(scale, canvasPosition, document);
        
        graph->deserializeJSON(document);
    }
protected:
    static constexpr const char* SCALE_FIELD_NAME = "editorScale";
    static constexpr const char* POS_X_FIELD_NAME = "editorXPos";
    static constexpr const char* POS_Y_FIELD_NAME = "editorYPos";
    
    static void StoreEditorValues(float scale, const ImVec2& canvasPosition, PrettyStringWriter& pw) {
        pw.Key(SCALE_FIELD_NAME);
        pw.Double(scale);
        
        pw.Key(POS_X_FIELD_NAME);
        pw.Double(canvasPosition.x);
        
        pw.Key(POS_Y_FIELD_NAME);
        pw.Double(canvasPosition.y);
    }
    
    static void LoadEditorValues(float& scale, ImVec2& canvasPosition, JSONObject& jo) {
        scale = jo[SCALE_FIELD_NAME].GetFloat();
        canvasPosition.x = jo[POS_X_FIELD_NAME].GetFloat();
        canvasPosition.y = jo[POS_Y_FIELD_NAME].GetFloat();
    }
    
    virtual std::unique_ptr<T> makeNewGraph(const NewGraphSettings& settings) = 0;
    /// Called when a button from the button row is clicked. This method is usually used to open a modal dialog that is drawn
    /// and handled in onDrawButtonRow() 
    virtual void onButtonClick([[maybe_unused]] LogicGraphEditorButtonFlagBits button) {}
    
    virtual void onDrawButtonRow() {}
    
    void drawNodeEditor();
    void drawBackgroundLines();
    void handleTransformations();
    void drawNodes();
    void drawConnections();
    void drawCanvasInfo();
    void drawNodeProperties();
    virtual void drawNodeExtraProperties([[maybe_unused]] typename T::NodeType& node) {}
    void showConnectionErrorTooltip(const char* text) const;
    bool validateConnection(bool connectIfValidated = false);
    void showLocalizedDocumentation(LocalizationHandle handle);
    void showNodeContextMenu();
    void clear();
    
    float computeConnectorSlotRadius();
    void drawConnectionCurve(const ImVec2& start, const ImVec2& end, ImU32 color);
    ImVec2 mousePosToCanvasPos();
    bool selectionContainsKey(NodeKey key) const;
    
    inline bool isConnectorHovered() const {
        return hoveredConnector.isValid();
    }
    
    struct ZSortComparator {
        bool operator()(typename T::NodeType* a, typename T::NodeType* b) const {
            return a->getZIndex() < b->getZIndex();
        }
    };
    
    struct ConnectorKey {
        inline ConnectorKey() : nodeKey(T::InvalidKey), connectorID(T::NodeType::Connector::InvalidID), isInput(false) {}
        inline ConnectorKey(NodeKey nodeKey, std::uint8_t connectorID, bool isInput)
            : nodeKey(nodeKey), connectorID(connectorID), isInput(isInput) {}
        
        inline bool isValid() const {
            return (nodeKey != T::InvalidKey) && (connectorID != T::NodeType::Connector::InvalidID);
        }
        
        NodeKey nodeKey;
        std::uint8_t connectorID;
        bool isInput;
        
        inline friend bool operator==(const ConnectorKey& a, const ConnectorKey& b) {
            return (a.nodeKey == b.nodeKey) && (a.connectorID == b.connectorID) && (a.isInput == b.isInput);
        }
    };
    
    struct ConnectorKeyHasher {
        inline std::size_t operator()(const ConnectorKey& k) const {
            std::size_t seed = 0;
            
            util::HashCombine(seed, std::hash<NodeKey>()(k.nodeKey));
            util::HashCombine(seed, std::hash<std::uint8_t>()(k.connectorID));
            util::HashCombine(seed, std::hash<bool>()(k.isInput));
            
            return seed;
        }
    };
    
    inline bool buttonUsed(LogicGraphEditorButtonFlagBits button) {
        return bool(settings.shownButtons & button);
    }
    
    std::unique_ptr<T> graph;
    std::unordered_set<NodeKey> selectedNodes;
    std::vector<char> name;
    std::set<typename T::NodeType*, ZSortComparator> zSortedNodes;
    std::unordered_map<ConnectorKey, std::pair<ImVec2, ImU32>, ConnectorKeyHasher> connectorDataCache;
    float scale;
    float nodeInfoWidth;
    ImVec2 canvasPosition;
    ImVec2 nodeCreationMenuLocation;
    ImVec2 canvasSize;
    ImVec2 lastScrollMax;
    const NodeEditorSettings settings;
    
    NodeKey hoveredNodeKey;
    NodeKey contextMenuNodeKey;
    ConnectorKey hoveredConnector;
    ConnectorKey newConnectionStart;
    DragMode dragMode;
    bool wasMouse1DraggingLastFrame;
    std::string lastValidationResult;
    
    std::string lastSort;
};

template <typename T>
bool LogicGraphEditor<T>::selectionContainsKey(NodeKey key) const {
    return (selectedNodes.find(key) != selectedNodes.end());
}

template <typename T>
void LogicGraphEditor<T>::showLocalizedDocumentation(LocalizationHandle handle) {
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        
        const std::string localizedDoc = LOC_SYS(handle);
        
        if (localizedDoc.empty()) {
            ImGui::TextColored(ImColor(255, 0, 0, 255), "No documentation found");
        } else {
            ImGui::TextUnformatted(localizedDoc.c_str(), localizedDoc.c_str() + localizedDoc.length());
        }
        
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

template <typename T>
void LogicGraphEditor<T>::clear() {
    selectedNodes.clear();
}

template <typename T>
void LogicGraphEditor<T>::show(bool* open) {
    IYFT_PROFILE(showGraphEditor, iyft::ProfilerTag::LogicGraph);
    
    const std::string windowName = getWindowName();
    if (ImGui::Begin(windowName.c_str(), open)) {
        
        if (settings.showNewAsClear) {
            if (ImGui::Button("Clear")) {
                graph = makeNewGraph(NewGraphSettings());
                clear();
            }
        } else {
            if (ImGui::Button("New")) {
                graph = makeNewGraph(NewGraphSettings());
                clear();
            }
        }
        
        ImGui::SameLine();
        
        if (buttonUsed(LogicGraphEditorButtonFlagBits::Load) && ImGui::Button("Load")) {
            onButtonClick(LogicGraphEditorButtonFlagBits::Load);
        }
        
        if (graph == nullptr) {
            onDrawButtonRow();
            ImGui::Text("No loaded graph");
        } else {
            ImGui::SameLine();
        
            if (buttonUsed(LogicGraphEditorButtonFlagBits::Save) && ImGui::Button("Save")) {
                onButtonClick(LogicGraphEditorButtonFlagBits::Save);
            }
            
            ImGui::SameLine();
            
            if (buttonUsed(LogicGraphEditorButtonFlagBits::SaveAs) && ImGui::Button("Save As")) {
                onButtonClick(LogicGraphEditorButtonFlagBits::SaveAs);
            }
            
            ImGui::SameLine();
            ImGui::Text("|");
            ImGui::SameLine();
            
            if (ImGui::Button("Validate")) {
                ImGui::OpenPopup("Validation Result");
                std::stringstream ss;
                ss << "Node graph validation failed with error(s):\n\n";
                if (graph->validate(&ss)) {
                    lastValidationResult = "The node graph was validated successfully.";
                } else {
                    lastValidationResult = ss.str();
                }
            }
            
            if (ImGui::BeginPopupModal("Validation Result", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("%s", lastValidationResult.c_str());
                
                if (ImGui::Button("OK")) {
                    ImGui::CloseCurrentPopup();
                }
                
                ImGui::EndPopup();
            }
            
            if (settings.showDebugOptions) {
                ImGui::SameLine();
                
                if (ImGui::Button("Sort")) {
                    auto results = graph->getTopologicalSort();
                    
                    if (graph->getNodeCount() == 0) {
                        assert(results.empty());
                        lastSort = "no nodes in graph sort.";
                    } else if (results.empty()) {
                        lastSort = "an error occured when trying to perform the topological sort.";
                    } else {
                        std::stringstream ss;
                        for (auto i = results.rbegin(); i != results.rend(); ++i) {
                            const NodeKey key = *i;
                            
                            const auto& node = *(graph->getNode(key));
                            if (node.hasName()) {
                                const char* str = node.getName().c_str();
                                
                                ss << "ID:" << key << "; " << str << "\n";
                            } else {
                                const std::string string = LOC_SYS(graph->getNodeTypeInfo(node.getType()).name);
                                const char* str = string.c_str();
                                
                                ss << "ID:" << key << "; " << str << "\n";
                            }
                        }
                        
                        lastSort = ss.str();
                    }
                }
            }
            
            onDrawButtonRow();
            
            drawNodeEditor();
        }
    }
    
    ImGui::End();
    
//     ImGui::ShowDemoWindow();
}

template <typename T>
bool LogicGraphEditor<T>::validateConnection(bool connectIfValidated) {
    const bool hoveringStart = (hoveredConnector == newConnectionStart);
    const bool sameNode = (hoveredConnector.nodeKey == newConnectionStart.nodeKey);
    
    if (!hoveringStart && sameNode) {
        showConnectionErrorTooltip("Must connect to a different node");
        return false;
    }
    
    if (!hoveringStart && !sameNode && (hoveredConnector.isInput && newConnectionStart.isInput)) {
        showConnectionErrorTooltip("Can't connect two inputs");
        return false;
    }
    
    if (!hoveringStart && !sameNode && (!hoveredConnector.isInput && !newConnectionStart.isInput)) {
        showConnectionErrorTooltip("Can't connect two outputs");
        return false;
    }
    
    const auto connectionStartNode = graph->getNode(newConnectionStart.nodeKey);
    const auto connectionEndNode = graph->getNode(hoveredConnector.nodeKey);
    
    NodeConnectionResult result;
    if (!newConnectionStart.isInput) {
        result = graph->validateConnection(connectionStartNode, newConnectionStart.connectorID,
                                           connectionEndNode, hoveredConnector.connectorID);
    } else {
        result = graph->validateConnection(connectionEndNode, hoveredConnector.connectorID,
                                           connectionStartNode, newConnectionStart.connectorID);
    }
    
    bool success = false;
    bool replace = false;
    switch (result) {
        case NodeConnectionResult::Success:
            success = true;
            break;
        case NodeConnectionResult::TypeMismatch: {
            std::stringstream ss;
            ss << "The types of the node connectors don't match.\n";
            
            if (!newConnectionStart.isInput) {
                ss << "Output is: " << graph->getConnectorTypeName(connectionStartNode->getOutputs()[newConnectionStart.connectorID].getType()) << "\n";
                ss << "Input is: " << graph->getConnectorTypeName(connectionEndNode->getInputs()[hoveredConnector.connectorID].getType());
            } else {
                ss << "Output is: " << graph->getConnectorTypeName(connectionEndNode->getOutputs()[hoveredConnector.connectorID].getType()) << "\n";
                ss << "Input is: " << graph->getConnectorTypeName(connectionStartNode->getInputs()[newConnectionStart.connectorID].getType());
            }
            
            const std::string string = ss.str();
            showConnectionErrorTooltip(string.c_str());
            break;
        }
        case NodeConnectionResult::InvalidSource:
            showConnectionErrorTooltip("Invalid source node");
            break;
        case NodeConnectionResult::InvalidSourceOutput:
            showConnectionErrorTooltip("Invalid output ID");
            break;
        case NodeConnectionResult::NullSource:
            showConnectionErrorTooltip("Source was null");
            break;
        case NodeConnectionResult::InvalidDestination:
            showConnectionErrorTooltip("Invalid destination node");
            break;
        case NodeConnectionResult::InvalidDestinationInput:
            showConnectionErrorTooltip("Invalid input ID");
            break;
        case NodeConnectionResult::DestinationIsDisabled:
            showConnectionErrorTooltip("Destination is disabled");
            break;
        case NodeConnectionResult::SourceIsDisabled:
            showConnectionErrorTooltip("Source is disabled");
            break;
        case NodeConnectionResult::NullDestination:
            showConnectionErrorTooltip("Destination was null");
            break;
        case NodeConnectionResult::OccupiedDestination:
            showConnectionErrorTooltip("A connection will be replaced");
            replace = true;
            break;
        case NodeConnectionResult::InsertionFailed:
            showConnectionErrorTooltip("Failed to connect");
            break;
        case NodeConnectionResult::UnableToConnectToSelf:
            showConnectionErrorTooltip("Must connect to a different node");
            break;
    }
    
    if ((success || replace) && connectIfValidated) {
        NodeConnectionResult connectionResult;
        if (!newConnectionStart.isInput) {
            using KeyConnectorPair = typename T::KeyConnectorPair;
            
            if (replace) {
                KeyConnectorPair source = graph->getSource(KeyConnectorPair(connectionEndNode->getKey(), hoveredConnector.connectorID));
                bool removed = graph->removeConnection(graph->getNode(source.getNodeKey()), source.getConnectorID(),
                                                       connectionEndNode, hoveredConnector.connectorID);
                
                assert(removed);
            }
            
            connectionResult = graph->addConnection(connectionStartNode, newConnectionStart.connectorID,
                                                    connectionEndNode, hoveredConnector.connectorID);
        } else {
            // When you start dragging from a connected input, it gets disconnected and this cannot happen,
            // unless someone messed up the disconnection logic
            if (replace) {
                assert(false);
            }
            
            connectionResult = graph->addConnection(connectionEndNode, hoveredConnector.connectorID,
                                                    connectionStartNode, newConnectionStart.connectorID);
        }
        
        assert(connectionResult == NodeConnectionResult::Success);
    }
    
    return success || (connectIfValidated && replace);
}

template <typename T>
void LogicGraphEditor<T>::showConnectionErrorTooltip(const char* text) const {
    ImGui::BeginTooltip();
    ImGui::SetWindowFontScale(scale);
    ImGui::Text("%s", text);
    ImGui::EndTooltip();
}

template <typename T>
ImVec2 LogicGraphEditor<T>::mousePosToCanvasPos() {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    
    const ImVec2 clipMin = dl->GetClipRectMin();
    const ImVec2 mousePos = ImGui::GetMousePos();
    const ImVec2 mouseRelativePos(mousePos.x - clipMin.x, mousePos.y - clipMin.y);
    
    return ImVec2(canvasPosition.x + (mouseRelativePos.x / scale),
                  canvasPosition.y + (mouseRelativePos.y / scale));
}

template <typename T>
void LogicGraphEditor<T>::showNodeContextMenu() {
    auto node = graph->getNode(contextMenuNodeKey);
    const auto& nodeTypeInfo = graph->getNodeTypeInfo(node->getType());
    
    const std::string typeName = LOC_SYS(nodeTypeInfo.name);
    ImGui::TextDisabled("Type: %s; Key: %d", typeName.c_str(), node->getKey());
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    showLocalizedDocumentation(nodeTypeInfo.documentation);
    
    ImGui::Separator();
    
    bool deleted = false;
    const bool deletable = nodeTypeInfo.deletable;
    if (ImGui::MenuItem("Delete", nullptr, false, deletable)) {
        selectedNodes.erase(contextMenuNodeKey);
        
        const bool result = graph->removeNode(contextMenuNodeKey);
        assert(result);
        
        hoveredNodeKey = T::InvalidKey;
        deleted = true;
    }
    
    if (deleted) {
        return;
    }
    
    if (!deletable && ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        
        const std::string cannotDelete = LOC_SYS(LH("cannot_delete", LocalizationNamespace));
        ImGui::Text("%s", cannotDelete.c_str());
        
        ImGui::EndTooltip();
    }
    
    const bool hasModes = node->supportsMultipleModes();
    if (ImGui::BeginMenu("Mode", hasModes)) {
        const auto& modes = node->getSupportedModes();
        
        for (std::size_t i = 0; i < modes.size(); ++i) {
            const std::string modeName = LOC_SYS(modes[i].name);
            if (ImGui::MenuItem(modeName.c_str())) {
                const bool changed = node->setSelectedModeID(i);
                
                if (!changed) {
                    throw std::logic_error("Failed to change the mode");
                }
                
                graph->revalidateNodeConnections(node);
            }
            
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                
                const std::string docString = LOC_SYS(modes[i].documentation);
                ImGui::Text("%s", docString.c_str());
                ImGui::EndTooltip();
            }
        }
        
        ImGui::EndMenu();
    }
}

template <typename T>
void LogicGraphEditor<T>::handleTransformations() {
    IYFT_PROFILE(handleTransformations, iyft::ProfilerTag::LogicGraph);
    
    const ImGuiIO& io = ImGui::GetIO();
    
    const bool hovered = ImGui::IsWindowHovered();
    const bool ctrl = io.KeyCtrl;
    const bool shift = io.KeyShift;
    const bool collapsed = ImGui::IsWindowCollapsed();
    const bool hasWheelMovedV = io.MouseWheel != 0.0f;
    const bool hasWheelMovedH = io.MouseWheelH != 0.0f;
    const bool anyClicks = ImGui::IsAnyMouseDown();
    const bool anyNodeHovered = (hoveredNodeKey != T::InvalidKey);
    const bool lmbDragging = ImGui::IsMouseDragging(0, 1.0f);
    const bool rmbDragging = ImGui::IsMouseDragging(1);
    
    if (hovered && !ctrl && !shift && !anyClicks && hasWheelMovedV && !collapsed) {
        const ImVec2 offsetPreMod = mousePosToCanvasPos();
        
        scale += io.MouseWheel * settings.zoomMultiplier;
        scale = std::clamp(scale, 0.5f, 3.0f);
        
        const ImVec2 offsetPostMod = mousePosToCanvasPos();
        
        const ImVec2 canvasPre(canvasPosition);
        
        const ImVec2 posDelta(offsetPreMod.x - offsetPostMod.x, offsetPreMod.y - offsetPostMod.y);
        canvasPosition.x += posDelta.x;
        canvasPosition.y += posDelta.y;
    }
    
    if (hovered && !ctrl && !shift && !hasWheelMovedV && !hasWheelMovedH && rmbDragging && !collapsed) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        
        const ImVec2 delta = ImGui::GetMouseDragDelta(1);
        
        canvasPosition.x -= delta.x / scale;
        canvasPosition.y -= delta.y / scale;
        
        ImGui::ResetMouseDragDelta(1);
    }
    
    const bool canSelect = hovered && !shift && !collapsed;
    if (canSelect && ImGui::IsMouseClicked(0) && !lmbDragging) {
        if (anyNodeHovered && !ctrl && !selectionContainsKey(hoveredNodeKey)) {
            selectedNodes.clear();
            
            auto insertionResult = selectedNodes.insert(hoveredNodeKey);
            assert(insertionResult.second);
            
            graph->nodeToTop(hoveredNodeKey);
        } else if (anyNodeHovered && ctrl && !selectionContainsKey(hoveredNodeKey)) {
            auto insertionResult = selectedNodes.insert(hoveredNodeKey);
            assert(insertionResult.second);
            
            graph->nodeToTop(hoveredNodeKey);
        } else if (!anyNodeHovered) {
            selectedNodes.clear();
        }
    }
    
    const bool canDrag = hovered && !ctrl && !shift && !collapsed;
    if (canDrag && lmbDragging) {
        if (dragMode == DragMode::Connector) {
            
            const auto startResult = connectorDataCache.find(newConnectionStart);
            assert(startResult != connectorDataCache.end());
            
            // We use the center of connection because it looks better than ImGui::GetMouseDragDelta()
            const ImVec2 start = startResult->second.first;
            const ImVec2 end = ImGui::GetMousePos();
            const ImU32 color = startResult->second.second;
            
            drawConnectionCurve(start, end, color);
            
            if (isConnectorHovered()) {
                const auto result = connectorDataCache.find(hoveredConnector);
                assert(result != connectorDataCache.end());
                
                validateConnection();
            }
        } else if (dragMode == DragMode::Node) {
            const ImVec2 delta = ImGui::GetMouseDragDelta(0);
            const Vec2 translation(delta.x / scale, delta.y / scale);
            
            for (const NodeKey k : selectedNodes) {
                graph->getNode(k)->translate(translation);
            }
            
            ImGui::ResetMouseDragDelta();
        } else if (dragMode == DragMode::NoDrag) {
            if (isConnectorHovered()) {
                dragMode = DragMode::Connector;
                
                if (hoveredConnector.isInput) {
                    using KeyConnectorPair = typename T::KeyConnectorPair;
                    
                    KeyConnectorPair source = graph->getSource(KeyConnectorPair(hoveredConnector.nodeKey, hoveredConnector.connectorID));
                    
                    // This unplugs an existing connection, similarly to how Blender does it
                    if (source.isValid()) {
                        bool removed = graph->removeConnection(graph->getNode(source.getNodeKey()), source.getConnectorID(),
                                                               graph->getNode(hoveredConnector.nodeKey), hoveredConnector.connectorID);
                        assert(removed);
                        newConnectionStart = ConnectorKey(source.getNodeKey(), source.getConnectorID(), false);
                    } else {
                        newConnectionStart = hoveredConnector;
                    }
                } else {
                    newConnectionStart = hoveredConnector;
                }
            } else if (anyNodeHovered) {
                dragMode = DragMode::Node;
            } else {
                dragMode = DragMode::Background;
            }
        }
    } else {
        if (dragMode == DragMode::Connector && isConnectorHovered()) {
            validateConnection(true);
        }
        
        dragMode = DragMode::NoDrag;
    }
    
    const ImGuiWindowFlags contextMenuFlags = ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoSavedSettings;
    if (canSelect && anyNodeHovered && !wasMouse1DraggingLastFrame && ImGui::IsMouseReleased(1) && !rmbDragging) {
        ImGui::OpenPopup("Node Context Menu");
        contextMenuNodeKey = hoveredNodeKey;
    }
    
    if (ImGui::BeginPopup("Node Context Menu", contextMenuFlags)) {
        showNodeContextMenu();
        
        ImGui::EndPopup();
    }
    
    if (canSelect && !anyNodeHovered && !wasMouse1DraggingLastFrame && ImGui::IsMouseReleased(1) && !rmbDragging) {
        ImGui::OpenPopup("Node Creation Menu");
        
        nodeCreationMenuLocation = mousePosToCanvasPos();
    }
    
    if (ImGui::BeginPopup("Node Creation Menu", contextMenuFlags)) {
        ImGui::TextDisabled("Create a new node");
        ImGui::Separator();
        
        using NodeGroupInt = std::underlying_type_t<typename T::NodeTypeInfoEnum>;
        for (NodeGroupInt i = 0; i < graph->getNodeGroupCount(); ++i) {
            auto infoEnumValue = static_cast<typename T::NodeTypeInfoEnum>(i);
            const auto& typeInfoVec = graph->getNodeGroupTypeInfos(infoEnumValue);
            
            bool hasInstantiableTypes = false;
            
            for (const auto& typeInfo : typeInfoVec) {
                if (typeInfo.instantiable) {
                    hasInstantiableTypes = true;
                    break;
                }
            }
            
            if (typeInfoVec.empty() || !hasInstantiableTypes) {
                continue;
            }
            
            const std::string groupName = LOC_SYS(graph->getNodeGroupNameHandle(infoEnumValue));
            assert(!groupName.empty());
            if (ImGui::BeginMenu(groupName.c_str())) {
                for (const auto& typeInfo : typeInfoVec) {
                    if (!typeInfo.instantiable) {
                        continue;
                    }
                    
                    const std::string nodeTypeName = LOC_SYS(typeInfo.name);
                    assert(!nodeTypeName.empty());
                    
                    if (ImGui::MenuItem(nodeTypeName.c_str())) {
                        graph->addNode(typeInfo.type, Vec2(nodeCreationMenuLocation.x, nodeCreationMenuLocation.y));
                    }
                    
                    showLocalizedDocumentation(typeInfo.documentation);
                }
                
                ImGui::EndMenu();
            }
        }
        
        ImGui::EndPopup();
    }
    
    if (rmbDragging) {
        wasMouse1DraggingLastFrame = true;
    } else {
        wasMouse1DraggingLastFrame = false;
    }
}

template <typename T>
void LogicGraphEditor<T>::drawCanvasInfo() {
    ImGui::Text("Canvas State");
    
    ImGui::AlignTextToFramePadding();
    ImGui::Text("X: %.0f; Y: %.0f; S: %.2f", canvasPosition.x, canvasPosition.y, scale);
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        canvasPosition = ImVec2(0.0f, 0.0f);
        scale = 1.0f;
    }
}

template <typename T>
void LogicGraphEditor<T>::drawNodeEditor() {
    IYFT_PROFILE(drawEditor, iyft::ProfilerTag::LogicGraph);
    
    bool nodeInfoPopped = false;
    bool nodeEditorPopped = false;
    
    // Push and and immediately pop to avoid inactive borders around the splitter, but keep them
    // around items inside
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    if (ImGui::BeginChild("Node Info", ImVec2(nodeInfoWidth, 0.0f), true)) {
        ImGui::PopStyleVar();
        nodeInfoPopped = true;
        
        drawCanvasInfo();
        ImGui::Separator();
        drawNodeProperties();
        ImGui::Separator();
        
        if (!lastSort.empty()) {
            ImGui::Text("Last Node Sort Result");
            ImGui::TextUnformatted(lastSort.c_str(), lastSort.c_str() + lastSort.length());
            ImGui::Separator();
        }
        
        if (settings.showDebugOptions) {
            ImGui::SetNextTreeNodeOpen(false, ImGuiCond_Once);
            if (ImGui::TreeNode("Editor Debug")) {
                ImGui::Text("Selected node");
                
                if (selectedNodes.size() == 1) {
                    const NodeKey selectionKey = *(selectedNodes.begin());
                    const auto& selectedNode = *(graph->getNode(selectionKey));
                    
                    ImGui::Text("\tKey: %u", selectionKey);
                    ImGui::Text("\tZ-index: %u", selectedNode.getZIndex());
                    
                    const Vec2 pos = selectedNode.getPosition();
                    ImGui::Text("\tPosition: %.2f %.2f", pos.x, pos.y);
                } else if (selectedNodes.size() > 1) {
                    ImGui::Text("\tMultiple nodes selected");
                } else {
                    ImGui::Text("\tNone");
                }
                
                ImGui::Text("Hovered node");
                if (hoveredNodeKey != T::InvalidKey) {
                    ImGui::Text("\tKey: %u", hoveredNodeKey);
                    ImGui::Text("\tZ-index: %u", graph->getNode(hoveredNodeKey)->getZIndex());
                    ImGui::Text("\tHovered connector");
                    if (isConnectorHovered()) {
                        ImGui::Text("\t\tID (type): %u (%s)", hoveredConnector.connectorID, (hoveredConnector.isInput ? "input" : "output"));
                    } else {
                        ImGui::Text("\t\tNone");
                    }
                } else {
                    ImGui::Text("\tNone");
                }
                
                ImGui::TreePop();
            }
        }
    }
    ImGui::EndChild();
    
    if (!nodeInfoPopped) {
        ImGui::PopStyleVar();
    }
    
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::SameLine();
    ImGui::InvisibleButton("Vertical Splitter", ImVec2(8.0f, -1.0f)); 
    if (ImGui::IsItemActive()) {
        nodeInfoWidth += ImGui::GetIO().MouseDelta.x;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
    ImGui::SameLine();
    ImGui::PopStyleVar();
    
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    if (ImGui::BeginChild("Node Editor Container", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar)) {
        ImGui::PopStyleVar(2);
        nodeEditorPopped = true;
        
        canvasSize = ImVec2(settings.canvasSize.x * scale, settings.canvasSize.y * scale);
        ImGui::SetWindowFontScale(scale);
        
        drawBackgroundLines();
        
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->ChannelsSplit(2);
        dl->ChannelsSetCurrent(1);
        drawNodes();
        dl->ChannelsSetCurrent(0);
        drawConnections();
        dl->ChannelsMerge();
        
        handleTransformations();
    }
    
    if (!nodeEditorPopped) {
        ImGui::PopStyleVar(2);
    }
    
    ImGui::EndChild();
}

template <typename T>
void LogicGraphEditor<T>::drawNodeProperties() {
    if (selectedNodes.empty()) {
        ImGui::Text("No nodes selected.");
        return;
    } else if (selectedNodes.size() > 1) {
        ImGui::Text("%zu nodes selected.", selectedNodes.size());
        return;
    }
    
    ImGui::Text("Selected Node Properties");
    
    const NodeKey selectedNodeKey = *(selectedNodes.begin());
    auto& node = *(graph->getNode(selectedNodeKey));
    
    const std::string nodeTypeName = LOC_SYS(graph->getNodeTypeInfo(node.getType()).name);
    ImGui::Text("Type: %s", nodeTypeName.c_str());
    
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    
    showLocalizedDocumentation(graph->getNodeTypeInfo(node.getType()).documentation);
    
    if (node.hasName()) {
        std::memcpy(name.data(), node.getName().c_str(), node.getName().length() + 1);
    } else {
        std::memset(name.data(), 0, 1);
    }
    
    if (ImGui::InputText("Name", name.data(), name.size())) {
        node.setName(name.data());
    }
    
    if (node.supportsMultipleModes()) {
//         ImGui::PushItemWidth(nodeWidth - (2.0f * windowPadding.x));
        const std::size_t modeID = node.getSelectedModeID();
        const auto& modes = node.getSupportedModes();
        
        if (ImGui::BeginCombo("Mode", LOC_SYS(modes[modeID].name).c_str())) {
            for (std::size_t i = 0; i < modes.size(); ++i) {
                const bool selected = (i == modeID);
                
                const std::string modeName = LOC_SYS(modes[i].name);
                if (ImGui::Selectable(modeName.c_str(), selected)) {
                    const bool changed = node.setSelectedModeID(i);
                    
                    if (!changed) {
                        throw std::logic_error("Failed to change the mode");
                    }
                    
                    graph->revalidateNodeConnections(&node);
                }
                
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    
                    const std::string docString = LOC_SYS(modes[i].documentation);
                    ImGui::Text("%s", docString.c_str());
                    ImGui::EndTooltip();
                }
                
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            
            ImGui::EndCombo();
        }
    }
    
    drawNodeExtraProperties(node);
}

template <typename T>
void LogicGraphEditor<T>::drawNodes() {
    IYFT_PROFILE(drawNodes, iyft::ProfilerTag::LogicGraph);
    
    const ImGuiStyle& style = ImGui::GetStyle();
    
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 clipMin = dl->GetClipRectMin();
//     const ImVec2 clipMax = dl->GetClipRectMax();
//     const ImVec2 clipDimensions = clipMax - clipMin;
    
    const ImU32 fontColor = ImColor(style.Colors[ImGuiCol_Text]);
    
    const ImU32 connectorColor = ImColor(style.Colors[ImGuiCol_Button]);
    const ImU32 hoveredConnectorColor = ImColor(style.Colors[ImGuiCol_ButtonHovered]);
    
    const ImVec2 framePadding(style.FramePadding.x * scale, style.FramePadding.y * scale);
    const float lineWithoutSpacing = ImGui::GetTextLineHeight();
    const ImVec2 itemSpacing(style.ItemSpacing.x * scale, style.ItemSpacing.y * scale);
    const float lineWithSpacing = lineWithoutSpacing + itemSpacing.y;
    const ImVec2 windowPadding(style.WindowPadding.x * scale, style.WindowPadding.y * scale);
//         const ImVec2 nodePadding = ImGui::GetStyle().WindowPadding;
    
    hoveredConnector = ConnectorKey();
    hoveredNodeKey = T::InvalidKey;
    
    {
        IYFT_PROFILE(nodeZSort, iyft::ProfilerTag::LogicGraph);
        zSortedNodes.clear();
        
        for (auto& n : graph->getNodes()) {
            zSortedNodes.insert(n.second);
        }
    }
    
    connectorDataCache.clear();
    
    for (auto& n : zSortedNodes) {
        IYFT_PROFILE(drawSingleNode, iyft::ProfilerTag::LogicGraph);
        
        auto& node = *n;
        const auto& inputs = node.getInputs();
        const auto& outputs = node.getOutputs();
        
        // First of all, estimate the size
        const float headerSectionHeight = framePadding.y + lineWithSpacing;
        const float inputSectionHeight = inputs.size() * lineWithSpacing;
        const float outputSectionHeight = outputs.size() * lineWithSpacing;
        
        const ImVec2 scaledPosition(canvasPosition.x * scale, canvasPosition.y * scale);
        const ImVec2 offset(clipMin.x - scaledPosition.x, clipMin.y - scaledPosition.y);
        
        const ImVec2 nodeStart(offset.x + (node.getPosition().x * scale), offset.y + (node.getPosition().y * scale));
        
        const float nodeHeight = headerSectionHeight + windowPadding.y + inputSectionHeight + outputSectionHeight;
        const float nodeWidth = settings.nodeWidth * scale;
        const ImVec2 nodeEnd(nodeStart.x + nodeWidth, nodeStart.y + nodeHeight);
        
        // TODO figure out a way to skip invisible nodes, while maintaining the connector position cache
        if (ImGui::IsMouseHoveringRect(nodeStart, nodeEnd)) {
            hoveredNodeKey = node.getKey();
            
            // Clear out hovered connector data from any nodes that are below this one
            if (isConnectorHovered()) {
                hoveredConnector = ConnectorKey();
            }
        }
        
        // Node's visible. Push the ID and draw.
        ImGui::PushID(node.getKey());
        ImGui::SetCursorScreenPos(nodeStart);

        const bool selected = selectionContainsKey(node.getKey());
        const ImU32 headerColor = (selected ? 
                                   ImColor(style.Colors[ImGuiCol_TitleBgActive]) :
                                   ImColor(style.Colors[ImGuiCol_TitleBg]));
        
        const ImU32 nodeColor = ImColor(style.Colors[ImGuiCol_WindowBg]);
        const ImVec4 nodeBorderColorVec = style.Colors[ImGuiCol_Border];
        const ImU32 nodeBorderColor = ImColor(nodeBorderColorVec);
        
        const ImVec2 headerEnd(nodeEnd.x, nodeStart.y + headerSectionHeight);
        const ImVec2 contentStart(nodeStart.x, headerEnd.y);
        
        dl->AddRectFilled(nodeStart, headerEnd, headerColor, ImGui::GetStyle().WindowRounding, ImDrawCornerFlags_Top);
        dl->AddRectFilled(contentStart, nodeEnd, nodeColor, ImGui::GetStyle().WindowRounding, ImDrawCornerFlags_Bot);
        dl->AddRect(nodeStart, nodeEnd, nodeBorderColor, ImGui::GetStyle().WindowRounding, ImDrawCornerFlags_All);
        
        ImVec2 cursor = nodeStart;
        
        // Draw the name of the node
        const ImVec2 namePos(cursor.x + framePadding.x, cursor.y + framePadding.y);
        if (node.hasName()) {
            const char* str = node.getName().c_str();
            dl->AddText(namePos, fontColor, str, str + node.getName().length());
        } else {
            const std::string string = LOC_SYS(graph->getNodeTypeInfo(node.getType()).name);
            const char* str = string.c_str();
            
            dl->AddText(namePos, fontColor, str, str + string.length());
        }
        
        cursor.y += lineWithSpacing + windowPadding.y;
        
        const float connectorRadius = computeConnectorSlotRadius();
        
        // Outputs
        const float outputConnectorHorizontalOffset = cursor.x + nodeWidth;
        const float outputConnectorNameHorizontalOffset = (cursor.x + nodeWidth) - (windowPadding.x + connectorRadius);
        for (std::uint8_t i = 0; i < outputs.size(); ++i) {
            const auto& o = outputs[i];
            
            const ImVec2 connectorPos(outputConnectorHorizontalOffset, cursor.y + lineWithoutSpacing * 0.5f);
            bool isHovered = false;
            
            if (ImGui::IsMouseHoveringRect(ImVec2(connectorPos.x - connectorRadius, connectorPos.y - connectorRadius),
                                           ImVec2(connectorPos.x + connectorRadius, connectorPos.y + connectorRadius))) {
                hoveredNodeKey = node.getKey();
            
                if (o.isEnabled()) {
                    hoveredConnector = ConnectorKey(node.getKey(), o.getID(), false);
                }
                
                isHovered = true;
            }
            
            ImU32 color = graph->getConnectorTypeColor(o.getType(), o.isEnabled());
            dl->AddCircleFilled(connectorPos, connectorRadius, color, 16);
            dl->AddCircle(connectorPos, connectorRadius, ((isHovered && o.isEnabled()) ? hoveredConnectorColor : connectorColor), 16, 2.0f * scale);
            
            if (o.hasName()) {
                const char* str = o.getName().c_str();
                const char* strEnd = str + o.getName().length();
                
                const ImVec2 stringDimensions = ImGui::CalcTextSize(str, strEnd);
                
                const ImVec2 connectorNamePos(outputConnectorNameHorizontalOffset - stringDimensions.x, cursor.y);
                dl->AddText(connectorNamePos, fontColor, str, strEnd);
            } else {
                const std::string string = LOC_SYS(o.getLocalizationHandle());
                const char* str = string.c_str();
                const char* strEnd = str + string.length();
                
                const ImVec2 stringDimensions = ImGui::CalcTextSize(str, strEnd);
                
                const ImVec2 connectorNamePos(outputConnectorNameHorizontalOffset - stringDimensions.x, cursor.y);
                dl->AddText(connectorNamePos, fontColor, str, strEnd);
            }
            
            connectorDataCache[ConnectorKey(node.getKey(), i, false)] = {connectorPos, color};
            cursor.y += lineWithSpacing;
        }
        
        // Inputs
        const float inputConnectorHorizontalOffset = cursor.x;
        const float inputConnectorNameHorizontalOffset = cursor.x + windowPadding.x + connectorRadius;
        for (std::uint8_t j = 0; j < inputs.size(); ++j) {
            const auto& i = inputs[j];
            const ImVec2 connectorPos(inputConnectorHorizontalOffset, cursor.y + lineWithoutSpacing * 0.5f);
            bool isHovered = false;
            
            if (ImGui::IsMouseHoveringRect(ImVec2(connectorPos.x - connectorRadius, connectorPos.y - connectorRadius),
                                           ImVec2(connectorPos.x + connectorRadius, connectorPos.y + connectorRadius))) {
                hoveredNodeKey = node.getKey();
            
                if (i.isEnabled()) {
                    hoveredConnector = ConnectorKey(node.getKey(), i.getID(), true);
                }
                
                isHovered = true;
            }
            
            ImU32 color = graph->getConnectorTypeColor(i.getType(), i.isEnabled());
            dl->AddCircleFilled(connectorPos, connectorRadius, color, 16);
            dl->AddCircle(connectorPos, connectorRadius, ((isHovered && i.isEnabled()) ? hoveredConnectorColor : connectorColor), 16, 2.0f * scale);
            
            const ImVec2 connectorNamePos(inputConnectorNameHorizontalOffset, cursor.y);
            if (i.hasName()) {
                const char* str = i.getName().c_str();
                
                dl->AddText(connectorNamePos, fontColor, str, str + i.getName().length());
            } else {
                const std::string string = LOC_SYS(i.getLocalizationHandle());
                const char* str = string.c_str();
                
                dl->AddText(connectorNamePos, fontColor, str, str + string.length());
            }
            
            connectorDataCache[ConnectorKey(node.getKey(), j, true)] = {connectorPos, color};
            cursor.y += lineWithSpacing;
        }
        
//         LOG_D(framePadding.y << " " << style.WindowPadding.y);
//         LOG_D(lineWithSpacing << " " << lineWithoutSpacing << " " << (lineWithoutSpacing - lineWithSpacing) << " " << style.ItemSpacing.x << " " << style.ItemSpacing.y);
//         ImGui::Text("%f, %f, %f", ImGui::GetTextLineHeight(), ImGui::GetFontSize(), ImGui::GetTextLineHeightWithSpacing());
        //n.second->getNameHash();
        ImGui::PopID();
    }
}

template <typename T>
void LogicGraphEditor<T>::drawConnections() {
    const auto& nodes = graph->getNodeConnections();
    const ImU32 color = ImColor(ImGui::GetStyle().Colors[ImGuiCol_PlotLines]);
    
    for (const auto& node : nodes) {
        for (const auto& connection : node.second) {
            auto source = connectorDataCache.find(ConnectorKey(node.first, connection.second.first, false));
            auto destination = connectorDataCache.find(ConnectorKey(connection.first, connection.second.second, true));
            
            assert(source != connectorDataCache.end());
            assert(destination != connectorDataCache.end());
            
            drawConnectionCurve(source->second.first, destination->second.first, color);
        }
    }
}

template <typename T>
void LogicGraphEditor<T>::drawBackgroundLines() {
    IYFT_PROFILE(drawBackgroundLines, iyft::ProfilerTag::LogicGraph);
    
    const ImVec4 baseLineColor = ImGui::GetStyle().Colors[ImGuiCol_Border];
    const ImU32 color = ImColor(baseLineColor.x, baseLineColor.y, baseLineColor.z, baseLineColor.w);
    const ImU32 originColor = ImColor(baseLineColor.x, baseLineColor.y, baseLineColor.z, 1.0f);
    
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 clipMin = dl->GetClipRectMin();
    const ImVec2 clipMax = dl->GetClipRectMax();
    const ImVec2 clipDimensions(clipMax.x - clipMin.x, clipMax.y - clipMin.y);
    
    // This computes the difference between the screen space and the canvas space
    const ImVec2 scaledPosition(canvasPosition.x * scale, canvasPosition.y * scale);
    const ImVec2 offset(clipMin.x - scaledPosition.x, clipMin.y - scaledPosition.y);
    
    const float verticalLineSpacing = settings.lineDensity.x * scale;
    const std::int64_t firstVerticalLine = scaledPosition.x / verticalLineSpacing;
    const std::int64_t lastVerticalLine = (scaledPosition.x + clipDimensions.x) / verticalLineSpacing;
    
    for (std::int64_t i = firstVerticalLine; i <= lastVerticalLine; ++i) {
        const ImVec2 start(offset.x + i * verticalLineSpacing, clipMin.y);
        const ImVec2 end(offset.x + i * verticalLineSpacing, clipMax.y);
        
        dl->AddLine(start, end, color);
    }
    
    const float horizontalLineSpacing = settings.lineDensity.y * scale;
    const std::int64_t firstHorizontalLine = scaledPosition.y / horizontalLineSpacing;
    const std::int64_t lastHorizontalLine = (scaledPosition.y + clipDimensions.y) / horizontalLineSpacing;
    
    for (std::int64_t i = firstHorizontalLine; i <= lastHorizontalLine; ++i) {
        const ImVec2 start(clipMin.x, offset.y + i * horizontalLineSpacing);
        const ImVec2 end(clipMax.x, offset.y + i * horizontalLineSpacing);
        
        dl->AddLine(start, end, color);
    }
    
    dl->AddCircleFilled(offset, 6.0f, originColor);
    
//     LOG_D(firstVerticalLine << " " << lastVerticalLine << " " << (lastVerticalLine - firstVerticalLine) << "\t\n" <<
//           firstHorizontalLine << " " << lastHorizontalLine << " " << (firstHorizontalLine - lastHorizontalLine));
}


template <typename T>
void LogicGraphEditor<T>::drawConnectionCurve(const ImVec2& start, const ImVec2& end, ImU32 color) {
    const ImVec2 delta = ImVec2(end.x - start.x, end.y - start.y);
    
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddBezierCurve(end,
                       ImVec2(end.x - delta.x * 0.8f, end.y - delta.y * 0.3f),
                       ImVec2(end.x + delta.x - delta.x * 1.3f, end.y - delta.y + delta.y * 0.3f),
                       start,
                       color, scale * settings.lineThickness);
}

template <typename T>
float LogicGraphEditor<T>::computeConnectorSlotRadius() {
    const float lineWithoutSpacing = ImGui::GetTextLineHeight();
    const float itemSpacing = ImGui::GetStyle().ItemSpacing.y * scale;
    const float lineWithSpacing = lineWithoutSpacing + itemSpacing;
    return lineWithSpacing * 0.3f;
}
}

#endif // IYF_LOGIC_GRAPH_EDITOR_HPP
