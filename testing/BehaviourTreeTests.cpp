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

#include "BehaviourTreeTests.hpp"

#include "ai/BehaviourTree.hpp"
#include "ai/Blackboard.hpp"

#include <iomanip>

#define IYF_TEST_EMULATED_UPDATE_INTERVAL 0.1 // 100 milliseconds between "updates"
#define IYF_VERBOSE_TREE_OUTPUT false // If enabling this does nothing, make sure IYF_LOG_BEHAVIOUR_NODE_ACTIONS is defined in BehaviourTree.cpp

const std::string KEY_IN_POCKET("KeyInPocket");
const iyf::StringHash KEY_IN_POCKET_HASH = iyf::HS(KEY_IN_POCKET);

const std::string ONLOOKERS_PRESENT("OnlookersPresent");
const iyf::StringHash ONLOOKERS_PRESENT_HASH = iyf::HS(ONLOOKERS_PRESENT);

const std::string TRUE_VALUE("TrueValue");
const iyf::StringHash TRUE_VALUE_HASH = iyf::HS(TRUE_VALUE);

namespace iyf::test {
inline const char* ReportIDName(ReportID id) {
    switch (id) {
        case ReportID::SulkingAndWaiting:
            return "Sulking and waiting";
        case ReportID::GoingTowardsTheDoor:
            return "Going towards the door";
        case ReportID::UsingKey:
            return "Using the key";
        case ReportID::LookingForSpare:
            return "Looking for a spare key";
        case ReportID::EnteringThroughTheDoor:
            return "Entering through the door";
        case ReportID::GoingTowardsTheWindow:
            return "Going towards the window";
        case ReportID::OpeningWindowGently:
            return "Opening the window gently";
        case ReportID::BreakingWindow:
            return "Breaking the window";
        case ReportID::BansheeScreamingAtWindow:
            return "Banshee screaming at window";
        case ReportID::EnteringThroughTheWindow:
            return "Entering through the window";
        case ReportID::CheckingForOnlookers:
            return "(Onlooker Service) Checking for onlookers";
        case ReportID::OnlookersDetected:
            return "(Onlooker Service) Onlookers detected";
        case ReportID::OnlookersLeft:
            return "(Onlooker Service) Onlookers left";
        case ReportID::ThinkingAboutAKey:
            return "(Key Check Service) Thinking about a key";
        case ReportID::KeyFound:
            return "(Key Check Service) Key Found";
        case ReportID::ERROR:
            return "ERROR - invalid report ID";
    }
    
    return nullptr;
}

inline const char* ResultName(BehaviourTreeResult result) {
    switch (result) {
        case BehaviourTreeResult::Success:
            return "SUCCESS";
        case BehaviourTreeResult::Failure:
            return "FAILURE";
        case BehaviourTreeResult::Running:
            return "RUNNING";
    }
    
    return nullptr;
}

struct TestTaskResults {
    TestTaskResults() : canUseKey(true), spareExists(true), windowNotLocked(true), windowBreakable(true), canBansheeScream(true) {}
    bool canUseKey;
    bool spareExists;
    bool windowNotLocked;
    bool windowBreakable;
    bool canBansheeScream;
};

struct ServiceReportIDs {
    inline ServiceReportIDs(ReportID pending, ReportID revertToPre, ReportID setToPost) : pending(pending), revertToPre(revertToPre), setToPost(setToPost) {}
    
    ReportID pending;
    ReportID revertToPre;
    ReportID setToPost;
};

class TimedTriggerService : public iyf::ServiceNode {
public:
    TimedTriggerService(BehaviourTree* tree, float timeUntilTrigger, ServiceReportIDs reportIDs, std::vector<ProgressReport>* reports, StringHash nameHash, BlackboardValue preChangeValue, BlackboardValue postChangeValue)
        : ServiceNode(tree), nameHash(nameHash), preChangeValue(preChangeValue), postChangeValue(postChangeValue), remainingTimeUntilTrigger(timeUntilTrigger), timeUntilTrigger(timeUntilTrigger), reports(reports), reportIDs(reportIDs)
    {
        Blackboard* bb = getBlackboard();
        if (bb->getRawValue(nameHash) != preChangeValue) {
            throw std::runtime_error("The Blackboard value must be equal to the preChangeValue");
        }
    }
    
