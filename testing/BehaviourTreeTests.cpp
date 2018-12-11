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
    TestTaskResults() : hasKey(true), spareExists(true), windowNotLocked(true), windowBreakable(true), canBansheeScream(true) {}
    bool hasKey;
    bool spareExists;
    bool windowNotLocked;
    bool windowBreakable;
    bool canBansheeScream;
};

class ProgressReportingTask : public iyf::TaskNode {
public:
    ProgressReportingTask(BehaviourTree* tree, ReportID reportID, std::vector<ProgressReport>* reports, std::size_t tickDelay, bool succeed)
        : TaskNode(tree), reports(reports), remainingTicks(tickDelay), tickDelay(tickDelay), succeedPtr(nullptr), reportID(reportID), succeed(succeed) {
            LOG_D("BOOL-C")
        }
    
    ProgressReportingTask(BehaviourTree* tree, ReportID reportID, std::vector<ProgressReport>* reports, std::size_t tickDelay, const bool* succeedPtr)
        : TaskNode(tree), reports(reports), remainingTicks(tickDelay), tickDelay(tickDelay), succeedPtr(succeedPtr), reportID(reportID), succeed(false) {
            LOG_D("BOOL-P")
        }
    
    virtual void onArriveFromParent() override {
        remainingTicks = tickDelay;
    }
    
