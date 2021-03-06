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

#ifndef IYF_LOGIC_GRAPH_HPP
#define IYF_LOGIC_GRAPH_HPP

#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <cassert>
#include <type_traits>
#include <sstream>

#include "localization/LocalizationHandle.hpp"
#include "logging/Logger.hpp"
#include "threading/ThreadProfiler.hpp"
#include "io/interfaces/TextSerializable.hpp"

#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"


namespace iyf {

/// Integer type used for connector IDs 
using LogicGraphConnectorID = std::uint8_t;
using ZIndex = std::uint32_t;

/// A small Vec2 helper. Not using a glm::vec2 or ImVec2 here because I may need to reuse this header elsewhere.
struct Vec2 {
    inline constexpr Vec2() : x(0.0f), y(0.0f) {}
    inline constexpr Vec2(float x, float y) : x(x), y(y) {}
    
    float x;
    float y;
};

/// Used inside LogicGraphNode objects. They may have two types of connectors - inputs and outputs.
template <typename NodeConnectorType>
class LogicGraphConnector {
public:
    static_assert(std::is_enum_v<NodeConnectorType>, "NodeConnectorType must be an enumerator");
    static_assert(std::is_same_v<std::underlying_type_t<NodeConnectorType>, std::uint8_t>, "The underlying type of the NodeConnectorType must be std::uint8_t");
    
    static constexpr std::uint8_t InvalidID = 255;
    
    using ConnectorType = NodeConnectorType;
    
    /// \brief Creates a LogicGraphConnector with a string name.
    ///
    /// Typically used when a node is generated from custom data that was specified by the end user.
    ///
    /// \param id The ID MUST match the position of this connector in the input or output vector and has to be less than InvalidID
    inline LogicGraphConnector(std::string name, LogicGraphConnectorID id, bool required, bool enabled, NodeConnectorType type) 
        : name(std::move(name)), locHandle(StringHash(0)), id(id), required(required), enabled(enabled), type(type) {
        checkID();
    }
    
    /// \brief Creates a LogicGraphConnector that can fetch a localized name.
    ///
    /// Typically used when creating nodes defined in code (e.g., for materials).
    ///
    /// \param id The ID MUST match the position of this connector in the input or output vector
    inline LogicGraphConnector(LocalizationHandle locHandle, LogicGraphConnectorID id, bool required, bool enabled, NodeConnectorType type)
        : locHandle(locHandle), id(id), required(required), enabled(enabled), type(type) {
        checkID();
    }
    
    /// \brief Return the localization handle that can be used to fetch a localized name for this connector
    ///
    /// \throw std::logic_error if this hasName() is true.
    inline LocalizationHandle getLocalizationHandle() const {
        if (hasName()) {
            throw std::logic_error("This is a named connector and it doesn't have a valid LocalizationHandle");
        }
        
        return locHandle;
    }
    
    inline void setLocalizationHandle(LocalizationHandle locHandle) {
        if (hasName()) {
            name.clear();
        }
        
        this->locHandle = locHandle;
    }
    
    /// \brief Check if this connector has an assigned name.
    ///
    /// If it doesn't one should be fetched from the localization system using getLocalizationHandle().
    inline bool hasName() const {
        return !name.empty();
    }
    
    /// \brief Return the name of this connector.
    ///
    /// \throw std::logic_error if this hasName() is false.
    inline const std::string& getName() const {
        if (!hasName()) {
            throw std::logic_error("This is an unamed connector and a LocalizationHandle should be used instead");
        }
        
        return name;
    }
    
    inline void setName(std::string name) {
        this->name = std::move(name);
    }
    
    /// \brief Return a numeric identifier. Usually the position of the connector in the node input or output vectors.
    inline LogicGraphConnectorID getID() const {
        return id;
    }
    
    /// \brief Should a warning or error be shown if this input is unused.
    inline bool isRequired() const {
        return required;
    }
    
    inline void setRequired(bool required) {
        this->required = required;
    }
    
    /// \brief Return the type of the connector.
    inline NodeConnectorType getType() const {
        return type;
    }
    
    inline void setType(NodeConnectorType type) {
        this->type = type;
    }
    
    inline bool isEnabled() const {
        return enabled;
    }
    
    inline void setEnabled(bool enabled) {
        this->enabled = enabled;
    }
private:
    inline void checkID() {
        if (id == InvalidID) {
            throw std::logic_error("The ID cannot be equal to InvalidID.");
        }
    }
    
    std::string name;
    LocalizationHandle locHandle;
    
    LogicGraphConnectorID id;
    bool required;
    bool enabled;
    NodeConnectorType type;
};

struct ModeInfo {
    ModeInfo(LocalizationHandle name, LocalizationHandle documentation) : name(name), documentation(documentation) {}
    
    const LocalizationHandle name;
    const LocalizationHandle documentation;
};

/// A node of the LogicGraph.
template <typename NodeTypeEnum, typename NodeConnector, typename NodeKey = std::uint32_t>
class LogicGraphNode : public TextSerializable {
public:
    static_assert(std::is_enum_v<NodeTypeEnum>, "NodeTypeEnum must be an enumerator");
    static_assert(std::is_integral_v<NodeKey>, "NodeKey must be an integral type");
    static_assert(std::is_unsigned_v<NodeKey>, "NodeKey must be an unsigned type");
    
    using TypeEnum = NodeTypeEnum;
    using Key = NodeKey;
    using Connector = NodeConnector;
    using NodeConnectorType = typename Connector::ConnectorType;
    
    LogicGraphNode(Key key, Vec2 position, ZIndex zIndex, std::size_t selectedMode = 0) : key(key), position(std::move(position)), zIndex(zIndex), selectedMode(selectedMode) {}
    
    virtual ~LogicGraphNode() {}
    
    virtual TypeEnum getType() const = 0;
    
