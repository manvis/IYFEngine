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

#ifndef IYF_BEHAVIOUR_TREE_TESTS_HPP
#define IYF_BEHAVIOUR_TREE_TESTS_HPP

#include "TestBase.hpp"
#include "ai/BehaviourTreeConstants.hpp"

#include <vector>

namespace iyf {
class BehaviourTree;
class Blackboard;

namespace test {
struct TestTaskResults;

enum class ReportID {
    SulkingAndWaiting,
    GoingTowardsTheDoor,
    UsingKey,
    LookingForSpare,
    EnteringThroughTheDoor,
    GoingTowardsTheWindow,
    OpeningWindowGently,
    BreakingWindow,
    BansheeScreamingAtWindow,
    EnteringThroughTheWindow,
    CheckingForOnlookers,
    OnlookersDetected,
    OnlookersLeft
};

enum class BehaviourTreeTestStage {
    NonDecorated, ///< Tasks, Selectors and Sequences
    DecoratedAbortOwn, ///< Same as Tasks + Services and a decorator that aborts own subtree
    DecoratedAbortLowerPriority, ///< Same as Tasks + Services and a decorator that aborts lower priority subtrees
    // SimpleParallel // We don't have parallel nodes at the moment
};

struct ProgressReport {
    inline ProgressReport(ReportID id, iyf::BehaviourTreeResult result) : id(id), result(result) {}
    
    ReportID id;
    iyf::BehaviourTreeResult result;
    
    inline friend bool operator==(const ProgressReport& a, const ProgressReport& b) {
        return (a.id == b.id) && (a.result == b.result);
    }
};


class BehaviourTreeTests : public TestBase {
public:
    BehaviourTreeTests(bool verbose);
    virtual ~BehaviourTreeTests();
    
    virtual std::string getName() const final override {
        return "Behaviour tree tests";
    }
    
    virtual void initialize() final override;
    virtual TestResults run() final override;
    virtual void cleanup() final override;
private:
    std::unique_ptr<iyf::BehaviourTree> makeTestTree(iyf::Blackboard* bb, std::vector<ProgressReport>* reportVector, const TestTaskResults& results, BehaviourTreeTestStage stage);
    std::string printReportVector(const std::vector<ProgressReport>& reportVector) const;
    
    TestResults runTree(iyf::BehaviourTree* tree, std::size_t maxReturnsToRoot = 1, std::size_t maxSteps = 100);
};

}
}

#endif // IYF_BEHAVIOUR_TREE_TESTS_HPP

