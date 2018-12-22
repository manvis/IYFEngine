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

#include "ai/BehaviourTree.hpp"

#include <sstream>
#include <algorithm>
#include <iomanip>

#include "ai/Blackboard.hpp"
#include "core/Logger.hpp"

#define IYF_LOG_BEHAVIOUR_NODE_ACTIONS

namespace iyf {
inline const char* TreeResultToString(BehaviourTreeResult result) {
    switch (result) {
    case BehaviourTreeResult::Success:
        return "Success";
    case BehaviourTreeResult::Failure:
        return "Failure";
    case BehaviourTreeResult::Running:
        return "Running";
    default:
        throw std::runtime_error("Unknown BehaviourTreeResult");
    }
}

BehaviourResultNextNodePair RootNode::update() {
    const std::size_t childCount = getChildren().size();
    assert(childCount <= 1);
    
    if (childCount == 1) {
        return {BehaviourTreeResult::Running, getChildren()[0]};
    } else if (childCount == 0) {
        return {BehaviourTreeResult::Running, this};
    } else {
        throw std::logic_error("A root node may only have a single child");
    }
}

void ServiceNode::buildNewDistribution() {
    if (randomActivationDeviation != 0.0f) {
        const float start = timeBetweenActivations - randomActivationDeviation;
        const float end = timeBetweenActivations + randomActivationDeviation;
        
        dist = std::uniform_real_distribution<float>(start, end);
    }
}

void ServiceNode::generateNextActivationTime() {
    if (randomActivationDeviation != 0.0f) {
        timeUntilNextActivation = dist(getTree()->getRandomNumberEngine());
    } else {
        timeUntilNextActivation = timeBetweenActivations;
    }
}

bool ServiceNode::setTiming(float timeBetweenActivations, float randomDeviation, bool resetTimerToNew) {
    if (timeBetweenActivations < 0.0f || randomDeviation > timeBetweenActivations) {
        return false;
    }
    
    this->timeBetweenActivations = timeBetweenActivations;
    this->randomActivationDeviation = randomDeviation;
    
    buildNewDistribution();
    
    if (resetTimerToNew) {
        generateNextActivationTime();
    }
    
    return true;
}

void ServiceNode::onArriveFromParent() {
    handleActivation();
    
    if (executeUpdateOnArrival) {
        execute();
    }
    
    if (restartTimerOnArrival) {
        generateNextActivationTime();
    }
}


BehaviourResultNextNodePair ServiceNode::update() {
    timeUntilNextActivation -= getTree()->getLastUpdateDelta();

    bool runsThisTime = (timeUntilNextActivation <= 0.0f);
    if (runsThisTime) {
        execute();
        generateNextActivationTime();
    }
    
    // This is ignored, unless logging is enabled
    return {runsThisTime ? BehaviourTreeResult::Running : BehaviourTreeResult::Success, nullptr};
}

BehaviourResultNextNodePair SequenceNode::update() {
    if (nextChild >= getChildren().size()) {
        if (!wasReachedFromParent()) {
            // A child has returned and we have no more children to execute.
            // Return its result and return execution to parent
            return {getLastChildResult(), getParent()};
        } else {
            // We were activated by the parent, but we're already at the end?
            // Means we have no child nodes. Notify success and return to parent
            return {BehaviourTreeResult::Success, getParent()};
        }
        // Ran through all nodes, need to return to parent and report result
        
    } else {
        if (!wasReachedFromParent()) {
            const BehaviourTreeResult childResult = getLastChildResult();
            
            if (childResult == BehaviourTreeResult::Success) {
                // The previous child succeeded, tell the tree to run the next child node
                const BehaviourResultNextNodePair resultPair(BehaviourTreeResult::Running, getChildren()[nextChild]);
                
                nextChild++;
                
                return resultPair;
            } else if (childResult == BehaviourTreeResult::Failure) {
                // Child failed, which means this node failed as well. Tell the tree to return to a parent node
                return {BehaviourTreeResult::Failure, getParent()};
            } else {
                throw std::logic_error("Invalid childResult value");
            }
        } else {
            // Execute the first node.
            const BehaviourResultNextNodePair resultPair(BehaviourTreeResult::Running, getChildren()[nextChild]);
                
            nextChild++;
            
            return resultPair;
        }
    }
}

BehaviourResultNextNodePair SelectorNode::update() {
    if (nextChild >= getChildren().size()) {
        if (!wasReachedFromParent()) {
            // A child has returned and we have no more children to execute.
            // Return its result and return execution to parent
            return {getLastChildResult(), getParent()};
        } else {
            // We were activated by the parent, but we're already at the end?
            // Means we have no child nodes. Notify failure (because we had no
            // successful children) and return to parent
            return {BehaviourTreeResult::Failure, getParent()};
        }
        // Ran through all nodes, need to return to parent and report result
        
    } else {
        if (!wasReachedFromParent()) {
            const BehaviourTreeResult childResult = getLastChildResult();
            
            if (childResult == BehaviourTreeResult::Success) {
                // The previous child succeeded, tell the tree the selector
                // succeeded and its parent needs to run next
                
                return {BehaviourTreeResult::Success, getParent()};
            } else if (childResult == BehaviourTreeResult::Failure) {
                // Child failed, which means we need to try the next one.
                const BehaviourResultNextNodePair resultPair(BehaviourTreeResult::Running, getChildren()[nextChild]);
                
                nextChild++;
                
                return resultPair;
            } else {
                throw std::logic_error("Invalid childResult value");
            }
        } else {
            // Execute the first node.
            const BehaviourResultNextNodePair resultPair(BehaviourTreeResult::Running, getChildren()[nextChild]);
                
            nextChild++;
            
            return resultPair;
        } 
        
    }
}

void IsAvailableDecoratorNode::initialize() {
    // This decorator can only track a single value
    const std::vector<StringHash>& values = getObservedBlackboardValueNames();
    assert(values.size() == 1);
    
    const BlackboardValueAvailability availability = getBlackboard()->isValueAvailable(values[0]);
    
    if (availability == BlackboardValueAvailability::Available) {
        currentResult = invert ? false : true;
    } else if (availability == BlackboardValueAvailability::NotAvailable) {
        currentResult = invert ? true : false;
    } else {
        throw std::runtime_error("Value not available");
    }
}

void IsAvailableDecoratorNode::onObservedValueChange(StringHash nameHash, bool availabilityChange, bool available) {
    const std::vector<StringHash>& values = getObservedBlackboardValueNames();
    
    // This decorator can only track a single value
    assert(values.size() == 1);
    assert(values[0] == nameHash);
    
    if (availabilityChange) {
        if (invert) {
            available = !available;
        }
        
        currentResult = available ? true : false;
    }
}

bool IsAvailableDecoratorNode::allowExecution() {
    return currentResult;
}

BehaviourResultNextNodePair IsAvailableDecoratorNode::update() {
    return {BehaviourTreeResult::Success, nullptr};
}

template <typename T>
constexpr bool CompareValues(ValueCompareOperation compareOp, T a, T b) {
    switch (compareOp) {
        case ValueCompareOperation::Less:
            return (a < b);
        case ValueCompareOperation::LessOrEqual:
            return (a <= b);
        case ValueCompareOperation::Equal:
            return (a == b);
        case ValueCompareOperation::GreaterOrEqual:
            return (a >= b);
        case ValueCompareOperation::Greater:
            return (a > b);
        case ValueCompareOperation::NotEqual:
            return (a != b);
    }
    
    return false;
}

CompareValuesDecoratorNode::CompareValuesDecoratorNode(BehaviourTree* tree, std::vector<StringHash> observedBlackboardValueNames, ValueCompareOperation compareOp, AbortMode abortMode) 
    : DecoratorNode(tree, observedBlackboardValueNames, abortMode), compareOp(compareOp) {}

void CompareValuesDecoratorNode::reevaluateResult() {
    bool result = std::visit([this](auto&& a, auto&& b) -> bool {
        using T1 = std::decay_t<decltype(a)>;
        using T2 = std::decay_t<decltype(b)>;
        
        if constexpr (!std::is_same_v<T1, T2>) {
            return false;
        } else if constexpr (std::is_same_v<T1, bool> || std::is_same_v<T1, float> || std::is_same_v<T1, int> || std::is_same_v<T1, std::string>) {
            return CompareValues<T1>(compareOp, a, b);
        } else if constexpr (std::is_same_v<T1, glm::vec3>) {
            switch (compareOp) {
                case ValueCompareOperation::Less:
                    return glm::all(glm::lessThan(a, b));
                case ValueCompareOperation::LessOrEqual:
                    return glm::all(glm::lessThanEqual(a, b));
                case ValueCompareOperation::Equal:
                    return glm::all(glm::equal(a, b));
                case ValueCompareOperation::GreaterOrEqual:
                    return glm::all(glm::greaterThanEqual(a, b));
                case ValueCompareOperation::Greater:
                    return glm::all(glm::greaterThan(a, b));
                case ValueCompareOperation::NotEqual:
                    return glm::all(glm::notEqual(a, b));
            }
            
            throw std::runtime_error("Unknown ValueCompareOperation");
        } else if constexpr (std::is_same_v<T1, void*>) {
            if (compareOp == ValueCompareOperation::Equal) {
                return a == b;
            } else {
                return false;
            }
        } else {
            static_assert(FalseType<T1>::value, "Unhandled type.");
        }
    }, a, b);
    
    currentResult = result;
}

void CompareValuesDecoratorNode::initialize() {
    const auto& observed = getObservedBlackboardValueNames();
    if (observed.size() != 2) {
        throw std::logic_error("Two observed values must have been provided");
    }
    
    const Blackboard* blackboard = getBlackboard();
    a = blackboard->getRawValue(observed[0]);
    b = blackboard->getRawValue(observed[1]);
    
    if (a.index() != b.index()) {
        throw std::logic_error("The observed values must be of the same type");
    }
    
    reevaluateResult();
}

void CompareValuesDecoratorNode::onObservedValueChange(StringHash nameHash, bool, bool) {
    const std::vector<StringHash>& values = getObservedBlackboardValueNames();
    
    assert(values.size() == 2);
    assert(nameHash == values[0] || nameHash == values[1]);
    
    const Blackboard* blackboard = getBlackboard();
    
    BlackboardValue& updatedValue = (nameHash == values[0]) ? a : b;
    updatedValue = blackboard->getRawValue(nameHash);
    
    reevaluateResult();
}

bool CompareValuesDecoratorNode::allowExecution() {
    return currentResult;
}

BehaviourResultNextNodePair CompareValuesDecoratorNode::update() {
    return {BehaviourTreeResult::Success, nullptr};
}

CompareValueConstantDecoratorNode::CompareValueConstantDecoratorNode(BehaviourTree* tree, StringHash observedBlackboardValueName, BlackboardValue b, ValueCompareOperation compareOp, AbortMode abortMode) 
    : CompareValuesDecoratorNode(tree, {observedBlackboardValueName}, compareOp, abortMode)
{
    this->b = b;
}

void CompareValueConstantDecoratorNode::onObservedValueChange(StringHash nameHash, bool, bool) {
    const std::vector<StringHash>& values = getObservedBlackboardValueNames();
    
    assert(values.size() == 1);
    assert(nameHash == values[0]);
    
    const Blackboard* blackboard = getBlackboard();
    a = blackboard->getRawValue(nameHash);
    
    reevaluateResult();
}

void CompareValueConstantDecoratorNode::initialize() {
    const auto& observed = getObservedBlackboardValueNames();
    if (observed.size() != 1) {
        throw std::logic_error("A single observed value must have been provided");
    }
    
    const Blackboard* blackboard = getBlackboard();
    a = blackboard->getRawValue(observed[0]);
    
    if (a.index() != b.index()) {
        throw std::logic_error("The observed values must be of the same type");
    }
    
    reevaluateResult();
}

BehaviourTree::BehaviourTree(Blackboard* blackboard, std::int32_t seed) 
        : blackboard(blackboard), root(nullptr), step(0), nextNodeToExecute(nullptr), lastExecutedNode(nullptr), lastDelta(0.0f), treeBuilt(false), loggingEnabled(false)
{
    if (seed == std::numeric_limits<std::int32_t>::max()) {
        randomNumberEngine = std::mt19937(seed);
    } else {
        std::random_device rd;
        randomNumberEngine = std::mt19937(rd());
    }
    
    addNode<RootNode>(nullptr);
    nextNodeToExecute = root;
}

BehaviourTree::~BehaviourTree() {
    for (const StringHash& sh : subscribedValues) {
        blackboard->unregisterListener(sh, this);
    }
    
    walkTree(root, [](BehaviourTreeNode* node){
        node->dispose();
        delete node;
    }, true);
}

bool BehaviourTree::isLoggingEnabled() const {
#ifdef IYF_LOG_BEHAVIOUR_NODE_ACTIONS
    return loggingEnabled;
#else // IYF_LOG_BEHAVIOUR_NODE_ACTIONS
    return false;
#endif // IYF_LOG_BEHAVIOUR_NODE_ACTIONS
}

bool BehaviourTree::setLoggingEnabled(bool enabled) {
    loggingEnabled = enabled;

#ifdef IYF_LOG_BEHAVIOUR_NODE_ACTIONS
    return true;
#else // IYF_LOG_BEHAVIOUR_NODE_ACTIONS
    return false;
#endif // IYF_LOG_BEHAVIOUR_NODE_ACTIONS
}

std::string BehaviourTree::toString() const {
    std::stringstream ss;
    
    if (!treeBuilt) {
        ss << "The tree hasn't been built yet\n";
        return ss.str();
    }
    
    walkTree(getRoot(), [&ss](const BehaviourTreeNode* node) {
        ss << std::setw(6) << node->getPriority();
        
        for (std::size_t d = 0; d <= node->getDepth(); ++d) {
            ss << " ";
        }
        
        if (node->getType() == BehaviourTreeNodeType::Decorator) {
            ss << u8"┌!D! ";
        } else if (node->getType() == BehaviourTreeNodeType::Service) {
            ss << u8"└!S! ";
        }
        
        if (node->hasName()) {
            ss << node->getName();
        } else {
            ss << "Unnamed";
        }
        
        ss << " (";
        ss << node->getDefaultName();
        ss << ")\n";
    }, false);
    
    return ss.str();
}

void BehaviourTree::recursiveTreeSetup(BehaviourTreeNode* node, std::uint32_t& priority, std::uint16_t depth) {
    std::vector<BehaviourTreeNode*>& children = node->children;
    const bool isDecoratable = node->isDecoratable();
    
    
    if (isDecoratable) {
        std::vector<DecoratorNode*>& decorators = static_cast<DecoratableNode*>(node)->decorators;
        for (std::size_t i = 0; i < decorators.size(); ++i) {
            priority++;
            
            DecoratorNode* d = decorators[i];
            
            d->priority = priority;
            d->depth = depth;
            
            const auto& observed = d->getObservedBlackboardValueNames();
            for (StringHash sh : observed) {
                const auto result = decoratorSubscriptionRegistry.find(sh);
                if (result == decoratorSubscriptionRegistry.end()) {
                    blackboard->registerListener(sh, this);
                    subscribedValues.emplace_back(sh);
                }
                
                decoratorSubscriptionRegistry.insert({sh, d});
            }
            
            d->initialize();
        }
    }
    
    node->priority = priority;
    node->depth = depth;
    
    if (isDecoratable) {
        std::vector<ServiceNode*>& services = static_cast<DecoratableNode*>(node)->services;
        for (std::size_t i = 0; i < services.size(); ++i) {
            priority++;
            
            services[i]->priority = priority;
            services[i]->depth = depth;
            services[i]->initialize();
        }
    }
    
    node->initialize();
    
    const std::uint16_t newDepth = depth + 1;
    for (std::size_t i = 0; i < children.size(); ++i) {
        priority++;
        recursiveTreeSetup(children[i], priority, newDepth);
    }
}

void BehaviourTree::walkTree(const BehaviourTreeNode* node, std::function<void(const BehaviourTreeNode*)> function, bool childrenFirst) const {
    assert(node != nullptr);
    const bool isDecoratable = node->isDecoratable();
    
    if (isDecoratable) {
        const std::vector<DecoratorNode*>& decorators = static_cast<const DecoratableNode*>(node)->getDecorators();
        for (const DecoratorNode* decorator : decorators) {
            function(decorator);
        }
    }
    
    if (!childrenFirst) {
        function(node);
    }
    
    if (isDecoratable) {
        const std::vector<ServiceNode*>& services = static_cast<const DecoratableNode*>(node)->getServices();
        for (const ServiceNode* service : services) {
            function(service);
        }
    }
    
    const std::vector<BehaviourTreeNode*> children = node->getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) {
        walkTree(children[i], function, childrenFirst);
    }
    