    /// \warning When overriding this, make sure to override getSupportedModes() as well.
    virtual bool supportsMultipleModes() const {
        return false;
    }
    
    /// \warning When overriding this, make sure to override supportsMultipleModes() as well.
    virtual const std::vector<ModeInfo>& getSupportedModes() const {
        throw std::logic_error(MultipleModesNotSupportedError);
        
        static std::vector<ModeInfo> modes;
        return modes;
    }
    
    inline std::size_t getSelectedModeID() const {
        if (!supportsMultipleModes()) {
            throw std::logic_error(MultipleModesNotSupportedError);
        }
        
        return selectedMode;
    }
    
    inline bool setSelectedModeID(std::size_t selectedMode, bool isDeserializing = false) {
        if (!supportsMultipleModes()) {
            throw std::logic_error(MultipleModesNotSupportedError);
        }
        
        const bool changeAccepted = onModeChange(this->selectedMode, selectedMode, isDeserializing);
        if (changeAccepted) {
            this->selectedMode = selectedMode;
        }
        
        return changeAccepted;
    }
    
    inline Key getKey() const {
        return key;
    }
    
    inline const std::vector<Connector>& getInputs() const {
        return inputs;
    }
    
    inline const std::vector<Connector>& getOutputs() const {
        return outputs;
    }
    
    inline const Vec2& getPosition() const {
        return position;
    }
    
    inline void setPosition(Vec2 position) const {
        this->position = std::move(position);
    }
    
    inline void translate(const Vec2& position) {
        this->position.x += position.x;
        this->position.y += position.y;
    }
    
    /// \brief Return a name that was assigned to this node.
    inline const std::string& getName() const {
        return name;
    }
    
    /// \brief Assign a name to this node.
    inline void setName(std::string name) {
        this->name = std::move(name);
    }
    
    /// \bried Check if the user has assigned a name to this node.
    ///
    /// If true, getName() should be used instead of fetching the name based on the LocalizationHandle
    inline bool hasName() const {
        return !name.empty();
    }
    
    inline ZIndex getZIndex() const {
        return zIndex;
    }
    
    inline void setZIndex(ZIndex zIndex) {
        this->zIndex = zIndex;
    }
    
    inline std::uint32_t incrementZIndex() {
        return ++zIndex;
    }
    
    /// The version number used to determine appropriate serialization logic. It should be increased every time the 
    /// internal logic of the node changes (e.g., new modes are added or removed).
    ///
    /// \warning If you override this, it's very likely you'll need to override serializeJSON() and deserializeJSON() to
    /// handle the changed logic.
    ///
    /// \returns The version of the node.
    virtual std::uint32_t getVersion() const {
        return 1;
    }
    
    virtual bool makesJSONRoot() const final override {
        return false;
    }
    
    virtual void serializeJSON(PrettyStringWriter& pw) const override {
        pw.Key(VERSION_FIELD_NAME);
        pw.Uint(getVersion());
        
        pw.Key(NODE_KEY_FIELD_NAME);
        pw.Uint64(key);
        
        pw.Key(NODE_TYPE_FIELD_NAME);
        pw.Uint64(static_cast<std::uint64_t>(getType()));
        
        pw.Key(POSITION_FIELD_NAME);
        pw.StartObject();
        
        pw.Key(POSITION_X_FIELD_NAME);
        pw.Double(position.x);
        
        pw.Key(POSITION_Y_FIELD_NAME);
        pw.Double(position.y);
        
        pw.EndObject();
        
        if (hasName()) {
            pw.Key(NAME_FIELD_NAME);
            pw.String(name);
        }
        
        if (supportsMultipleModes()) {
            pw.Key(MODE_FIELD_NAME);
            pw.Uint64(selectedMode);
        }
    }
    
    virtual void deserializeJSON(JSONObject& jo) override {
        key = jo[NODE_KEY_FIELD_NAME].GetUint64();
        position = Vec2(jo[POSITION_FIELD_NAME][POSITION_X_FIELD_NAME].GetDouble(),
                        jo[POSITION_FIELD_NAME][POSITION_Y_FIELD_NAME].GetDouble());
        
        assert(jo[NODE_TYPE_FIELD_NAME].GetUint64() == static_cast<std::uint64_t>(getType()));
        
        if (jo.HasMember(NAME_FIELD_NAME)) {
            name = jo[NAME_FIELD_NAME].GetString();
        }
        
        if (jo.HasMember(MODE_FIELD_NAME)) {
            setSelectedModeID(jo[MODE_FIELD_NAME].GetUint64(), true);
        }
    }
    
    static constexpr const char* NODE_TYPE_FIELD_NAME = "type";
    static constexpr const char* NODE_KEY_FIELD_NAME = "key";
    static constexpr const char* NODE_CONNECTOR_FIELD_NAME = "connector";
protected:
    static constexpr const char* VERSION_FIELD_NAME = "version";
    static constexpr const char* NAME_FIELD_NAME = "name";
    static constexpr const char* MODE_FIELD_NAME = "mode";
    static constexpr const char* POSITION_FIELD_NAME = "position";
    static constexpr const char* POSITION_X_FIELD_NAME = "x";
    static constexpr const char* POSITION_Y_FIELD_NAME = "y";
    
    /// Called by setSelectedModeID(). You should override this function and implement all mode change logic in it. For example,
    /// you should use this function to check if the requestedModeID is valid. If changing the mode changes the type of inputs (or
    /// outputs), enables or disables them, all such changes need to be performed in this function, using the appropriate setters.
    ///
    /// \remark setSelectedModeID() returns the result of this function. If it's true, all connections will be automatically revalidated
    /// and connections that are no longer valid will be removed.
    ///
    /// \warning NEVER add or remove inputs - only enable or disable them.
    /// 
    /// \return true if change was applied, false otherwise.
    virtual bool onModeChange([[maybe_unused]] std::size_t currentModeID, [[maybe_unused]] std::size_t requestedModeID, [[maybe_unused]] bool isDeserializing) {
        return true;
    }
    
