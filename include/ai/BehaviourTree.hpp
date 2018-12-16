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

#ifndef IYF_BEHAVIOUR_TREE_HPP
#define IYF_BEHAVIOUR_TREE_HPP

#include <cstdint>
#include <stdexcept>
#include <vector>
#include <cassert>
#include <functional>
#include <random>
#include <unordered_map>
#include <set>

#include "utilities/hashing/Hashing.hpp"
#include "utilities/NonCopyable.hpp"
#include "ai/BlackboardCallbackListener.hpp"
#include "ai/BlackboardValue.hpp"
#include "ai/BehaviourTreeConstants.hpp"

namespace iyf {

class Blackboard;
class BehaviourTree;
class ServiceNode;
class DecoratorNode;
class BehaviourTreeNode;

using BehaviourResultNextNodePair = std::pair<BehaviourTreeResult, BehaviourTreeNode*>;

class BehaviourTreeNode : private NonCopyable {
public:
    BehaviourTreeNode(BehaviourTree* tree, BehaviourTreeNodeType type) : tree(tree), parent(nullptr), priority(0), depth(0), lastChildResult(static_cast<std::uint8_t>(BehaviourTreeResult::Success)), nodeType(static_cast<std::uint8_t>(type)), reachedFromParent(true) {}
    
    virtual ~BehaviourTreeNode() {}
    
    virtual void initialize() {}
    virtual void dispose() {}
    virtual void abort() {}
    virtual void onArriveFromParent() {}
    virtual void onReturnFromChild([[maybe_unused]] BehaviourTreeResult result) {}
    virtual BehaviourResultNextNodePair update() = 0;
    
    /// A name to use when no name is set
    virtual const std::string& getDefaultName() const {
        static std::string name = "Tree Node";
        return name;
    }
    
    /// The max number of children this node is allowed to have
    virtual std::size_t maxChildren() const {
        return std::numeric_limits<std::size_t>::max();
    }
    
    /// Called when returning from a child node to a parent node
    inline void returnFromChild(BehaviourTreeResult result) {
        lastChildResult = static_cast<std::uint8_t>(result);
        reachedFromParent = false;
        
        onReturnFromChild(result);
    }
    
    inline BehaviourTreeResult getLastChildResult() const {
        return static_cast<BehaviourTreeResult>(lastChildResult);
    }
    
    inline void arriveFromParent() {
        reachedFromParent = true;
        
        onArriveFromParent();
    }
    
    inline BehaviourTreeNodeType getType() const {
        return static_cast<BehaviourTreeNodeType>(nodeType);
    }
    
    /// Can the node be cast to Decoratable?
    inline bool isDecoratable() const {
        const BehaviourTreeNodeType type = getType();
        if (type == BehaviourTreeNodeType::Task || type == BehaviourTreeNodeType::Composite) {
            return true;
        } else {
            return false;
        }
    }
    
    BehaviourTreeNode* getParent() {
        return parent;
    }
    
    inline const std::vector<BehaviourTreeNode*>& getChildren() const {
        return children;
    }
    
    inline bool wasReachedFromParent() const {
        return reachedFromParent;
    }
    
    inline bool hasName() const {
        return !name.empty();
    }
    
    inline const std::string& getName() const {
        if (!name.empty()) {
            return name;
        } else {
            return getDefaultName();
        }
    }
    
    inline void setName(std::string newName) {
        name = std::move(newName);
    }
    
    inline std::uint32_t getPriority() const {
        return priority;
    }
    
    inline std::uint16_t getDepth() const {
        return depth;
    }
    
    BehaviourTree* getTree() const {
        return tree;
    }
    
    Blackboard* getBlackboard() const;
    
    inline friend bool operator<(const BehaviourTreeNode& a, const BehaviourTreeNode& b) {
        assert(a.tree == b.tree);
        
        // Priority is lower when the number is higher
        return a.priority < b.priority;
    }
    