    if (childrenFirst) {
        function(node);
    }
}

void BehaviourTree::walkTree(BehaviourTreeNode* node, std::function<void(BehaviourTreeNode*)> function, bool childrenFirst) {
    assert(node != nullptr);
    bool isDecoratable = node->isDecoratable();
    
    if (isDecoratable) {
        std::vector<DecoratorNode*>& decorators = static_cast<DecoratableNode*>(node)->decorators;
        for (DecoratorNode* decorator : decorators) {
            function(decorator);
        }
    }
    
    if (!childrenFirst) {
        function(node);
    }
    
    if (isDecoratable) {
        std::vector<ServiceNode*>& services = static_cast<DecoratableNode*>(node)->services;
        for (ServiceNode* service : services) {
            function(service);
        }
    }
    
    std::vector<BehaviourTreeNode*> children = node->getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) {
        walkTree(children[i], function, childrenFirst);
    }
    
    if (childrenFirst) {
        function(node);
    }
}

bool BehaviourTree::buildTree() {
    if (treeBuilt) {
        return false;
    }
    
    std::uint32_t priority = 0;
    recursiveTreeSetup(root, priority, 0);
    
    treeBuilt = true;
    return true;
}

void BehaviourTree::update(float delta) {
    lastDelta = delta;
    
#ifdef IYF_LOG_BEHAVIOUR_NODE_ACTIONS
        std::stringstream ssLog;
        
        if (loggingEnabled) {
            ssLog << "STEP: " << step << "\n\tACTIVE SERVICE IDs: ";
            for (const ServiceNode* service : activeServices) {
                ssLog << service->getPriority() << " ";
            }
            ssLog << "\n";
        }
#endif // IYF_LOG_BEHAVIOUR_NODE_ACTIONS

    // First of all, update services and let them change the blackboard
    for (ServiceNode* service : activeServices) {
        [[maybe_unused]] auto result = service->update();
        
#ifdef IYF_LOG_BEHAVIOUR_NODE_ACTIONS
        logNodeAndResult(ssLog, service, result.first, false);
#endif // IYF_LOG_BEHAVIOUR_NODE_ACTIONS
    }

    // Next, process all pending blackboard value change notifications.
    for (const auto& n : pendingNotifications) {
        auto result = decoratorSubscriptionRegistry.equal_range(n.first);

        for (auto it = result.first; it != result.second; ++it) {
#ifdef IYF_LOG_BEHAVIOUR_NODE_ACTIONS
            bool ownSubtreeAborted = false;
            bool lowerPriorityAborted = false;
            std::size_t abortedOwnSubtreeNodeCount = 0;
            std::size_t abortedLowerPriorityNodeCount = 0;
#endif // IYF_LOG_BEHAVIOUR_NODE_ACTIONS
        
            DecoratorNode* processedDecorator = it->second;
            processedDecorator->onObservedValueChange(n.first, n.second.availabilityChange, n.second.available);
            
            const AbortMode abortMode = processedDecorator->getAbortMode();
            
            BehaviourTreeNode* parent = processedDecorator->getParent();
            DecoratableNode* decoratable = static_cast<DecoratableNode*>(parent);
            
            auto decoratableIterator = activeBranch.end();
            
            // If we need to abort something, we need to check if the decoratable is in the active branch or not
            if (abortMode != AbortMode::None) {
                for (auto n = activeBranch.begin(); n != activeBranch.end() ; ++n) {
                    if ((*n) == parent) {
                        decoratableIterator = n;
                    }
                }
            }
            
            // We can only abort own subtree if it's being executed now
            if ((abortMode == AbortMode::OwnSubtree || abortMode == AbortMode::Both) && decoratableIterator != activeBranch.end()) {
                [[maybe_unused]] std::size_t abortedNodeCount = abort(decoratableIterator);
                
#ifdef IYF_LOG_BEHAVIOUR_NODE_ACTIONS
                abortedOwnSubtreeNodeCount = abortedNodeCount;
                ownSubtreeAborted = true;
#endif // IYF_LOG_BEHAVIOUR_NODE_ACTIONS
                
                nextNodeToExecute = decoratable->getParent();
                
                // TODO is returning failure really our best option here?
                nextNodeToExecute->returnFromChild(BehaviourTreeResult::Failure);
                
                lastExecutedNode = decoratable;
            }
            
            // We should only abort lower priority nodes if they 
            if ((abortMode == AbortMode::LowerPriority || abortMode == AbortMode::Both) && decoratableIterator == activeBranch.end()) {
                const std::uint32_t priority = decoratable->getPriority();
                
                auto lastNodeToAbort = activeBranch.end();
                for (auto n = activeBranch.begin(); n != activeBranch.end(); ++n) {
                    if ((*n)->getPriority() >= priority) {
                        lastNodeToAbort = n;
                        break;
                    }
                }
                
                [[maybe_unused]] std::size_t abortedNodeCount = 0;
                
                if (lastNodeToAbort != activeBranch.end()) {
                    abortedNodeCount = abort(lastNodeToAbort);
                    
                    nextNodeToExecute = (*lastNodeToAbort)->getParent();
                    
                    // TODO is returning failure really our best option here?
                    nextNodeToExecute->returnFromChild(BehaviourTreeResult::Failure);
                    
                    lastExecutedNode = (*lastNodeToAbort);
                }
                
#ifdef IYF_LOG_BEHAVIOUR_NODE_ACTIONS
                abortedLowerPriorityNodeCount = abortedNodeCount;
                lowerPriorityAborted = true;
#endif // IYF_LOG_BEHAVIOUR_NODE_ACTIONS
            }
            
#ifdef IYF_LOG_BEHAVIOUR_NODE_ACTIONS
            const char* modeStr = nullptr;
            switch (abortMode) {
                case AbortMode::None:
                    modeStr = "FALSE";
                    break;
                case AbortMode::LowerPriority:
                    modeStr = "LOWER PRIORITY";
                    break;
                case AbortMode::OwnSubtree:
                    modeStr = "OWN SUBTREE";
                    break;
                case AbortMode::Both:
                    modeStr = "OWN SUBTREE AND LOWER PRIORITY";
                    break;
            }
            
            if (loggingEnabled) {
                ssLog << "\tDECORATOR: \"" << processedDecorator->getName() <<"\" got notified about a value change.\n"
                        "\t\t   Node abort requested? " << modeStr << "\n" <<
                        "\t\t   Own subtree aborted? " << std::boolalpha << ownSubtreeAborted << " (" << abortedOwnSubtreeNodeCount << " node(s))\n" <<
                        "\t\t   Lower priority nodes aborted? " << std::boolalpha << lowerPriorityAborted << " (" << abortedLowerPriorityNodeCount << " node(s))\n" <<
                        "\t\t   Next node to execute is: \"" << nextNodeToExecute->getName() << "\"\n";
            }
            
#endif // IYF_LOG_BEHAVIOUR_NODE_ACTIONS
            // TODO figure out where to put branch abandonments
        }
    }
    
    pendingNotifications.clear();

    do {
        assert(nextNodeToExecute != nullptr);
        
        BehaviourResultNextNodePair result;
        
        // If the next node has decorators, we need to make sure they allow its execution
        if (nextNodeToExecute->isDecoratable() && !checkIfDecoratorsAllowExecution(static_cast<DecoratableNode*>(nextNodeToExecute))) {
            result = {BehaviourTreeResult::Failure, nextNodeToExecute->getParent()};
            
#ifdef IYF_LOG_BEHAVIOUR_NODE_ACTIONS
            if (loggingEnabled) {
                logNodeAndResult(ssLog, nextNodeToExecute, result.first, true);
            }
#endif // IYF_LOG_BEHAVIOUR_NODE_ACTIONS
            
            lastExecutedNode = nextNodeToExecute;
            nextNodeToExecute = result.second;
        } else {
            result = nextNodeToExecute->update();
            
#ifdef IYF_LOG_BEHAVIOUR_NODE_ACTIONS
            if (loggingEnabled) {
                logNodeAndResult(ssLog, nextNodeToExecute, result.first, false);
            }
#endif // IYF_LOG_BEHAVIOUR_NODE_ACTIONS
            
            lastExecutedNode = nextNodeToExecute;
            nextNodeToExecute = result.second;
        }

        const std::size_t nextDepth = nextNodeToExecute->getDepth();
        const std::size_t lastDepth = lastExecutedNode->getDepth();
        if (nextDepth > lastDepth) {
            // Check if we need to tell about our arrival. Arrival logic shouldn't run if the decorators won't allow the node to run (it will be
            // popped immediately during the next iteration)
            const bool nextIsDecoratable = nextNodeToExecute->isDecoratable();
            const bool shouldNotifyArrivalToDecoratable = (nextIsDecoratable &&
                                                           checkIfDecoratorsAllowExecution(static_cast<DecoratableNode*>(nextNodeToExecute)));
            
            if (shouldNotifyArrivalToDecoratable) {
                const DecoratableNode* node = static_cast<DecoratableNode*>(nextNodeToExecute);
                
                const auto& decorators = node->getDecorators();
                for (DecoratorNode* decorator : decorators) {
                    decorator->arriveFromParent();
                }
            }
            
            if (!nextIsDecoratable || shouldNotifyArrivalToDecoratable) {
                nextNodeToExecute->arriveFromParent();
            }
            activeBranch.emplace_back(nextNodeToExecute);
            
            if (shouldNotifyArrivalToDecoratable) {
                const DecoratableNode* node = static_cast<DecoratableNode*>(nextNodeToExecute);
                
                const auto& services = node->getServices();
                for (ServiceNode* service : services) {
                    service->arriveFromParent();
                    auto result = activeServices.emplace(service);
                    assert(result.second);
                }
            }
        } else if (nextDepth < lastDepth) {
            // TODO Check decorators to see if parent can run and if the decorator adjusts flow (e.g., forces us into loop)
            nextNodeToExecute->returnFromChild(result.first);
            activeBranch.pop_back();

            if (lastExecutedNode->isDecoratable()) {
                const DecoratableNode* node = static_cast<DecoratableNode*>(lastExecutedNode);
                
                const auto& services = node->getServices();
                for (ServiceNode* service : services) {
                    const std::size_t removed = activeServices.erase(service);
                    assert(removed == 1);
                }
            }
        }
/*#ifdef IYF_LOG_BEHAVIOUR_NODE_ACTIONS
    if (loggingEnabled) {
        ssLog << "\t\tBRANCH: ";
        for (BehaviourTreeNode* node : activeBranch) {
            ssLog << node->getPriority() << " ";
        }
        ssLog << "\n";
    }
#endif // IYF_LOG_BEHAVIOUR_NODE_ACTIONS */
    } while (lastExecutedNode->getType() != BehaviourTreeNodeType::Task && !returnedToRoot());
    step++;
#ifdef IYF_LOG_BEHAVIOUR_NODE_ACTIONS
    if (loggingEnabled) {
        LOG_D(ssLog.str());
    }
#endif // IYF_LOG_BEHAVIOUR_NODE_ACTIONS
}