    template <typename ConnectorIdentifier>
    inline void addInput(ConnectorIdentifier identifier, NodeConnectorType type, bool enabled = true, bool required = false) {
        addConnector(true, identifier, required, enabled, type);
    }
    
    template <typename ConnectorIdentifier>
    inline void addOutput(ConnectorIdentifier identifier, NodeConnectorType type, bool enabled = true, bool required = false) {
        addConnector(false, identifier, required, enabled, type);
    }
    
    template <typename ConnectorIdentifier>
    inline void addConnector(bool isInput, ConnectorIdentifier identifier, bool required, bool enabled, NodeConnectorType type) {
        std::vector<Connector>& destination = isInput ? inputs : outputs;
        std::size_t nextID = destination.size();
        
        if (nextID >= Connector::InvalidID) {
            throw std::logic_error("The number of inputs and outputs must be less than Connector::InvalidID.");
        }
        
        if (type >= NodeConnectorType::COUNT) {
            throw std::logic_error("The type of the connector must be less than NodeConnectorType::COUNT.");
        }
        
        destination.emplace_back(identifier, nextID, required, enabled, type);
    }
    
    inline Connector& getInput(std::size_t id) {
        return inputs[id];
    }
    
    inline Connector& getOutput(std::size_t id) {
        return outputs[id];
    }
protected:
    /// \warning Always favor the multiple mode approach and only use this function in EXTREMELY SPECIAL cases. Moreover, make sure to call
    /// LogicGraph::disconnectNode() before calling this and add new connectors immediately afterwards.
    inline void removeAllConnectors() {
        inputs.clear();
        outputs.clear();
    }
private:
    static constexpr const char* MultipleModesNotSupportedError = "This node doesn't support multiple modes.";
    
    Key key;
    Vec2 position;
    ZIndex zIndex;
    std::string name;
    std::size_t selectedMode;
    std::vector<Connector> inputs;
    std::vector<Connector> outputs;
};

template <typename LogicGraphNode, typename GraphNodeGroup>
struct LogicGraphNodeTypeInfo {
    using NodeType = typename LogicGraphNode::TypeEnum;
    using NodeGroup = GraphNodeGroup;
    
    LogicGraphNodeTypeInfo() : name(StringHash()), documentation(StringHash()), instantiable(false), deletable(false), valid(false) {}
    
    LogicGraphNodeTypeInfo(NodeType type, LocalizationHandle name, LocalizationHandle documentation, NodeGroup group, bool instantiable, bool deletable)
     : name(name), documentation(documentation), type(type), group(group), instantiable(instantiable), deletable(deletable), valid(true) {}
    
    /// A LocalizationHandle that can be used to retrieve a localized name for this node type.
    LocalizationHandle name;
    
    /// A LocalizationHandle that can be used to retrieve a localized documentation string for this node type.
    LocalizationHandle documentation;
    
    NodeType type;
    NodeGroup group;
    
    /// If true, nodes of this type may be instantiated by calling the LogicGraph::addNode() function.
    bool instantiable;
    
    /// If true, nodes of this type may be deleted by calling the LogicGraph::removeNode() function.
    bool deletable;
    
    bool valid;
};

/*class NodeConnectionResult {
public:
    //
private:
    //
};*/
enum class NodeConnectionResult {
    Success,
    TypeMismatch,
    InvalidSource,
    InvalidSourceOutput,
    NullSource,
    InvalidDestination,
    InvalidDestinationInput,
    DestinationIsDisabled,
    SourceIsDisabled,
    NullDestination,
    OccupiedDestination,
    InsertionFailed,
    UnableToConnectToSelf,
};

enum class LogicGraphValidationResult {
    Success,
    CycleFound,
    RequiredInputsNotConnected,
    Empty,
};

template <typename GraphNodeType, typename GraphNodeTypeInfo>
class LogicGraph : public TextSerializable {
public:
    using NodeType = GraphNodeType;
    using NodeTypeEnum = typename NodeType::TypeEnum;
    using NodeKey = typename NodeType::Key;
    using NodeTypeInfo = GraphNodeTypeInfo;
    using NodeTypeInfoEnum = typename NodeTypeInfo::NodeGroup;
    using ConnectorType = typename NodeType::Connector::ConnectorType;
    
    using ConnectorIDPair = std::pair<LogicGraphConnectorID, LogicGraphConnectorID>;
    using DestinationMultiMap = std::unordered_multimap<NodeKey, ConnectorIDPair>;
    
    static constexpr NodeKey InvalidKey = std::numeric_limits<NodeKey>::max();
    static constexpr std::uint32_t DataVersion = 1;
    
    class KeyConnectorPair {
    public:
        KeyConnectorPair() : key(InvalidKey), connector(NodeType::Connector::InvalidID) {}
        KeyConnectorPair(NodeKey key, LogicGraphConnectorID connector) : key(key), connector(connector) {}
        
        inline NodeKey getNodeKey() const {
            return key;
        }
        
        inline LogicGraphConnectorID getConnectorID() const {
            return connector;
        }
        
        inline friend bool operator<(const KeyConnectorPair& a, const KeyConnectorPair& b) {
            if (a.key < b.key) {
                return true;
            }
            
            if (b.key < a.key) {
                return false;
            }
            
            if (a.connector < b.connector) {
                return true;
            }
            
            if (b.connector < a.connector) {
                return false;
            }
            
            return false;
        }
        
        inline friend bool operator==(const KeyConnectorPair& a, const KeyConnectorPair& b) {
            return (a.key == b.key) && (a.connector == b.connector);
        }
        