    virtual void handleActivation() {
        remainingTimeUntilTrigger = timeUntilTrigger;
    }
    
    virtual void execute() {
        remainingTimeUntilTrigger -= getTimeBetweenActivations();
        
        if (remainingTimeUntilTrigger <= 0.0f) {
            Blackboard* bb = getBlackboard();
            if (bb->getRawValue(nameHash) == preChangeValue) {
                bb->setValue(nameHash, postChangeValue);
                reports->emplace_back(reportIDs.setToPost, BehaviourTreeResult::Running, getTree()->getStepNumber());
            } else {
                bb->setValue(nameHash, preChangeValue);
                reports->emplace_back(reportIDs.revertToPre, BehaviourTreeResult::Running, getTree()->getStepNumber());
            }
            
            assert(std::numeric_limits<float>::has_infinity);
            remainingTimeUntilTrigger = std::numeric_limits<float>::infinity();
        } else {
            reports->emplace_back(reportIDs.pending, BehaviourTreeResult::Running, getTree()->getStepNumber());
        }
    }
private:
    StringHash nameHash;
    BlackboardValue preChangeValue;
    BlackboardValue postChangeValue;
    float remainingTimeUntilTrigger;
    float timeUntilTrigger;
    
    std::vector<ProgressReport>* reports;
    ServiceReportIDs reportIDs;
};

class ProgressReportingTask : public iyf::TaskNode {
public:
    ProgressReportingTask(BehaviourTree* tree, ReportID reportID, std::vector<ProgressReport>* reports, std::size_t tickDelay, bool succeed)
        : TaskNode(tree), reports(reports), remainingTicks(tickDelay), tickDelay(tickDelay), succeedPtr(nullptr), reportID(reportID), succeed(succeed) {}
    
    ProgressReportingTask(BehaviourTree* tree, ReportID reportID, std::vector<ProgressReport>* reports, std::size_t tickDelay, const bool* succeedPtr)
        : TaskNode(tree), reports(reports), remainingTicks(tickDelay), tickDelay(tickDelay), succeedPtr(succeedPtr), reportID(reportID), succeed(false) {}
    
    virtual void onArriveFromParent() override {
        remainingTicks = tickDelay;
    }
    
    virtual BehaviourResultNextNodePair update() override {
        if (remainingTicks > 0) {
            reports->emplace_back(reportID, BehaviourTreeResult::Running, getTree()->getStepNumber());
            remainingTicks--;
            
            return {BehaviourTreeResult::Running, this};
        } else {
            const BehaviourTreeResult result = shouldSucceed() ? BehaviourTreeResult::Success : BehaviourTreeResult::Failure;
            
            reports->emplace_back(reportID, result, getTree()->getStepNumber());
            return {result, getParent()};
        }
    }
protected:
    inline bool shouldSucceed() const {
        if (succeedPtr == nullptr) {
            return succeed;
        } else {
            return *succeedPtr;
        }
    }
        