bool BehaviourTree::checkIfDecoratorsAllowExecution(DecoratableNode* decoratable) {
    auto& decorators = decoratable->getDecorators();
    bool allowExecution = true;
    
    for (DecoratorNode* d : decorators) {
        allowExecution &= d->allowExecution();
    }
    
    return allowExecution;
}

// void BehaviourTree::abort() {
//     throw std::runtime_error("Not yet implemented");
// }

std::size_t BehaviourTree::abort(std::vector<BehaviourTreeNode*>::iterator lastItemToAbort) {
    if ((*lastItemToAbort)->getType() == BehaviourTreeNodeType::Root) {
        throw std::logic_error("Cannot abort root");
    }
    
    if (lastItemToAbort == activeBranch.end()) {
        return 0;
    }
    
    std::size_t abortedNodeCount = 0;
    for (auto i = --activeBranch.end(); i >= lastItemToAbort;) {
        BehaviourTreeNode* node = (*i);
        node->abort();
        
#ifdef IYF_LOG_BEHAVIOUR_NODE_ACTIONS
        if (loggingEnabled) {
            LOG_D("Aborting Node: \"" << node->getName() << "\"");
        }
#endif // IYF_LOG_BEHAVIOUR_NODE_ACTIONS
        
        if (node->isDecoratable()) {
            DecoratableNode* decoratableNode = static_cast<DecoratableNode*>(node);
            
            const auto& decorators = decoratableNode->getDecorators();
            for (DecoratorNode* decorator : decorators) {
                decorator->abort();
                
                abortedNodeCount++;
                
#ifdef IYF_LOG_BEHAVIOUR_NODE_ACTIONS
                if (loggingEnabled) {
                    LOG_D("Aborting Decorator \"" << decorator->getName() << "\" of Node: \"" << node->getName() << "\"");
                }
#endif // IYF_LOG_BEHAVIOUR_NODE_ACTIONS
            }
            
            const auto& services = decoratableNode->getServices();
            for (ServiceNode* service : services) {
                service->abort();
                
                const std::size_t removalCount = activeServices.erase(service);
                assert(removalCount == 1);
                abortedNodeCount += removalCount;
                
#ifdef IYF_LOG_BEHAVIOUR_NODE_ACTIONS
                if (loggingEnabled) {
                    LOG_D("Aborting Service \"" << service->getName() << "\" of Node: \"" << node->getName() << "\"");
                }
#endif // IYF_LOG_BEHAVIOUR_NODE_ACTIONS
            }
        }
        
        i--;
        
        activeBranch.pop_back();
        abortedNodeCount++;
    }
    
    return abortedNodeCount;
}