    inline friend bool operator==(const BehaviourTreeNode& a, const BehaviourTreeNode& b) {
        assert(a.tree == b.tree);
        
        // Priority is lower when the number is higher
        return a.priority == b.priority;
    }
    
private:
    friend class BehaviourTree;
    
    BehaviourTree* tree;
    BehaviourTreeNode* parent;
    
    std::vector<BehaviourTreeNode*> children;
    std::string name;
    
    std::uint32_t priority;
    std::uint16_t depth;
    
    std::uint8_t lastChildResult : 2;
    std::uint8_t nodeType : 3;
    bool reachedFromParent : 1;
    
    // 2 more free bits
};

class RootNode : public BehaviourTreeNode {
public:
    RootNode(BehaviourTree* tree) : BehaviourTreeNode(tree,  BehaviourTreeNodeType::Root) {
        setName("Root");
    }
    
    virtual const std::string& getDefaultName() const override {
        static std::string name = "Root Node";
        return name;
    }
    
    virtual std::size_t maxChildren() const final override {
        return 1;
    }
    
    virtual BehaviourResultNextNodePair update() final override;
};

class ServiceNode : public BehaviourTreeNode {
public:
    ServiceNode(BehaviourTree* tree, float timeBetweenActivations = 0.5f, float randomActivationDeviation = 0.0f)
        : BehaviourTreeNode(tree, BehaviourTreeNodeType::Service), timeBetweenActivations(timeBetweenActivations), randomActivationDeviation(randomActivationDeviation),
          timeUntilNextActivation(timeBetweenActivations), executeUpdateOnArrival(false), restartTimerOnArrival(false)
    {
        buildNewDistribution();
    }
    
    virtual std::size_t maxChildren() const override {
        return 0;
    }
    
    /// Use this to perform any setup needed before execution.
    virtual void handleActivation() = 0;
    
    /// Use this to execute the actual service.
    virtual void execute() = 0;
    
    virtual void onArriveFromParent() final override;
    virtual BehaviourResultNextNodePair update() final override;
    
    virtual const std::string& getDefaultName() const override {
        static std::string name = "Service Node";
        return name;
    }
    
    inline float getTimeUntilNextActivation() const {
        return timeUntilNextActivation;
    }
    
    inline float getTimeBetweenActivations() const {
        return timeBetweenActivations;
    }
    
    inline float getRandomActivationDeviation() const {
        return randomActivationDeviation;
    }
    
    inline bool executesUpdateOnArrival() const {
        return executeUpdateOnArrival;
    }
    
    inline bool restartsTimerOnArrival() const {
        return restartTimerOnArrival;
    }
    
    bool setTiming(float timeBetweenActivations, float randomDeviation, bool resetTimerToNew = true);
    
    inline void setExecuteUpdateOnArrival(bool execute) {
        executeUpdateOnArrival = execute;
    }
    
    inline void setRestartTimerOnArrival(bool restart) {
        restartTimerOnArrival = restart;
    }
private:
    void buildNewDistribution();
    void generateNextActivationTime();
    
    float timeBetweenActivations;
    float randomActivationDeviation;
    float timeUntilNextActivation;
    std::uniform_real_distribution<float> dist;
    
    bool executeUpdateOnArrival;
    bool restartTimerOnArrival;
};

/// Used to determine if the decorated node and/or the whole subtree is allowed
/// to execute.
///
/// \remark Classes that implement DecoratorNode must return either 
/// BehaviourTreeResult::Success or BehaviourTreeResult::Failure when update() is 
/// called.
class DecoratorNode : public BehaviourTreeNode {
public:
    DecoratorNode(BehaviourTree* tree, std::vector<StringHash> observedBlackboardValueNames, AbortMode abortMode)
        : BehaviourTreeNode(tree, BehaviourTreeNodeType::Decorator), observedBlackboardValueNames(std::move(observedBlackboardValueNames)), abortMode(abortMode) {}
        
