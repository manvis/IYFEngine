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

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <memory>
#include <ctime>
#include <sstream>
#include <mutex>
#include <fstream>

//#include "localization/Localization.hpp"

namespace iyf {

// Inspired by
// http://stackoverflow.com/questions/8337300/c11-how-do-i-implement-convenient-logging-without-a-singleton

class LoggerOutput {
public:
    virtual void output(const std::string& message) = 0;
    virtual ~LoggerOutput() {};
};

class StringLoggerOutput : public LoggerOutput {
public:
    virtual void output(const std::string &message) override final {
        std::lock_guard<std::mutex> lock(stringMutex);
        logString.append(message).append("\n");
    }
    
    std::string getAndClear() {
        std::lock_guard<std::mutex> lock(stringMutex);
        std::string temp = logString;
        logString.clear();
        
        return temp;
    }
    
private:
    std::mutex stringMutex;
    std::string logString;
};

class FileLoggerOutput : public LoggerOutput {
public:
    FileLoggerOutput(const std::string& filePath) {
#ifdef APPEND_TO_LOG
        file.open(filePath, std::ofstream::app);
#else
        file.open(filePath, std::ofstream::trunc);
#endif
        if (!file.good()) {
            throw std::runtime_error("Failed to open log file.");
        }
    }

    virtual void output(const std::string &message) override final {
        std::lock_guard<std::mutex> lock(fileMutex);
        file << message << "\n";
        file.flush();
    }

    virtual ~FileLoggerOutput() {
        file.close();
    };
private:
    std::mutex fileMutex;
    std::ofstream file;
};

class LogSplitter : public LoggerOutput {
public:
    LogSplitter(std::unique_ptr<LoggerOutput> logOut1, std::unique_ptr<LoggerOutput> logOut2) : logOut1(std::move(logOut1)), logOut2(std::move(logOut2)) {}
    
    virtual void output(const std::string &message) override final {
        logOut1->output(message);
        logOut2->output(message);
    }
    
    LoggerOutput* getObserverToLog1() {
        return logOut1.get();
    }
    
    LoggerOutput* getObserverToLog2() {
        return logOut2.get();
    }
private:
    std::unique_ptr<LoggerOutput> logOut1;
    std::unique_ptr<LoggerOutput> logOut2;
};

class Logger {
public:
    enum class LogLevel {
        Verbose, Info, Debug, Warning, Error
    };

    Logger(std::unique_ptr<LoggerOutput> logOut)
        : output(std::move(logOut)) {}

    void operator() (const std::string& logMessage,
                     LogLevel logLevel,
                     const char* functionName,
                     const char* fileName,
                     int fileLine)
    {
        char out[25];
        std::time_t timeobj = std::time(nullptr);
        std::strftime(out, 25, "%Y-%m-%d %H:%M:%S ", std::localtime(&timeobj));
        std::ostringstream ss;

        switch (logLevel) {
            case LogLevel::Verbose :
                ss << out << verbose
                   << ": \n\t" << logMessage;
                output->output(ss.str());

                break;
            case LogLevel::Info :
                ss << out << info
                   << ": \n\t" << logMessage;
                output->output(ss.str());

                break;
            case LogLevel::Debug :
                ss << out << debug
                   << " in FUNCTION " << functionName
                   << ", FILE " << fileName
                   << ", LINE " << fileLine
                   << ": \n\t" << logMessage;
                output->output(ss.str());

                break;
            case LogLevel::Warning :
                ss << out << warning
                   << ": \n\t" << logMessage;
                output->output(ss.str());

                break;
            case LogLevel::Error :
                ss << out << error
                   << ": \n\t" << logMessage;
                output->output(ss.str());

                break;
        }
    }
    
    LoggerOutput* getOutputObserver() {
        return output.get();
    }

private:
    const std::string verbose = "VERBOSE";
    const std::string info = "INFO";
    const std::string debug = "DEBUG";
    const std::string warning = "WARNING";
    const std::string error = "ERROR";

    std::unique_ptr<LoggerOutput> output;
};

Logger& DefaultLog();
}

// Base macro for logging
#define LOG(Instance_, Level_, Message_)             \
    Instance_(                                       \
        static_cast<std::ostringstream&>(            \
            std::ostringstream().flush() << Message_ \
        ).str(),                                     \
        Level_,                                      \
        __FUNCTION__,                                \
        __FILE__,                                    \
        __LINE__                                     \
    );


#define LOG_V(Message_) LOG(iyf::DefaultLog(), iyf::Logger::LogLevel::Verbose, Message_)
#define LOG_I(Message_) LOG(iyf::DefaultLog(), iyf::Logger::LogLevel::Info, Message_)
#define LOG_W(Message_) LOG(iyf::DefaultLog(), iyf::Logger::LogLevel::Warning, Message_)
#define LOG_E(Message_) LOG(iyf::DefaultLog(), iyf::Logger::LogLevel::Error, Message_)

// Debug logging gets disabled in release builds that have assertions disabled
#ifdef NDEBUG
    #define LOG_D(_) do {} while(0);
#else
    #define LOG_D(Message_) LOG(iyf::DefaultLog(), iyf::Logger::LogLevel::Debug, Message_)
#endif // NDEBUG
//
//// Localized logger
//#define LLOG_V(Message_)

#endif // LOGGER_H