        inline bool isValid() const {
            return (key != InvalidKey) && (connector != NodeType::Connector::InvalidID); 
        }
    private:
        NodeKey key;
        LogicGraphConnectorID connector;
    };
    
    struct KeyConnectorPairHash {
        std::size_t operator()(const KeyConnectorPair& kcp) const {
            // TODO use hashcombine
            return std::hash<NodeKey>()(kcp.getNodeKey()) ^ (std::hash<LogicGraphConnectorID>()(kcp.getConnectorID()) << 1);
        }
    };
    
    inline LogicGraph() : nextKey(0), nextZIndex(0), nodeTypeInfoSetupComplete(false), nodeGroupSetupComplete(false) {
        nodeTypeInfo.resize(static_cast<std::size_t>(NodeTypeEnum::COUNT));
        nodeGroupNames.resize(static_cast<std::size_t>(NodeTypeInfoEnum::COUNT), LocalizationHandle(StringHash(0)));
    }
    
    virtual ~LogicGraph() {
        clear();
    }
    
    inline const std::unordered_map<NodeKey, NodeType*>& getNodes() const {
        return nodes;
    }
    
    inline const std::unordered_map<NodeKey, DestinationMultiMap>& getNodeConnections() const {
        return connections;
    }
    
    inline std::size_t getNodeCount() const {
        return nodes.size();
    }
    
    virtual std::string getConnectorTypeName(ConnectorType type) const = 0;
    virtual std::uint32_t getConnectorTypeColor(ConnectorType type, bool enabled) const = 0;
    
    /// This function creates a new node and inserts into the node graph.
    NodeType* addNode(NodeTypeEnum type, const Vec2& position) {
        return addNodeImpl(getNextKey(), type, position, false);
    }
    
    inline const NodeTypeInfo& getNodeTypeInfo(NodeTypeEnum type) const {
        assert(nodeTypeInfoSetupComplete && nodeGroupSetupComplete);
        
        return nodeTypeInfo[static_cast<std::size_t>(type)];
    }
    
    LocalizationHandle getNodeGroupNameHandle(NodeTypeInfoEnum group) const {
        assert(nodeTypeInfoSetupComplete && nodeGroupSetupComplete);
        
        return nodeGroupNames[static_cast<std::size_t>(group)];
    }
    
    inline const std::vector<NodeTypeInfo>& getNodeGroupTypeInfos(NodeTypeInfoEnum group) const {
        assert(nodeTypeInfoSetupComplete && nodeGroupSetupComplete);
        
        return groupedNodeTypeInfo[static_cast<std::size_t>(group)];
    }
    
    inline std::underlying_type_t<NodeTypeEnum> getNodeTypeCount() const {
        return static_cast<std::underlying_type_t<NodeTypeEnum>>(NodeTypeEnum::COUNT);
    }
    
    inline std::underlying_type_t<NodeTypeInfoEnum> getNodeGroupCount() const {
        return static_cast<std::underlying_type_t<NodeTypeInfoEnum>>(NodeTypeInfoEnum::COUNT);
    }
    
    virtual bool removeNode(NodeKey key) {
        NodeType* node = getNode(key);
        if (!getNodeTypeInfo(node->getType()).deletable) {
            return false;
        }
        
        const bool disconnectionResult = disconnectNode(key);
        if (!disconnectionResult) {
            return false;
        }
        
        auto nodeResult = nodes.find(key);
        assert(nodeResult != nodes.end());
        
        delete nodeResult->second;
        nodes.erase(nodeResult);
        
        return true;
    }
    
    virtual bool disconnectNode(NodeKey key) {
        NodeType* node = getNode(key);
        
        if (node == nullptr) {
            return false;
        }
        
        removeNodeInputs(node, false);
        removeNodeOutputs(node, false);
        
        auto destinations = connections.find(key);
        assert(destinations != connections.end());
        assert(destinations->second.empty());
        
        connections.erase(destinations);
        
        return true;
    }
    
    inline NodeType* getNode(NodeKey key) const {
        auto result = nodes.find(key);
        if (result == nodes.end()) {
            return nullptr;
        }
        
        return result->second;
    }
    
    inline bool hasNode(NodeKey key) const {
        return (nodes.find(key) != nodes.end());
    }
    
    inline bool nodeToTop(NodeKey key) {
        NodeType* node = getNode(key);
        
        if (node == nullptr) {
            return false;
        }
        
        node->setZIndex(nextZIndex);
        nextZIndex++;
        
        return true;
    }
    
    /// \remark This vector is built like a stack. That is, you probably want to use a reverse iterator to access
    /// the contents or reverse it before use.
    ///
    /// \remark Based on https://www.geeksforgeeks.org/topological-sorting/
    ///
    /// \returns A vector that contains topologically sorted nodes or an empty vector if validation failed.
    std::vector<NodeKey> getTopologicalSort() const {
        std::vector<NodeKey> sortedNodes;
        if (!validate(nullptr)) {
            return sortedNodes;
        }
        
        sortedNodes.reserve(nodes.size());
        
        std::unordered_set<NodeKey> visitedNodes;
        for (const auto& node : nodes) {
            if (!contains(visitedNodes, node.first)) {
                recursiveTopologicalSort(node.first, visitedNodes, sortedNodes);
            }
        }
        
        return sortedNodes;
    }
    
    /// \remark Based on https://www.geeksforgeeks.org/detect-cycle-in-a-graph/
    bool hasCycles() const {
        std::unordered_set<NodeKey> visitedNodes;
        std::unordered_set<NodeKey> currentNodes;
        
        for (const auto& node : nodes) {
            if (recursiveCycleFinder(node.first, visitedNodes, currentNodes)) {
                return true;
            }
        }
        
        return false;
    }
    