    std::vector<ProgressReport>* reports;
    std::size_t remainingTicks;
    std::size_t tickDelay;
    const bool* succeedPtr;
    ReportID reportID;
    bool succeed;
};

inline ProgressReportingTask* MakeProgressReportingTask(iyf::BehaviourTree* tree, BehaviourTreeNode* parent, const char* name, ReportID reportID, std::vector<ProgressReport>* reports, bool succeed, std::size_t tickDelay) {
    ProgressReportingTask* prt = tree->addNode<ProgressReportingTask>(parent, reportID, reports, tickDelay, succeed);
    prt->setName(name);
    
    return prt;
}

inline ProgressReportingTask* MakeProgressReportingTask(iyf::BehaviourTree* tree, BehaviourTreeNode* parent, const char* name, ReportID reportID, std::vector<ProgressReport>* reports, const bool* succeed, std::size_t tickDelay) {
    ProgressReportingTask* prt = tree->addNode<ProgressReportingTask>(parent, reportID, reports, tickDelay, succeed);
    prt->setName(name);
    
    return prt;
}

#define IYF_MAKE_REPORTING_TASK(parent, name, reportID, succeed, tickDelay) MakeProgressReportingTask(tree.get(), parent, name, reportID, reportVector, succeed, tickDelay)

    
inline iyf::SequenceNode* MakeSequence(iyf::BehaviourTree* tree, BehaviourTreeNode* parent, const char* name) {
    iyf::SequenceNode* node = tree->addNode<iyf::SequenceNode>(parent);
    node->setName(name);
    
    return node;
}

#define IYF_MAKE_SEQUENCE(parent, name) MakeSequence(tree.get(), parent, name);

inline iyf::SelectorNode* MakeSelector(iyf::BehaviourTree* tree, BehaviourTreeNode* parent, const char* name) {
    iyf::SelectorNode* node = tree->addNode<iyf::SelectorNode>(parent);
    node->setName(name);
    
    return node;
}

#define IYF_MAKE_SELECTOR(parent, name) MakeSelector(tree.get(), parent, name);

#define IYF_RUN_TREE_TEST(tree, name, maxReturnsToRoot) \
    if (TestResults results = runTree(tree.get(), maxReturnsToRoot); !results.isSuccessful()) {\
        return results;\
    } else {\
        if (expectedReportVector != reportVector) {\
            std::stringstream ss;\
            ss << name " failed";\
            ss << "\n\tEXPECTED: ";\
            ss << printReportVector(expectedReportVector);\
            ss << "\n\tGOT:";\
            ss << printReportVector(reportVector);\
            \
            return TestResults(false, ss.str());\
        }\
        \
        reportVector.clear();\
    }\
    tree->resetStepCounter();

    

std::unique_ptr<iyf::BehaviourTree> BehaviourTreeTests::makeTestTree(iyf::Blackboard* bb, std::vector<ProgressReport>* reportVector, const TestTaskResults& results, BehaviourTreeTestStage stage) {
    std::unique_ptr<iyf::BehaviourTree> tree = std::make_unique<iyf::BehaviourTree>(bb, IYF_VERBOSE_TREE_OUTPUT);
    
    auto mainSelector = IYF_MAKE_SELECTOR(tree->getRoot(), "Main Sequence");
    
    auto useTheDoor = IYF_MAKE_SEQUENCE(mainSelector, "Use the Door Sequence");
    IYF_MAKE_REPORTING_TASK(useTheDoor, "Go Towards the Door", ReportID::GoingTowardsTheDoor, true, 1);
    auto openDoor = IYF_MAKE_SELECTOR(useTheDoor, "Open Door");
    auto useKey = IYF_MAKE_REPORTING_TASK(openDoor, "Use Key", ReportID::UsingKey, &results.canUseKey, 0);
    IYF_MAKE_REPORTING_TASK(openDoor, "Look For Spare", ReportID::LookingForSpare, &results.spareExists, 1);
    IYF_MAKE_REPORTING_TASK(useTheDoor, "Enter through the Door", ReportID::EnteringThroughTheDoor, true, 0);
    
    auto useTheWindow = IYF_MAKE_SEQUENCE(mainSelector, "Use Window Sequence");
    IYF_MAKE_REPORTING_TASK(useTheWindow, "Go Towards the Window", ReportID::GoingTowardsTheWindow, true, 1);
    auto openWindow = IYF_MAKE_SELECTOR(useTheWindow, "Open Window");
    IYF_MAKE_REPORTING_TASK(openWindow, "Gently Open the Window", ReportID::OpeningWindowGently, &results.windowNotLocked, 0);
    
    // Extra duration is needed to check if the tasks are correctly interupted
    IYF_MAKE_REPORTING_TASK(openWindow, "Break Window", ReportID::BreakingWindow, &results.windowBreakable, (stage == BehaviourTreeTestStage::NonDecorated) ? 1 : 2);
    IYF_MAKE_REPORTING_TASK(openWindow, "Banshee Scream at Window", ReportID::BansheeScreamingAtWindow, &results.canBansheeScream, 1);
    IYF_MAKE_REPORTING_TASK(useTheWindow, "Enter through the Window", ReportID::EnteringThroughTheWindow, true, 0);
    
    if (stage == BehaviourTreeTestStage::DecoratedAbortOwn) {
        ServiceReportIDs onlookerCheckIDs(ReportID::CheckingForOnlookers, ReportID::OnlookersLeft, ReportID::OnlookersDetected);
        
        auto checkForOnlookers = tree->addNode<TimedTriggerService>(useTheWindow, 0.4f, onlookerCheckIDs, reportVector, ONLOOKERS_PRESENT_HASH, false, true);
        checkForOnlookers->setName("Check for onlookers");
        checkForOnlookers->setTiming(0.1f, 0.0f, true);
        checkForOnlookers->setExecuteUpdateOnArrival(true);
        checkForOnlookers->setRestartTimerOnArrival(false);
        
        auto stopBreakingIn = tree->addNode<iyf::CompareValuesDecoratorNode>(openWindow, std::vector<iyf::StringHash>{ONLOOKERS_PRESENT_HASH, TRUE_VALUE_HASH}, ValueCompareOperation::NotEqual, AbortMode::OwnSubtree);
        stopBreakingIn->setName("Stop breaking in into own house");
    } else if (stage == BehaviourTreeTestStage::DecoratedAbortLowerPriority) {//DecoratedAbortLowerPriorityBlackboardCompare
        ServiceReportIDs thinkAboutKeyIDs(ReportID::ThinkingAboutAKey, ReportID::ERROR, ReportID::KeyFound);
        
        auto thinkAboutAKey = tree->addNode<TimedTriggerService>(mainSelector, 0.9f, thinkAboutKeyIDs, reportVector, KEY_IN_POCKET_HASH, false, true);
        thinkAboutAKey->setName("Think about a key");
        thinkAboutAKey->setTiming(0.2f, 0.0f, true);
//         thinkAboutAKey->setExecuteUpdateOnArrival(true);
        thinkAboutAKey->setRestartTimerOnArrival(false);
        
        auto canUnlock = tree->addNode<iyf::CompareValueConstantDecoratorNode>(useKey, KEY_IN_POCKET_HASH, true, ValueCompareOperation::Equal, AbortMode::LowerPriority);
        canUnlock->setName("Can unlock");
    }
    
    return tree;
}

BehaviourTreeTests::BehaviourTreeTests(bool verbose) : iyf::test::TestBase(verbose) {}
BehaviourTreeTests::~BehaviourTreeTests() {}

void BehaviourTreeTests::initialize() {}

TestResults BehaviourTreeTests::run() {
    iyf::BlackboardInitializer bbi;
    bbi.name = "HouseEntryBlackboard";
    bbi.initialValues.reserve(3);
    bbi.initialValues.emplace_back(ONLOOKERS_PRESENT, false);
    bbi.initialValues.emplace_back(KEY_IN_POCKET, false);
    bbi.initialValues.emplace_back(TRUE_VALUE, true);
    
    TestTaskResults ttr;
    iyf::Blackboard bb(bbi);
    std::vector<ProgressReport> reportVector;
    
    // ----- BEGIN NON DECORATED TREE TESTS
    std::unique_ptr<iyf::BehaviourTree> nonDecoratedTree = makeTestTree(&bb, &reportVector, ttr, BehaviourTreeTestStage::NonDecorated);
    nonDecoratedTree->buildTree();
//     nonDecoratedTree->setLoggingEnabled(true);
    
    if (isOutputVerbose()) {
        LOG_V("Non decorated behaviour tree:\n" << nonDecoratedTree->toString());
    }
    
    // TEST Non decorated #1 (everything OK from start)
    std::vector<ProgressReport> expectedReportVector = {
        ProgressReport(ReportID::GoingTowardsTheDoor,    BehaviourTreeResult::Running, 0),
        ProgressReport(ReportID::GoingTowardsTheDoor,    BehaviourTreeResult::Success, 1),
        ProgressReport(ReportID::UsingKey,               BehaviourTreeResult::Success, 2),
        ProgressReport(ReportID::EnteringThroughTheDoor, BehaviourTreeResult::Success, 3),
    };
    
    IYF_RUN_TREE_TEST(nonDecoratedTree, "Non decorated #1", 1);
    
    // TEST Non decorated #2 (forgotten keys, checks if selector continues after the first fail)
    ttr.canUseKey = false;
    
    expectedReportVector = {
        ProgressReport(ReportID::GoingTowardsTheDoor,    BehaviourTreeResult::Running, 0),
        ProgressReport(ReportID::GoingTowardsTheDoor,    BehaviourTreeResult::Success, 1),
        ProgressReport(ReportID::UsingKey,               BehaviourTreeResult::Failure, 2),
        ProgressReport(ReportID::LookingForSpare,        BehaviourTreeResult::Running, 3),
        ProgressReport(ReportID::LookingForSpare,        BehaviourTreeResult::Success, 4),
        ProgressReport(ReportID::EnteringThroughTheDoor, BehaviourTreeResult::Success, 5),
    };
    
    IYF_RUN_TREE_TEST(nonDecoratedTree, "Non decorated #2", 1);
    
     // TEST Non decorated #3 (spare key not found, checks if selector fails after all tasks fail and if the parent sequence fails as well)
    ttr.spareExists = false;
    
    expectedReportVector = {
        ProgressReport(ReportID::GoingTowardsTheDoor,      BehaviourTreeResult::Running, 0),
        ProgressReport(ReportID::GoingTowardsTheDoor,      BehaviourTreeResult::Success, 1),
        ProgressReport(ReportID::UsingKey,                 BehaviourTreeResult::Failure, 2),
        ProgressReport(ReportID::LookingForSpare,          BehaviourTreeResult::Running, 3),
        ProgressReport(ReportID::LookingForSpare,          BehaviourTreeResult::Failure, 4),
        ProgressReport(ReportID::GoingTowardsTheWindow,    BehaviourTreeResult::Running, 5),
        ProgressReport(ReportID::GoingTowardsTheWindow,    BehaviourTreeResult::Success, 6),
        ProgressReport(ReportID::OpeningWindowGently,      BehaviourTreeResult::Success, 7),
        ProgressReport(ReportID::EnteringThroughTheWindow, BehaviourTreeResult::Success, 8),
    };
    
    IYF_RUN_TREE_TEST(nonDecoratedTree, "Non decorated #3", 1);
    
    // TEST Non decorated #4 (Nothing works, checks if root makes us retry when all actions fail)
    ttr.windowNotLocked = false;
    ttr.windowBreakable = false;
    ttr.canBansheeScream = false;
    
    expectedReportVector = {
        ProgressReport(ReportID::GoingTowardsTheDoor,      BehaviourTreeResult::Running, 0),
        ProgressReport(ReportID::GoingTowardsTheDoor,      BehaviourTreeResult::Success, 1),
        ProgressReport(ReportID::UsingKey,                 BehaviourTreeResult::Failure, 2),
        ProgressReport(ReportID::LookingForSpare,          BehaviourTreeResult::Running, 3),
        ProgressReport(ReportID::LookingForSpare,          BehaviourTreeResult::Failure, 4),
        ProgressReport(ReportID::GoingTowardsTheWindow,    BehaviourTreeResult::Running, 5),
        ProgressReport(ReportID::GoingTowardsTheWindow,    BehaviourTreeResult::Success, 6),
        ProgressReport(ReportID::OpeningWindowGently,      BehaviourTreeResult::Failure, 7),
        ProgressReport(ReportID::BreakingWindow,           BehaviourTreeResult::Running, 8),
        ProgressReport(ReportID::BreakingWindow,           BehaviourTreeResult::Failure, 9),
        ProgressReport(ReportID::BansheeScreamingAtWindow, BehaviourTreeResult::Running, 10),
        ProgressReport(ReportID::BansheeScreamingAtWindow, BehaviourTreeResult::Failure, 11),
    };
    
    const std::size_t size = expectedReportVector.size();
    expectedReportVector.reserve(size * 2);
    std::copy_n(expectedReportVector.begin(), size, std::back_inserter(expectedReportVector));
    
    for (std::size_t i = 12; i < expectedReportVector.size(); ++i) {
        // WARNING: i + 1 is NOT a bug. The tree pauses if it gets back to root. This is done to prevent infinite loops.
        expectedReportVector[i].step = i + 1;
    }
    
    IYF_RUN_TREE_TEST(nonDecoratedTree, "Non decorated #4", 2);
    
    // ----- END NON DECORATED TREE TESTS
    nonDecoratedTree = nullptr;
    
    // ----- BEGIN DECORATED TREE TESTS
    std::unique_ptr<iyf::BehaviourTree> decoratedTree = makeTestTree(&bb, &reportVector, ttr, BehaviourTreeTestStage::DecoratedAbortOwn);
    decoratedTree->buildTree();
    //decoratedTree->setLoggingEnabled(true);
    
    if (isOutputVerbose()) {
        LOG_V("Decorated behaviour tree:\n" << decoratedTree->toString());
    }
    
    // TEST Decorated #1 (Nothing works, we spot onlookers using a service and have to quit trying to break into own home (decorator aborts own subtree), onlookers stay)
    expectedReportVector = {
        ProgressReport(ReportID::GoingTowardsTheDoor,   BehaviourTreeResult::Running, 0),
        ProgressReport(ReportID::GoingTowardsTheDoor,   BehaviourTreeResult::Success, 1),
        ProgressReport(ReportID::UsingKey,              BehaviourTreeResult::Failure, 2),
        ProgressReport(ReportID::LookingForSpare,       BehaviourTreeResult::Running, 3),
        ProgressReport(ReportID::LookingForSpare,       BehaviourTreeResult::Failure, 4),
        ProgressReport(ReportID::CheckingForOnlookers,  BehaviourTreeResult::Running, 5),
        ProgressReport(ReportID::GoingTowardsTheWindow, BehaviourTreeResult::Running, 5),
        ProgressReport(ReportID::CheckingForOnlookers,  BehaviourTreeResult::Running, 6),
        ProgressReport(ReportID::GoingTowardsTheWindow, BehaviourTreeResult::Success, 6),
        ProgressReport(ReportID::CheckingForOnlookers,  BehaviourTreeResult::Running, 7),
        ProgressReport(ReportID::OpeningWindowGently,   BehaviourTreeResult::Failure, 7),
        ProgressReport(ReportID::CheckingForOnlookers,  BehaviourTreeResult::Running, 8),
        ProgressReport(ReportID::BreakingWindow,        BehaviourTreeResult::Running, 8),
        ProgressReport(ReportID::OnlookersDetected,     BehaviourTreeResult::Running, 9),
        ProgressReport(ReportID::GoingTowardsTheDoor,   BehaviourTreeResult::Running, 10),
        ProgressReport(ReportID::GoingTowardsTheDoor,   BehaviourTreeResult::Success, 11),
        ProgressReport(ReportID::UsingKey,              BehaviourTreeResult::Failure, 12),
        ProgressReport(ReportID::LookingForSpare,       BehaviourTreeResult::Running, 13),
        ProgressReport(ReportID::LookingForSpare,       BehaviourTreeResult::Failure, 14),
        ProgressReport(ReportID::CheckingForOnlookers,  BehaviourTreeResult::Running, 15),
        ProgressReport(ReportID::GoingTowardsTheWindow, BehaviourTreeResult::Running, 15),
        ProgressReport(ReportID::CheckingForOnlookers,  BehaviourTreeResult::Running, 16),
        ProgressReport(ReportID::GoingTowardsTheWindow, BehaviourTreeResult::Success, 16),
        ProgressReport(ReportID::CheckingForOnlookers,  BehaviourTreeResult::Running, 17),
    };
    IYF_RUN_TREE_TEST(decoratedTree, "Decorated #1", 2);
    
    // TEST Decorated #2 (Key works, but it's blocked by a decorator. We remember having a key in a different pocket using a service and quit trying to break into own home
    // (decorator aborts lower prioriy subtree))
    ttr.canUseKey = true;
    
    decoratedTree = makeTestTree(&bb, &reportVector, ttr, BehaviourTreeTestStage::DecoratedAbortLowerPriority);
    decoratedTree->buildTree();
//     decoratedTree->setLoggingEnabled(true);
    
    if (isOutputVerbose()) {
        LOG_V("Decorated behaviour tree:\n" << decoratedTree->toString());
    }
    
    expectedReportVector = {
        ProgressReport(ReportID::GoingTowardsTheDoor,    BehaviourTreeResult::Running, 0),
        ProgressReport(ReportID::GoingTowardsTheDoor,    BehaviourTreeResult::Success, 1),
        ProgressReport(ReportID::ThinkingAboutAKey,      BehaviourTreeResult::Running, 2),
        ProgressReport(ReportID::LookingForSpare,        BehaviourTreeResult::Running, 3),
        ProgressReport(ReportID::ThinkingAboutAKey,      BehaviourTreeResult::Running, 4),
        ProgressReport(ReportID::LookingForSpare,        BehaviourTreeResult::Failure, 4),
        ProgressReport(ReportID::GoingTowardsTheWindow,  BehaviourTreeResult::Running, 5),
        ProgressReport(ReportID::ThinkingAboutAKey,      BehaviourTreeResult::Running, 6),
        ProgressReport(ReportID::GoingTowardsTheWindow,  BehaviourTreeResult::Success, 6),
        ProgressReport(ReportID::OpeningWindowGently,    BehaviourTreeResult::Failure, 7),
        ProgressReport(ReportID::ThinkingAboutAKey,      BehaviourTreeResult::Running, 8),
        ProgressReport(ReportID::BreakingWindow,         BehaviourTreeResult::Running, 8),
        ProgressReport(ReportID::BreakingWindow,         BehaviourTreeResult::Running, 9),
        ProgressReport(ReportID::KeyFound,               BehaviourTreeResult::Running, 10),
        ProgressReport(ReportID::GoingTowardsTheDoor,    BehaviourTreeResult::Running, 11),
        ProgressReport(ReportID::GoingTowardsTheDoor,    BehaviourTreeResult::Success, 12),
        ProgressReport(ReportID::ThinkingAboutAKey,      BehaviourTreeResult::Running, 13),
        ProgressReport(ReportID::UsingKey,               BehaviourTreeResult::Success, 13),
        ProgressReport(ReportID::EnteringThroughTheDoor, BehaviourTreeResult::Success, 14),
        ProgressReport(ReportID::ThinkingAboutAKey,      BehaviourTreeResult::Running, 15),
    };
    IYF_RUN_TREE_TEST(decoratedTree, "Decorated #2", 2);
    
    // ----- END DECORATED TREE TESTS
    decoratedTree = nullptr;
    
    return TestResults(true, "");
}

void BehaviourTreeTests::cleanup() {}

TestResults BehaviourTreeTests::runTree(iyf::BehaviourTree* tree, std::size_t maxReturnsToRoot, std::size_t maxSteps) {
    std::size_t currentStep = 0;
    std::size_t returnsToRoot = 0;
    
    while (returnsToRoot < maxReturnsToRoot && currentStep < maxSteps) {
        tree->update(IYF_TEST_EMULATED_UPDATE_INTERVAL);
        currentStep++;
        
        if (tree->returnedToRoot()) {
            returnsToRoot++;
        }
    }
    
    if (currentStep >= maxSteps) {
        return TestResults(false, "The test failed to complete in a reasonable amount of time");
    } else {
        return TestResults(true, "");
    }
}

std::string BehaviourTreeTests::printReportVector(const std::vector<ProgressReport>& reportVector) const {
    std::stringstream ss;
    ss << "\n\tN is number of the record in the vector\n\tS is the number of the tree step";
    for (std::size_t i = 0; i < reportVector.size(); ++i) {
        const ProgressReport& pr = reportVector[i];
        ss << "\n\tN: " << std::setw(3) << i << "; S: " << std::setw(3) << pr.step << "; " << ReportIDName(pr.id) << " " << ResultName(pr.result);
    }
    
    return ss.str();
}

}
