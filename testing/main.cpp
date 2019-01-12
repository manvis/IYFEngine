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

#include <memory>
#include <vector>
#include <chrono>
#include <iostream>

#include "core/Engine.hpp"
#include "core/Logger.hpp"
#include "TestState.hpp"
#include "TestBase.hpp"
#include "FileMonitorTests.hpp"
#include "MemorySerializerTests.hpp"
#include "CSVParserTests.hpp"
#include "BehaviourTreeTests.hpp"

//#include "did/InitState.h"

// If true, some tests will output additional debug data
#define VERBOSE_TESTS true

#define ADD_TESTS(x) runner.addTest(std::make_unique<test::x>(VERBOSE_TESTS));

using namespace iyf;

class TestRunner {
public:
    void addTest(std::unique_ptr<test::TestBase> test) {
        tests.push_back(std::move(test));
    }
    
    void runTests() {
        std::size_t count = 0;
        
        for (const auto& t : tests) {
            LOG_V("TEST STARTING\n\t\tName:     " << t->getName());
            
            t->initialize();
            
            auto start = std::chrono::high_resolution_clock::now();
            auto results = t->run();
            auto end = std::chrono::high_resolution_clock::now();
            
            std::chrono::duration<float, std::milli> durationMS = end - start;
            
            LOG_V("TEST FINISHED\n\t\t" << 
                  "Name:     " << t->getName() << "\n\t\t" <<
                  "Result:   " << (results.isSuccessful() ? "SUCCESS" : "FAILURE") << "\n\t\t" << 
                  "Notes:    " << (results.getNotes().empty() ? "NONE" : results.getNotes()) << "\n\t\t" << 
                  "Duration: " << durationMS.count() << "ms");
            
            t->cleanup();
            
            count++;
        }
        
        LOG_V("Tests completed");
    }
private:
    std::vector<std::unique_ptr<test::TestBase>> tests;
};

TestRunner runner;

// WARNING keep this signature of main or compilation WILL FAIL. This is caused by SDL
int main(int, char*[]) {
    // TODO use a special GameState called TestState to perfoem tests that require the Engine to be running
//     // Create an instance of the game engine and set up timing variables
//     std::unique_ptr<Engine> gameEngine(new Engine(argv[0]));
//     // Set the first state
//     //gameEngine.changeState(wgg::GameplayState::instance());
//     gameEngine->pushState(std::make_unique<test::TestState>(gameEngine.get()));
//     gameEngine->executeMainLoop();
//     //gameEngine.changeState(did::InitState::instance());
//     //gameEngine.changeState(GameInitState::instance());
    
//     ADD_TESTS(FileMonitorTests)
    ADD_TESTS(MemorySerializerTests)
//    ADD_TESTS(CSVParserTests)
//     ADD_TESTS(BehaviourTreeTests)
    
    runner.runTests();
    
    return 0;
}