    virtual BehaviourResultNextNodePair update() override {
        if (remainingTicks > 0) {
            reports->emplace_back(reportID, BehaviourTreeResult::Running);
            remainingTicks--;
            
            return {BehaviourTreeResult::Running, this};
        } else {
            const BehaviourTreeResult result = shouldSucceed() ? BehaviourTreeResult::Success : BehaviourTreeResult::Failure;
            
            reports->emplace_back(reportID, result);
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

#define IYF_RUN_TREE_TEST(name, maxReturnsToRoot) \
    if (TestResults results = runTree(nonDecoratedTree.get(), maxReturnsToRoot); !results.isSuccessful()) {\
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
    }

    

std::unique_ptr<iyf::BehaviourTree> BehaviourTreeTests::makeNonDecoratedTree(iyf::Blackboard* bb, std::vector<ProgressReport>* reportVector, const TestTaskResults& results) {
    std::unique_ptr<iyf::BehaviourTree> tree = std::make_unique<iyf::BehaviourTree>(bb, IYF_VERBOSE_TREE_OUTPUT);
    
    auto mainSelector = IYF_MAKE_SELECTOR(tree->getRoot(), "Main Sequence");
    
    auto useTheDoor = IYF_MAKE_SEQUENCE(mainSelector, "Use the Door Sequence");
    IYF_MAKE_REPORTING_TASK(useTheDoor, "Go Towards the Door", ReportID::GoingTowardsTheDoor, true, 1);
    auto openDoor = IYF_MAKE_SELECTOR(useTheDoor, "Open Door");
    IYF_MAKE_REPORTING_TASK(openDoor, "Use Key", ReportID::UsingKey, &results.hasKey, 0);
    IYF_MAKE_REPORTING_TASK(openDoor, "Look For Spare", ReportID::LookingForSpare, &results.spareExists, 1);
    IYF_MAKE_REPORTING_TASK(useTheDoor, "Enter through the Door", ReportID::EnteringThroughTheDoor, true, 0);
    
    auto useTheWindow = IYF_MAKE_SEQUENCE(mainSelector, "Use Window Sequence");
    IYF_MAKE_REPORTING_TASK(useTheWindow, "Go Towards the Window", ReportID::GoingTowardsTheWindow, true, 1);
    auto openWindow = IYF_MAKE_SELECTOR(useTheWindow, "Open Window");
    IYF_MAKE_REPORTING_TASK(openWindow, "Gently Open the Window", ReportID::OpeningWindowGently, &results.windowNotLocked, 0);
    IYF_MAKE_REPORTING_TASK(openWindow, "Break Window", ReportID::BreakingWindow, &results.windowBreakable, 1);
    IYF_MAKE_REPORTING_TASK(openWindow, "Banshee Scream at Window", ReportID::BansheeScreamingAtWindow, &results.canBansheeScream, 1);
    IYF_MAKE_REPORTING_TASK(useTheWindow, "Enter through the Window", ReportID::EnteringThroughTheWindow, true, 0);
    
    // TODO Sulk if all else fails
    //IYF_MAKE_REPORTING_TASK(mainSelector, "Sulk and Wait for Locksmith", ReportID::SulkAndWait, true, 0);
    
    return tree;
}

BehaviourTreeTests::BehaviourTreeTests(bool verbose) : iyf::test::TestBase(verbose) {}
BehaviourTreeTests::~BehaviourTreeTests() {}

void BehaviourTreeTests::initialize() {}

TestResults BehaviourTreeTests::run() {
    iyf::BlackboardInitializer bbi;
    bbi.name = "HouseEntryBlackboard";
    bbi.initialValues.reserve(3);
    bbi.initialValues.emplace_back("a", true);
    
    TestTaskResults ttr;
    iyf::Blackboard bb(bbi);
    std::vector<ProgressReport> reportVector;
    
    std::unique_ptr<iyf::BehaviourTree> nonDecoratedTree = makeNonDecoratedTree(&bb, &reportVector, ttr);
    nonDecoratedTree->buildTree();
    
    if (isOutputVerbose()) {
        LOG_V("Non decorated behaviour tree:\n" << nonDecoratedTree->toString());
    }
    
    // TEST Non decorated #1 (everything OK from start)
    std::vector<ProgressReport> expectedReportVector = {
        ProgressReport(ReportID::GoingTowardsTheDoor, BehaviourTreeResult::Running),
        ProgressReport(ReportID::GoingTowardsTheDoor, BehaviourTreeResult::Success),
        ProgressReport(ReportID::UsingKey, BehaviourTreeResult::Success),
        ProgressReport(ReportID::EnteringThroughTheDoor, BehaviourTreeResult::Success),
    };
    
    IYF_RUN_TREE_TEST("Non decorated #1", 1);
    
    // TEST Non decorated #2 (forgotten keys, checks if selector continues after the first fail)
    ttr.hasKey = false;
    
    expectedReportVector = {
        ProgressReport(ReportID::GoingTowardsTheDoor, BehaviourTreeResult::Running),
        ProgressReport(ReportID::GoingTowardsTheDoor, BehaviourTreeResult::Success),
        ProgressReport(ReportID::UsingKey, BehaviourTreeResult::Failure),
        ProgressReport(ReportID::LookingForSpare, BehaviourTreeResult::Running),
        ProgressReport(ReportID::LookingForSpare, BehaviourTreeResult::Success),
        ProgressReport(ReportID::EnteringThroughTheDoor, BehaviourTreeResult::Success),
    };
    
    IYF_RUN_TREE_TEST("Non decorated #2", 1);
    
     // TEST Non decorated #3 (spare key not found, checks if selector fails after all tasks fail and if the parent sequence fails as well)
    ttr.spareExists = false;
    
    expectedReportVector = {
        ProgressReport(ReportID::GoingTowardsTheDoor, BehaviourTreeResult::Running),
        ProgressReport(ReportID::GoingTowardsTheDoor, BehaviourTreeResult::Success),
        ProgressReport(ReportID::UsingKey, BehaviourTreeResult::Failure),
        ProgressReport(ReportID::LookingForSpare, BehaviourTreeResult::Running),
        ProgressReport(ReportID::LookingForSpare, BehaviourTreeResult::Failure),
        ProgressReport(ReportID::GoingTowardsTheWindow, BehaviourTreeResult::Running),
        ProgressReport(ReportID::GoingTowardsTheWindow, BehaviourTreeResult::Success),
        ProgressReport(ReportID::OpeningWindowGently, BehaviourTreeResult::Success),
        ProgressReport(ReportID::EnteringThroughTheWindow, BehaviourTreeResult::Success),
    };
    
    IYF_RUN_TREE_TEST("Non decorated #3", 1);
    
     // TEST Non decorated #4 (Nothing works, checks if root makes us retry when all actions fail)
    ttr.windowNotLocked = false;
    ttr.windowBreakable = false;
    ttr.canBansheeScream = false;
    
    expectedReportVector = {
        ProgressReport(ReportID::GoingTowardsTheDoor, BehaviourTreeResult::Running),
        ProgressReport(ReportID::GoingTowardsTheDoor, BehaviourTreeResult::Success),
        ProgressReport(ReportID::UsingKey, BehaviourTreeResult::Failure),
        ProgressReport(ReportID::LookingForSpare, BehaviourTreeResult::Running),
        ProgressReport(ReportID::LookingForSpare, BehaviourTreeResult::Failure),
        ProgressReport(ReportID::GoingTowardsTheWindow, BehaviourTreeResult::Running),
        ProgressReport(ReportID::GoingTowardsTheWindow, BehaviourTreeResult::Success),
        ProgressReport(ReportID::OpeningWindowGently, BehaviourTreeResult::Failure),
        ProgressReport(ReportID::BreakingWindow, BehaviourTreeResult::Running),
        ProgressReport(ReportID::BreakingWindow, BehaviourTreeResult::Failure),
        ProgressReport(ReportID::BansheeScreamingAtWindow, BehaviourTreeResult::Running),
        ProgressReport(ReportID::BansheeScreamingAtWindow, BehaviourTreeResult::Failure),
    };
    
    const std::size_t size = expectedReportVector.size();
    expectedReportVector.reserve(size * 2);
    std::copy_n(expectedReportVector.begin(), size, std::back_inserter(expectedReportVector));
    
    IYF_RUN_TREE_TEST("Non decorated #4", 2);
    
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
    for (std::size_t i = 0; i < reportVector.size(); ++i) {
        const ProgressReport& pr = reportVector[i];
        ss << "\n\t" << std::setw(6) << i << ": " << ReportIDName(pr.id) << " " << ResultName(pr.result);
    }
    
    return ss.str();
}

}