void BehaviourTree::logNodeAndResult(std::stringstream& ssLog, BehaviourTreeNode* node, BehaviourTreeResult result, bool blockedByDecorator) const {
    ssLog << "\t";
    
    if (node->hasName()) {
        ssLog << node->getName();
    } else {
        ssLog << "Unnamed";
    }
              
    ssLog << " (" << node->getDefaultName()
              << ", P: "
              << node->getPriority()
              << ", D: "
              << node->getDepth()
              << ");";
    
    if (node->getType() == BehaviourTreeNodeType::Service) {
        if (result == BehaviourTreeResult::Running) {
            ssLog << " RES: Running"; 
        } else {
            ssLog << " RES: Sleeping"; 
        }
    } else {
        ssLog << " RES: ";
        
        if (result == BehaviourTreeResult::Failure && blockedByDecorator) {
            ssLog << TreeResultToString(result) << " (blocked by a decorator)";
        } else {
            ssLog << TreeResultToString(result);
        }
    }
    
    ssLog << "\n";
}

void BehaviourTree::valueUpdated(StringHash nameHash) {
    setPendingNotifications(nameHash, false, false);
}

void BehaviourTree::availabilityUpdated(StringHash nameHash, bool available) {
    setPendingNotifications(nameHash, true, available);
}

void BehaviourTree::setPendingNotifications(StringHash nameHash, bool availabilityChange, bool available) {
    auto result = pendingNotifications.find(nameHash);
    if (result == pendingNotifications.end()) {
        pendingNotifications.emplace(std::make_pair(nameHash, PendingNotification(availabilityChange, available)));
    } else if (availabilityChange) {
        // Always promote to an availability change notification
        result->second.availabilityChange = true;
        result->second.available = available;
    }
}


}