    virtual void onObservedValueChange([[maybe_unused]] StringHash nameHash, [[maybe_unused]] bool availabilityChange, [[maybe_unused]] bool available) {}
    
    virtual const std::string& getDefaultName() const override {
        static std::string name = "Decorator Node";
        return name;
    }
    
    const std::vector<StringHash>& getObservedBlackboardValueNames() const {
        return observedBlackboardValueNames;
    }
    
    AbortMode getAbortMode() const {
        return abortMode;
    }
private:
    std::vector<StringHash> observedBlackboardValueNames;
    AbortMode abortMode;
};

class IsAvailableDecoratorNode : public DecoratorNode {
public:
    IsAvailableDecoratorNode(BehaviourTree* tree, StringHash observedBlackboardValue, bool invert, AbortMode abortMode) 
        : DecoratorNode(tree, {observedBlackboardValue}, abortMode), invert(invert) {}
    
    virtual void onObservedValueChange(StringHash nameHash, bool availabilityChange, bool available) final override;
    virtual BehaviourResultNextNodePair update() final override;
    virtual void initialize() final override;
    
    virtual const std::string& getDefaultName() const final override {
        static std::string name = "Is Available Decorator Node";
        return name;
    }
private:
    BehaviourTreeResult currentResult;
    bool invert;
};

class CompareValuesDecoratorNode : public DecoratorNode {
public:
    CompareValuesDecoratorNode(BehaviourTree* tree, std::vector<StringHash> observedBlackboardValueNames, bool invert, AbortMode abortMode) 
        : DecoratorNode(tree, observedBlackboardValueNames, abortMode), invert(invert) {}
    
    virtual void onObservedValueChange(StringHash nameHash, bool availabilityChange, bool available) final override;
    virtual BehaviourResultNextNodePair update() final override;
    virtual void initialize() final override;
    
    virtual const std::string& getDefaultName() const final override {
        static std::string name = "Compare Values Decorator Node";
        return name;
    }
private:
    void reevaluateResult();
    BlackboardValue a;
    BlackboardValue b;
    BehaviourTreeResult currentResult;
    bool invert;
};

class DecoratableNode : public BehaviourTreeNode {
public:
    DecoratableNode(BehaviourTree* tree, BehaviourTreeNodeType type) : BehaviourTreeNode(tree, type), cachedDecoratorResult(true) {}
    
    const std::vector<ServiceNode*>& getServices() const {
        return services;
    }
    
    const std::vector<DecoratorNode*>& getDecorators() const {
        return decorators;
    }
    
    inline bool decoratorsAllowExecution() const {
        return cachedDecoratorResult;
    }
private:
    friend class BehaviourTree;
    std::vector<ServiceNode*> services;
    std::vector<DecoratorNode*> decorators;
    
    bool cachedDecoratorResult;
};

class TaskNode : public DecoratableNode {
public:
    TaskNode(BehaviourTree* tree) : DecoratableNode(tree, BehaviourTreeNodeType::Task) {}
    
    virtual const std::string& getDefaultName() const override {
        static std::string name = "Task Node";
        return name;
    }
    
    virtual std::size_t maxChildren() const final override {
        return 0;
    }
};

class CompositeNode : public DecoratableNode {
public:
    CompositeNode(BehaviourTree* tree) : DecoratableNode(tree, BehaviourTreeNodeType::Composite) {}
};

class SequenceNode : public CompositeNode {
public:
    SequenceNode(BehaviourTree* tree) : CompositeNode(tree) {}
    
    virtual const std::string& getDefaultName() const override {
        static std::string name = "Sequence Node";
        return name;
    }
    
    virtual void onArriveFromParent() final override {
        nextChild = 0;
    }
    
    virtual BehaviourResultNextNodePair update() final override;
private:
    std::size_t nextChild;
};

class SelectorNode : public CompositeNode {
public:
    SelectorNode(BehaviourTree* tree) : CompositeNode(tree) {}
    