    bool requiredInputsConnected(std::stringstream* ss) const {
        const bool log = (ss != nullptr);
        
        bool result = true;
        
        for (const auto& node : nodes) {
            const auto& inputs = node.second->getInputs();
            
            for (std::size_t i = 0; i < inputs.size(); ++i) {
                const auto& input = inputs[i];
                if (input.isRequired() && input.isEnabled() && !(getInputSource(node.first, i).isValid())) {
                    if (log) {
                        (*ss) << "ERROR: Node " << node.first << " connector " << i << " needs an input\n";
                    }
                    result = false;
                }
            }
        }
        
        return result;
    }
    
    inline KeyConnectorPair getInputSource(NodeKey key, LogicGraphConnectorID id) const {
        const KeyConnectorPair kcp(key, id);
        
        auto result = busyInputs.find(kcp);
        
        if (result == busyInputs.end()) {
            return KeyConnectorPair();
        } else {
            return result->second;
        }
    }
    
    /// \brief Validates this node graph and writes all errors to an std::stringstream if one is provided.
    ///
    /// \param ss Optional std::stringstream that will receive all errors.
    virtual bool validate(std::stringstream* ss) const {
        const bool log = (ss != nullptr);
        
        if (nodes.empty()) {
            if (log) {
                (*ss) << "ERROR: The node graph is empty.\n";
            }
            
            return false;
        }
        
        if (hasCycles()) {
            if (log) {
                (*ss) << "ERROR: The node graph has at least one cycle.\n";
            }
            
            return false;
        }
        
        if (!requiredInputsConnected(ss)) {
            return false;
        }
        
        return true;
    }
    
    /// \brief Checks if two nodes can be connected.
    ///
    /// \return An enum value that informs the user if a connection can be established and the reason why if it cannot.
    NodeConnectionResult validateConnection(const NodeType* source, LogicGraphConnectorID outputID, const NodeType* destination, LogicGraphConnectorID inputID) const {
        // Catch null pointers
        if (source == nullptr) {
            return NodeConnectionResult::NullSource;
        }
        
        if (destination == nullptr) {
            return NodeConnectionResult::NullDestination;
        }
        
        // Source and destination must be separate nodes
        if (source == destination) {
            return NodeConnectionResult::UnableToConnectToSelf;
        }
        
        // Check if the provided nodes live in this graph
        auto sourceNodeInMap = nodes.find(source->getKey());
        if (sourceNodeInMap == nodes.end() || (*sourceNodeInMap).second != source) {
            return NodeConnectionResult::InvalidSource;
        }
        
        auto destinationNodeInMap = nodes.find(destination->getKey());
        if (destinationNodeInMap == nodes.end() || (*destinationNodeInMap).second != destination) {
            return NodeConnectionResult::InvalidDestination;
        }
        
        // Check if the user wants to connect to existing connectors
        const auto& outputs = source->getOutputs();
        if (outputID >= outputs.size()) {
            return NodeConnectionResult::InvalidSourceOutput;
        }
        
        const auto& inputs = destination->getInputs();
        if (inputID >= inputs.size()) {
            return NodeConnectionResult::InvalidDestinationInput;
        }
        
        // Check if connector types match
        if (outputs[outputID].getType() != inputs[inputID].getType()) {
            return NodeConnectionResult::TypeMismatch;
        }
        
        // Check if connectors are enabled
        if (!inputs[inputID].isEnabled()) {
            return NodeConnectionResult::DestinationIsDisabled;
        }
        
        if (!outputs[outputID].isEnabled()) {
            return NodeConnectionResult::SourceIsDisabled;
        }
        
        // Check if the destination is already in use
        const KeyConnectorPair sourceKeyConnection(source->getKey(), outputID);
        const KeyConnectorPair destinationKeyConnection(destination->getKey(), inputID);
        
        auto busyCheckResult = busyInputs.find(destinationKeyConnection);
        if (busyCheckResult != busyInputs.end()) {
            return NodeConnectionResult::OccupiedDestination;
        }
        
        return NodeConnectionResult::Success;
    }
    
    /// Validates all node connections and removes those that are no longer possible. This may happen after the mode of the node changes.
    void revalidateNodeConnections(const NodeType* node) {
        if (node == nullptr) {
            return;
        }
        
        auto sourceNodeInMap = nodes.find(node->getKey());
        if (sourceNodeInMap == nodes.end() || (*sourceNodeInMap).second != node) {
            return;
        }
        
        removeNodeInputs(node, true);
        removeNodeOutputs(node, true);
    }
    
    NodeConnectionResult addConnection(const NodeType* source, LogicGraphConnectorID outputID, const NodeType* destination, LogicGraphConnectorID inputID) {
        NodeConnectionResult result = validateConnection(source, outputID, destination, inputID);
        
        if (result != NodeConnectionResult::Success) {
            return result;
        }
        
        DestinationMultiMap& destinations = connections[source->getKey()];
        
        const ConnectorIDPair idPair(outputID, inputID);
        const KeyConnectorPair sourceKeyConnection(source->getKey(), outputID);
        const KeyConnectorPair destinationKeyConnection(destination->getKey(), inputID);
        
        // TODO this check should probably be debug only
        const auto connectionsToDestination = destinations.equal_range(destination->getKey());
        for (auto i = connectionsToDestination.first; i != connectionsToDestination.second; ++i) {
            if (i->second == idPair) {
                throw std::runtime_error("invalid multimap state");
            }
        }
        
        destinations.insert({destination->getKey(), idPair});
        busyInputs[destinationKeyConnection] = sourceKeyConnection;
        
        return NodeConnectionResult::Success;
    }
    
