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

#include "ConfigurationTests.hpp"
#include "configuration/Configuration.hpp"
#include "io/DefaultFileSystem.hpp"

#include <sstream>
#include <fstream>

#include <filesystem>
namespace fs = std::filesystem;

namespace iyf::test {
struct ExpectedConfigTestValues {
    std::vector<std::pair<Path, ConfigurationFile::ParseResult>> parseResults;
    ConfigurationFile configFilePreAdd;
    ConfigurationFile configFilePostAdd;
};

const Path ConfigurationTests::configFileDirName = "confTest";
const Path ConfigurationTests::configFileName = "system.conf";
const Path ConfigurationTests::userConfigFileName = "user.conf";
const std::size_t ConfigurationTests::expectedValueCount = 13;

ConfigurationTests::ConfigurationTests(bool verbose) : iyf::test::TestBase(verbose) {}
ConfigurationTests::~ConfigurationTests() {}

void ConfigurationTests::initialize() {
    configFileContents =
        "core.test01 = \"whoAMI?\"\n"           //  0
        "core.test02 = whoA\"REU?\n"            //  1
        "core.test03 = whoAREwe?\"\n"           //  2
        " =2\n"                                 //  3
        "a=2\n"                                 //  4
        "a = 51619991551500\n"                  //  5
        "// Comment\n"                          //  6
        "engine.test01 = \"string_par\"am\"\n"  //  7
        ".test02 = 64\n"                        //  8
        "geb.=\"aklm\"\n"                       //  9
        "dodo = 128\n"                          // 10
        "dodv = 128.1564\n"                     // 11
        "dodsp = 128.1564 \n"                   // 12
        "zimpl = true \n"                       // 13
        "pimpl = true\n"                        // 14
        "hub = false\n"                         // 15
        ".=5\n"                                 // 16
        "a= \n"                                 // 17
        "šim.klm = 916616\n"                    // 18
        "wi.ld =    \"dogs\"   \n"              // 19
        "cha.os =    \"\"\n"                    // 20
        "grump.\n"                              // 21
        "//editor, gfx, sound, ctrl\n"          // 22
        "vem.Ūči= 99";                          // 23
    
    userConfigFileContents =
        "hum.d =\"dum\"\n"          // 0
        "//Comment line\n"          // 1
        ".dodo =\"now-a-string\"\n" // 2
        ".zimpl = false";           // 3
    
    std::error_code ec;
    destinationPath = fs::current_path(ec);
    if (ec) {
        throw std::runtime_error("Fatal test error. Failed to retrieve the current path.");
    } else if (isOutputVerbose()) {
        LOG_V("Current directory is {}", destinationPath);
    }
    
    destinationPath /= configFileDirName;
    
    const bool destinationExists = fs::exists(destinationPath.getNativeString(), ec);
    if (ec) {
        throw std::runtime_error("Fatal test error. Failed to determine if the test destination exists.");
    }
    
    if (destinationExists) {
        if (isOutputVerbose()) {
            LOG_V("Test destination directory {} already exists. Deleting and recreating it.", destinationPath);
        }
        
        fs::remove_all(destinationPath.getNativeString(), ec);
        if (ec) {
            throw std::runtime_error("Fatal test error. Failed to delete old test directory.");
        }
    }
    
    const bool created = fs::create_directory(destinationPath.getNativeString(), ec);
    if (ec || !created) {
        throw std::runtime_error("Fatal test error. Failed to create the test directory.");
    } else if (isOutputVerbose()) {
        LOG_V("Successfully created a new test destination directory {}", destinationPath);
    }
    
    configFilePath = destinationPath;
    configFilePath /= configFileName;
    
    std::ofstream outConfig(configFilePath.getNativeString(), std::ios::out | std::ios::binary);
    outConfig << configFileContents;
    outConfig.close();
    
    userConfigFilePath = destinationPath;
    userConfigFilePath /= userConfigFileName;
    
    std::ofstream outUserConfig(userConfigFilePath.getNativeString(), std::ios::out | std::ios::binary);
    outUserConfig << userConfigFileContents;
    outUserConfig.close();
    
    ConfigurationFile::ParseResult configFileParseResults;
    configFileParseResults.lineCount = 24;
    configFileParseResults.linesWithErrors = {
        {1, ConfigurationFileError::InvalidStringParameter},
        {2, ConfigurationFileError::InvalidStringParameter},
        {3, ConfigurationFileError::NoIdentifier},
        {9, ConfigurationFileError::NoName},
        {16, ConfigurationFileError::NoName},
        {17, ConfigurationFileError::NoValue},
        {18, ConfigurationFileError::NamespaceNotAlphanumericASCII},
        {21, ConfigurationFileError::InvalidLineSyntax},
        {23, ConfigurationFileError::NameNotAlphanumericASCII},
    };
    
    ConfigurationFile::ParseResult userConfigFileParseResults;
    userConfigFileParseResults.lineCount = 4;
    
    expectedValues = new ExpectedConfigTestValues;
    expectedValues->parseResults.emplace_back(std::make_pair(configFilePath, std::move(configFileParseResults)));
    expectedValues->parseResults.emplace_back(std::make_pair(userConfigFilePath, std::move(userConfigFileParseResults)));
    
    expectedValues->configFilePreAdd.lines = {
        ConfigurationValue(std::string("dum"), "d", "hum", 0),
        NonConfigLine("//Comment line"),
        ConfigurationValue(std::string("now-a-string"), "dodo", "", 2),
        ConfigurationValue(false, "zimpl", "", 3),
    };
    
    expectedValues->configFilePostAdd.lines = expectedValues->configFilePreAdd.lines;
    expectedValues->configFilePostAdd.lines[0] = ConfigurationValue(std::string("glowing"), "d", "hum", 0);
    expectedValues->configFilePostAdd.lines.emplace_back(ConfigurationValue(256l, "ned", "", 4));
    expectedValues->configFilePostAdd.lines.emplace_back(ConfigurationValue(591.1614846, "dodsp", "", 5));
}

#define IYF_TEST_VALUE(actual, expected) \
    if (actual != expected) { \
        return TestResults(false, fmt::format("Incorrect value {}.\n\t\t\tExpected: {}\n\t\t\tGot: {}", #actual, expected, actual));\
    }

TestResults ConfigurationTests::run() {
    const std::vector<ConfigurationPath> paths = {
        ConfigurationPath(configFilePath, &DefaultFileSystem::Instance()),
        ConfigurationPath(userConfigFilePath, &DefaultFileSystem::Instance())
    };
    
    std::vector<std::pair<Path, ConfigurationFile::ParseResult>> results;
    
    Configuration cfg(paths, Configuration::Mode::Editable, &results);
    
    if (expectedValues->parseResults.size() != results.size()) {
        return TestResults(false, "The count of ParseResult entries differs");
    }
    
    const std::size_t actualValueCount = cfg.getValueCount();
    if (actualValueCount != expectedValueCount) {
        if (isOutputVerbose()) {
            LOG_V("All loaded config values:\n{}", cfg.printAllValues());
        }
        
        return TestResults(false, fmt::format("Expected {} config values, however, {} were loaded", expectedValueCount, actualValueCount));
    }
    
    for (std::size_t i = 0; i < expectedValues->parseResults.size(); ++i) {
        const auto& r = expectedValues->parseResults[i];
        if (r != results[i]) {
            if (isOutputVerbose()) {
                LOG_D("\nEXPECTED {}:\n{}\nACTUAL   {}:\n{}", r.first, r.second.printErrors(), results[i].first, results[i].second.printErrors())
            }
            return TestResults(false, "The expected results don't match real ones");
        }
    }
    
    const ConfigurationFile userConfFile = cfg.getUserConfigFile();
    
    if (userConfFile != expectedValues->configFilePreAdd) {
        if (isOutputVerbose()) {
            LOG_D("\nEXPECTED:\n{}\nACTUAL:\n{}", expectedValues->configFilePreAdd.print(), userConfFile.print())
        }
        
        return TestResults(false, "Parsed user's config file doesn't match the expected one");
    }
    
    auto editor = cfg.makeConfigurationEditor();
    editor->setValue("d", "hum", std::string("glowing"));
    editor->setValue("ned", "", 256l);
    editor->setValue("dodsp", "", 591.1614846);
    editor->commit(true);
    editor = nullptr;
    
    const ConfigurationFile userConfFilePostUpdate = cfg.getUserConfigFile();
    if (userConfFilePostUpdate != expectedValues->configFilePostAdd) {
        if (isOutputVerbose()) {
            LOG_D("\nEXPECTED:\n{}\nACTUAL:\n{}", expectedValues->configFilePostAdd.print(), userConfFilePostUpdate.print())
        }
        
        return TestResults(false, "User's config file after update doesn't match the expected one");
    }
    
    cfg.serialize();
    
    Configuration cfg2(paths, Configuration::Mode::Editable, &results);
    
    if (cfg.resolvedConfigurationValues.data != cfg2.resolvedConfigurationValues.data) {
        return TestResults(false, "Configuration values changed during serialization");
    }
    
    std::string v1 = cfg2.getValue("test01", "core");
    std::string v2 = cfg2.getValue("dodo", "");
    std::int64_t v3 = cfg2.getValue("a", "");
    bool v4 = cfg2.getValue("zimpl", "");
    bool v5 = cfg2.getValue("pimpl", "");
    double v6 = cfg2.getValue("dodsp", "");
    std::string v7 = cfg2.getValue("d", "hum");
    
    IYF_TEST_VALUE(v1, "whoAMI?");
    IYF_TEST_VALUE(v2, "now-a-string");
    IYF_TEST_VALUE(v3, 51619991551500l);
    IYF_TEST_VALUE(v4, false);
    IYF_TEST_VALUE(v5, true);
    IYF_TEST_VALUE(v6, 591.1614846);
    IYF_TEST_VALUE(v7, "glowing");
    
    return TestResults(true, "");
}

void ConfigurationTests::cleanup() {
    //
}

}