    virtual const std::string& getDefaultName() const override {
        static std::string name = "Selector Node";
        return name;
    }
    
    virtual void onArriveFromParent() final override {
        nextChild = 0;
    }
    
    virtual BehaviourResultNextNodePair update() final override;
private:
    std::size_t nextChild;
};

class BehaviourTree : public BlackboardCallbackListener {
public:
    BehaviourTree(Blackboard* blackboard, std::int32_t seed = std::numeric_limits<std::int32_t>::max());
    
    /// Is the BehaviourTree logging every step? Always returns false if the Engine was built without defining IYF_LOG_BEHAVIOUR_NODE_ACTIONS.
    bool isLoggingEnabled() const;
    
    /// Tell the behaviour tree to start or stop logging. The setting is ignored and false is returned if the Engine was built without defining IYF_LOG_BEHAVIOUR_NODE_ACTIONS.
    bool setLoggingEnabled(bool enabled);
    
    virtual ~BehaviourTree();
    
    void update(float delta);
    
//     /// Abort the running subtree, all services and start from root next time update() is called. This will not consume any pending notifications from the Blackboard.
//     void abort();
    
    virtual void valueUpdated(StringHash nameHash) final override;
    virtual void availabilityUpdated(StringHash nameHash, bool available) final override;
    
    inline RootNode* getRoot() {
        return root;
    }
    
    inline const RootNode* getRoot() const {
        return root;
    }
    
    // TODO prevent repeated insertions!!!
    
    template <typename T, typename... Ts>
    T* addNode(BehaviourTreeNode* parent, Ts... args) {
        checkForNewNodeErrors(parent);
        
        BehaviourTreeNode* newNode = allocateNode<T>(std::forward<Ts>(args)...);
        
        switch (newNode->getType()) {
        case BehaviourTreeNodeType::Composite:
        case BehaviourTreeNodeType::Task:
            if (parent->children.size() + 1 > parent->maxChildren()) {
                delete newNode;
                throw std::logic_error("This node type cannot have that many children");
            }
            parent->children.emplace_back(newNode);
            break;
        case BehaviourTreeNodeType::Service: {
            if (!parent->isDecoratable()) {
                delete newNode;
                throw std::logic_error("This node type cannot accept decorators or services");
            }
            
            DecoratableNode* decoratable = static_cast<DecoratableNode*>(parent);
            decoratable->services.emplace_back(static_cast<ServiceNode*>(newNode));
            break;
        }
        case BehaviourTreeNodeType::Decorator: {
            if (!parent->isDecoratable()) {
                delete newNode;
                throw std::logic_error("This node type cannot accept decorators or services");
            }
            
            DecoratableNode* decoratable = static_cast<DecoratableNode*>(parent);
            decoratable->decorators.emplace_back(static_cast<DecoratorNode*>(newNode));
            break;
        }
        case BehaviourTreeNodeType::Root:
            if (root != nullptr) {
                throw std::logic_error("You cannot create a second root node.");
            } else {
                root = static_cast<RootNode*>(newNode);
            }
            break;
        case BehaviourTreeNodeType::COUNT:
            throw std::logic_error("COUNT is not a valid node type");
        }
        
        newNode->parent = parent;
        
        return static_cast<T*>(newNode);
    }
    
    bool buildTree();
    
    std::string toString() const;
    
    void walkTree(const BehaviourTreeNode* node, std::function<void(const BehaviourTreeNode*)> function, bool childrenFirst = false) const;
    
    /// Used by the nodes to generate random values
    std::mt19937& getRandomNumberEngine() {
        return randomNumberEngine;
    }
    
    inline float getLastUpdateDelta() const {
        return lastDelta;
    }
    