    bool removeConnection(const NodeType* source, LogicGraphConnectorID outputID, const NodeType* destination, LogicGraphConnectorID inputID, bool skipValidation = false) {
        if (!skipValidation) {
            NodeConnectionResult result = validateConnection(source, outputID, destination, inputID);
            
            if (result != NodeConnectionResult::OccupiedDestination) {
                return false;
            }
        }
        
        DestinationMultiMap& destinations = connections[source->getKey()];
        
        const ConnectorIDPair idPair(outputID, inputID);
        const KeyConnectorPair sourceKeyConnection(source->getKey(), outputID);
        const KeyConnectorPair destinationKeyConnection(destination->getKey(), inputID);
        
        auto destIterator = destinations.equal_range(destination->getKey());
        assert(destIterator.first != destinations.end());
        
        bool foundConnection = false;
        for (auto it = destIterator.first; it != destIterator.second; ++it) {
            if (it->second == idPair) {
                destinations.erase(it);
                foundConnection = true;
                break;
            }
        }
        assert(foundConnection);
        
        auto busyInput = busyInputs.find(destinationKeyConnection);
        assert(busyInput != busyInputs.end());
        
        busyInputs.erase(destinationKeyConnection);
        
        return true;
    }
    
    inline KeyConnectorPair getSource(KeyConnectorPair input) const {
        auto source = busyInputs.find(input);
        if (source != busyInputs.end()) {
            return source->second;
        }
        
        return KeyConnectorPair();
    }
    
    void clear() {
        nextKey = 0;
        nextZIndex = 0;
        
        for (auto& n : nodes) {
            delete n.second;
        }
        nodes.clear();
        busyInputs.clear();
        connections.clear();
    }
    
    virtual bool makesJSONRoot() const final override {
        return false;
    }
    
    virtual void serializeJSON(PrettyStringWriter& pw) const override {
        pw.Key(VERSION_FIELD_NAME);
        pw.Uint(1);
        
        pw.Key(NEXT_KEY_FIELD_NAME);
        pw.Uint64(nextKey);
        
        // Sort nodes. A consistent order is important for version control
        std::vector<NodeKey> sortedNodes;
        sortedNodes.reserve(nodes.size());
        for (const auto& n : nodes) {
            sortedNodes.emplace_back(n.first);
        }
        
        std::sort(sortedNodes.begin(), sortedNodes.end());
        
        pw.Key(NODE_ARRAY_FIELD_NAME);
        pw.StartArray();
        for (const NodeKey k : sortedNodes) {
            const NodeType* n = getNode(k);
            assert(n != nullptr);
            
            pw.StartObject();
            n->serializeJSON(pw);
            pw.EndObject();
        }
        pw.EndArray();
        
        // Sort connections
        std::vector<KeyConnectorPair> sortedInputs;
        sortedInputs.reserve(busyInputs.size());
        for (const auto& c : busyInputs) {
            sortedInputs.emplace_back(c.first);
        }
        
        std::sort(sortedInputs.begin(), sortedInputs.end());
        
        pw.Key(CONNECTION_ARRAY_FIELD_NAME);
        pw.StartArray();
        for (const KeyConnectorPair& i : sortedInputs) {
            const auto src = busyInputs.find(i);
            
            pw.StartObject();
            pw.Key(SOURCE_FIELD_NAME);
            serializeKeyConnectorPair(pw, src->second);
            
            pw.Key(DESTINATION_FIELD_NAME);
            serializeKeyConnectorPair(pw, i);
            pw.EndObject();
        }
        pw.EndArray();
    }
    
    virtual void deserializeJSON(JSONObject& jo) override {
        clear();
        
        nextKey = static_cast<NodeKey>(jo[NEXT_KEY_FIELD_NAME].GetUint64());
        
        for (auto& n : jo[NODE_ARRAY_FIELD_NAME].GetArray()) {
            NodeTypeEnum type = static_cast<NodeTypeEnum>(n[NodeType::NODE_TYPE_FIELD_NAME].GetUint64());
            NodeType* node = addNodeImpl(static_cast<NodeKey>(n[NodeType::NODE_KEY_FIELD_NAME].GetUint64()), type, Vec2(0.0f, 0.0f), true);
            
            node->deserializeJSON(n);
        }
        
        for (auto& c : jo[CONNECTION_ARRAY_FIELD_NAME].GetArray()) {
            const KeyConnectorPair source = deserializeKeyConnectorPair(c[SOURCE_FIELD_NAME]);
            const KeyConnectorPair destination = deserializeKeyConnectorPair(c[DESTINATION_FIELD_NAME]);
            
            const NodeType* sourceNode = getNode(source.getNodeKey());
            const NodeType* destinationNode = getNode(destination.getNodeKey());
            
            const NodeConnectionResult result = addConnection(sourceNode, source.getConnectorID(), destinationNode, destination.getConnectorID());
            
            if (result != NodeConnectionResult::Success) {
                const char* resultName = "UnknownError";
                
                switch (result) {
                    case NodeConnectionResult::TypeMismatch:
                        resultName = "Type Mismatch";
                        break;
                    case NodeConnectionResult::InvalidSource:
                        resultName = "Invalid Source";
                        break;
                    case NodeConnectionResult::InvalidSourceOutput:
                        resultName = "Invalid Source Output";
                        break;
                    case NodeConnectionResult::NullSource:
                        resultName = "Null Source";
                        break;
                    case NodeConnectionResult::InvalidDestination:
                        resultName = "Invalid Destination";
                        break;
                    case NodeConnectionResult::InvalidDestinationInput:
                        resultName = "Invalid Destination Input";
                        break;
                    case NodeConnectionResult::DestinationIsDisabled:
                        resultName = "Destination is Disabled";
                        break;
                    case NodeConnectionResult::SourceIsDisabled:
                        resultName = "Source is Disabledh";
                        break;
                    case NodeConnectionResult::NullDestination:
                        resultName = "Null Destination";
                        break;
                    case NodeConnectionResult::OccupiedDestination:
                        resultName = "Occupied Destination";
                        break;
                    case NodeConnectionResult::InsertionFailed:
                        resultName = "Insertion Failed";
                        break;
                    case NodeConnectionResult::UnableToConnectToSelf:
                        resultName = "Can't Connect to Self";
                        break;
                    case NodeConnectionResult::Success:
                        throw std::runtime_error("Impossible NodeConnectionResult value");
                        break;
                }
                
                LOG_W("A deserialized logic graph connection is invalid."
                      "\n\tSource Key (connector ID): {} ({})"
                      "\n\tDestination Key (connector ID): {} ({})"
                      "\n\tResult: ", source.getNodeKey(), source.getConnectorID(), destination.getNodeKey(), destination.getConnectorID(), resultName);
            }
        }
        
        assert(nextKey == static_cast<NodeKey>(jo[NEXT_KEY_FIELD_NAME].GetUint64()));
    }
protected:
    static constexpr const char* VERSION_FIELD_NAME = "version";
    static constexpr const char* NEXT_KEY_FIELD_NAME = "nextKey";
    static constexpr const char* NODE_ARRAY_FIELD_NAME = "nodes";
    static constexpr const char* CONNECTION_ARRAY_FIELD_NAME = "connections";
    static constexpr const char* SOURCE_FIELD_NAME = "source";
    static constexpr const char* DESTINATION_FIELD_NAME = "destination";
    
    inline void serializeKeyConnectorPair(PrettyStringWriter& pw, const KeyConnectorPair& c) const {
        pw.StartObject();
        pw.Key(NodeType::NODE_KEY_FIELD_NAME);
        pw.Uint64(c.getNodeKey());
        
        pw.Key(NodeType::NODE_CONNECTOR_FIELD_NAME);
        pw.Uint64(c.getConnectorID());
        pw.EndObject();
    }
    
    inline KeyConnectorPair deserializeKeyConnectorPair(JSONObject& jo) const {
        return KeyConnectorPair(static_cast<NodeKey>(jo[NodeType::NODE_KEY_FIELD_NAME].GetUint64()),
                                static_cast<LogicGraphConnectorID>(jo[NodeType::NODE_CONNECTOR_FIELD_NAME].GetUint64()));
    }

    void removeNodeInputs(const NodeType* node, bool onlyInvalid) {
        const auto& inputs = node->getInputs();
        for (std::size_t i = 0; i < inputs.size(); ++i) {
            const KeyConnectorPair destinationKeyConnection(node->getKey(), i);
            
            auto input = busyInputs.find(destinationKeyConnection);
            if (input != busyInputs.end()) {
                NodeType* source = getNode(input->second.getNodeKey());
                assert(source != nullptr);
                
                auto connectorID = input->second.getConnectorID();
                
                if (onlyInvalid) {
                    NodeConnectionResult validationResult = validateConnection(source, connectorID, node, i);
                    if (validationResult != NodeConnectionResult::Success && validationResult != NodeConnectionResult::OccupiedDestination) {
                        const bool removed = removeConnection(source, connectorID, node, i, true);
                        assert(removed);
                    }
                } else {
                    const bool removed = removeConnection(source, connectorID, node, i, true);
                    assert(removed);
                }
            }
        }
    }
    
    void removeNodeOutputs(const NodeType* node, bool onlyInvalid) {
        DestinationMultiMap& destinations = connections[node->getKey()];
        for (auto destination = destinations.begin(); destination != destinations.end();) {
            auto outputConnectorID = destination->second.first;
            auto inputConnectorID = destination->second.second;
            NodeType* destinationNode = getNode(destination->first);
            
            assert(destinationNode != nullptr);
            
            // We may have to erase this element and that would invalidate the iterator, therefore, we need to advance it before use
            ++destination;
            
            if (onlyInvalid) {
                NodeConnectionResult validationResult = validateConnection(node, outputConnectorID, destinationNode, inputConnectorID);
                if (validationResult != NodeConnectionResult::Success && validationResult != NodeConnectionResult::OccupiedDestination) {
                    
                    const bool removed = removeConnection(node, outputConnectorID, destinationNode, inputConnectorID, true);
                    assert(removed);
                }
            } else {
                const bool removed = removeConnection(node, outputConnectorID, destinationNode, inputConnectorID, true);
                assert(removed);
            }
        }
    }
    
    /// This function is called by addNode() and should perform actual node creation.
    ///
    /// \remark When implementing, make sure to use newNode() or base your node creation logic implementation on it.
    /// newNode() contains asserts that will help you catch certain common node implementation errors and assing
    /// valid keys and zindexes. Once a node is created, you must call insertNode() to insert it into the graph.
    ///
    /// \warning if isDeserializing is true, you must still create the node, even if its LogicGraphNodeTypeInfo says
    /// that it's not instantiable.
    virtual NodeType* addNodeImpl(NodeKey key, NodeTypeEnum type, const Vec2& position, bool isDeserializing) = 0;
    
    /// This function inserts the node into the graph and increments the nextKey (unless you're deserializing).
    ///
    /// This function is supposed to be used inside addNode().
    inline bool insertNode(NodeType* node, NodeKey key, bool isDeserializing) {
        NodeKey finalKey;
        if (isDeserializing) {
            finalKey = key;
        } else {
            finalKey = getNextKey(true);
            assert(finalKey == key);
        }
        
        auto insertionResult = nodes.insert({finalKey, node});
        nextZIndex++;
        
        return insertionResult.second;
    }
    
    /// Get the next node key without incrementing it.
    /// 
    /// This function is supposed to be used inside addNode() or newNode().
    inline NodeKey getNextKey() const {
        return nextKey;
    }
    
    /// Get the next node key, optionally incrementing it.
    /// 
    /// This function is supposed to be used inside addNode() or newNode().
    inline NodeKey getNextKey(bool increment) {
        if (increment) {
            return nextKey++;
        } else {
            return nextKey;
        }
    }
    