    inline Blackboard* getBlackboard() const {
        return blackboard;
    }
    /// \remark For internal use, use in tests and debugging
    inline bool returnedToRoot() const {
        return activeBranch.empty();
    }
    
    /// \remark For use in tests and debugging
    inline BehaviourTreeNode* getNextNodeToExecute() const {
        return nextNodeToExecute;
    }
    
    /// \remark For use in tests and debugging
    inline BehaviourTreeNode* getLastExecutedNode() const {
        return lastExecutedNode;
    }
    
    /// \remark For use in tests and debugging
    inline std::size_t getActiveServiceCount() const {
        return activeServices.size();
    }
private:
    /// Starts from the end and aborts all items up to and including lastItemToAbort.
    ///
    /// \warning this function assumes the lastItemToAbort exist in the vector
    std::size_t abort(std::vector<BehaviourTreeNode*>::iterator lastItemToAbort);
    
    void walkTree(BehaviourTreeNode* node, std::function<void(BehaviourTreeNode*)> function, bool childrenFirst = false);
    
    template <typename T, typename... Ts>
    T* allocateNode(Ts... args) {
        //TODO as an optimization, it would be nice to eventually use a big memory buffer
        return new T(this, args...);
    }
    
    void recursiveTreeSetup(BehaviourTreeNode* node, std::uint32_t& priority, std::uint16_t depth);
    void logNodeAndResult(std::stringstream& ssLog, BehaviourTreeNode* node, BehaviourTreeResult result, bool blockedByDecorator) const;
    void setPendingNotifications(StringHash nameHash, bool availabilityChange, bool available);
    
    inline void checkForNewNodeErrors(BehaviourTreeNode* parent) const {
        checkIfBuilt();
        isParentNull(parent);
        isSameTree(parent);
    } 
    
    inline void checkIfBuilt() const {
        if (treeBuilt) {
            throw std::logic_error("Can't add nodes to an already built tree");
        }
    }
    
    inline void isParentNull(BehaviourTreeNode* parent) const {
        // Make sure we don't trigger an error when making the root node.
        if (parent == nullptr && root != nullptr) {
            throw std::logic_error("Parent node can't be null");
        }
    }
    
    inline void isSameTree(BehaviourTreeNode* parent) const {
        if (parent != nullptr && parent->tree != this) {
            throw std::logic_error("Parent node is from a different tree");
        }
    }
    
    Blackboard* blackboard;
    RootNode* root;
    std::uint64_t step;
    
    BehaviourTreeNode* nextNodeToExecute;
    BehaviourTreeNode* lastExecutedNode;
    
    struct ServicePointerComparator {
        inline bool operator()(const ServiceNode* const & a, const ServiceNode* const & b) const {
            return a->getPriority() < b->getPriority();
        }
    };
    
    std::vector<BehaviourTreeNode*> activeBranch;
    std::set<ServiceNode*, ServicePointerComparator> activeServices;
    
    std::vector<StringHash> subscribedValues;
    std::unordered_multimap<StringHash, DecoratorNode*> decoratorSubscriptionRegistry;
    
    std::mt19937 randomNumberEngine;
    
    struct PendingNotification {
        PendingNotification(bool availabilityChange, bool available)
            : availabilityChange(availabilityChange), available(available) {}
        
        bool availabilityChange;
        bool available;
        
        inline bool isAvailabilityChangeNotification() const {
            return availabilityChange;
        }
        
        inline bool isAvailable() const {
            if (availabilityChange) {
                return available;
            } else {
                return true;
            }
        }
        
        inline bool isUpdateNotification() const {
            return !availabilityChange;
        }
    };
    
    std::unordered_map<StringHash, PendingNotification> pendingNotifications;
    
    float lastDelta;
    bool treeBuilt;
    bool loggingEnabled;
};

inline Blackboard* BehaviourTreeNode::getBlackboard() const {
    return tree->getBlackboard();
}

}


#endif // IYF_BEHAVIOUR_TREE_HPP