    inline std::uint32_t getNextZIndex() const {
        return nextZIndex;
    }
    
    /// A helper method used to catch wrong type errors.
    template <typename T>
    inline NodeType* newNode(NodeKey key, const Vec2& position, NodeTypeEnum type, [[maybe_unused]] bool isDeserializing) {
        NodeType* node = new T(key, position, getNextZIndex());
        
#ifndef NDEBUG
        assert(node->getType() == type);
        assert(node->getInputs().size() > 0 || node->getOutputs().size() > 0);
        
        for (std::size_t i = 0; i < node->getInputs().size(); ++i) {
            assert(node->getInputs()[i].getID() == i);
        }
        
        for (std::size_t o = 0; o < node->getOutputs().size(); ++o) {
            assert(node->getOutputs()[o].getID() == o);
        }
#endif // NDEBUG
        
        return node;
    }
    
    void setNodeGroupName(NodeTypeInfoEnum group, LocalizationHandle lh) {
        nodeGroupNames[static_cast<std::size_t>(group)] = lh;
    }
    
    void addNodeTypeInfo(NodeTypeEnum type, const char* locNameKey, const char* locDocKey, const char* locNamespace, NodeTypeInfoEnum group, bool instantiable = true, bool deletable = true) {
        nodeTypeInfo[static_cast<std::size_t>(type)] = NodeTypeInfo(type, LH(locNameKey, locNamespace), LH(locDocKey, locNamespace), group, instantiable, deletable);
    }
    
    void validateNodeTypeInfo() {
        if (nodeTypeInfoSetupComplete || nodeGroupSetupComplete) {
            throw std::logic_error("This function must only run once");
        }
        
        groupedNodeTypeInfo.resize(static_cast<std::size_t>(NodeTypeInfoEnum::COUNT));
        for (std::size_t i = 0; i < nodeTypeInfo.size(); ++i) {
            const auto& ti = nodeTypeInfo[i];
            if (!ti.valid || (ti.type != static_cast<NodeTypeEnum>(i))) {
                std::string error = "NodeTypeInfo for ";
                error.append(std::to_string(i));
                error.append(" is invalid.");
                
                throw std::runtime_error(error);
            } else {
                groupedNodeTypeInfo[static_cast<std::size_t>(ti.group)].emplace_back(ti);
            }
        }
        
        for (std::size_t i = 0; i < nodeGroupNames.size(); ++i) {
            if (nodeGroupNames[i].getHashValue() == 0) {
                // This will fail horribly if it hashes to 0
                std::string error = "LocalizationHandle for node group ";
                error.append(std::to_string(i));
                error.append(" has not been set.");
                
                throw std::runtime_error(error);
            }
        }
        
        nodeTypeInfoSetupComplete = true;
        nodeGroupSetupComplete = true;
    }
private:
    inline static bool contains(std::unordered_set<NodeKey>& set, NodeKey key) {
        return set.count(key) == 1;
    }
    
    inline std::unordered_set<NodeKey> multimapToSet(const DestinationMultiMap& destinationMap) const {
        // In our case, a node may connect to an another node multiple times because nodes may have multiple inputs
        // this set is used to ensure all destinations only get processed once.
        std::unordered_set<NodeKey> destinationSet;
        destinationSet.reserve(destinationMap.size());
        
        for (const auto& destination : destinationMap) {
            destinationSet.insert(destination.first);
        }
        
        return destinationSet;
    }
    
    void recursiveTopologicalSort(NodeKey key, std::unordered_set<NodeKey>& visitedNodes, std::vector<NodeKey>& sortedNodes) const {
        visitedNodes.insert(key);
        
        auto destinationIter = connections.find(key);
        if (destinationIter != connections.end()) {
            const DestinationMultiMap& destinationMap = destinationIter->second;
        
            const auto destinationSet = multimapToSet(destinationMap);
            
            for (const auto& destination : destinationSet) {
                if (!contains(visitedNodes, destination)) {
                    recursiveTopologicalSort(destination, visitedNodes, sortedNodes);
                }
            }
        }
        
        sortedNodes.push_back(key);
    }
    
    bool recursiveCycleFinder(NodeKey key, std::unordered_set<NodeKey>& visitedNodes, std::unordered_set<NodeKey>& currentNodes) const {
        if (!contains(visitedNodes, key)) {
            visitedNodes.insert(key);
            currentNodes.insert(key);
            
            auto destinationIter = connections.find(key);
            if (destinationIter != connections.end()) {
                const DestinationMultiMap& destinationMap = destinationIter->second;
            
                const auto destinationSet = multimapToSet(destinationMap);
                
                for (const auto& destination : destinationSet) {
                    if (!contains(visitedNodes, destination) && recursiveCycleFinder(destination, visitedNodes, currentNodes)) {
                        return true;
                    } else if (contains(currentNodes, destination)) {
                        return true;
                    }
                }
            }
        }
        
        currentNodes.erase(key);
        return false;
    }
    
    NodeKey nextKey;
    ZIndex nextZIndex;
    
    std::unordered_map<NodeKey, NodeType*> nodes;
    
    /// Inputs can only have a single connection coming into them. This map is used to
    /// quickly check if the connector is occupied and to determine the source of data
    /// coming into it.
    std::unordered_map<KeyConnectorPair, KeyConnectorPair, KeyConnectorPairHash> busyInputs;
    
    /// All connections that exist inside the graph
    std::unordered_map<NodeKey, DestinationMultiMap> connections;
    
    std::vector<NodeTypeInfo> nodeTypeInfo;
    std::vector<std::vector<NodeTypeInfo>> groupedNodeTypeInfo;
    std::vector<LocalizationHandle> nodeGroupNames;
    bool nodeTypeInfoSetupComplete;
    bool nodeGroupSetupComplete;
};

}


#endif // IYF_LOGIC_GRAPH_HPP

