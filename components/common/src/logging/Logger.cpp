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

#include "logging/Logger.hpp"

#include <mutex>
#include <fstream>
#include <ctime>

namespace iyf
{
#define IYF_LOG_TO_BOTH
    
constexpr const char* LOG_LVL_VERBOSE = "VERBOSE";
constexpr const char* LOG_LVL_INFO  = "INFO";
constexpr const char* LOG_LVL_DEBUG  = "DEBUG";
constexpr const char* LOG_LVL_WARNING  = "WARNING";
constexpr const char* LOG_LVL_ERROR  = "ERROR";

class StringLoggerOutput : public LoggerOutput {
public:
    virtual void output(const std::string &message) override final {
        std::lock_guard<std::mutex> lock(stringMutex);
        logString.append(message).append("\n");
    }
    
    virtual std::string getAndClearLogBuffer() final override {
        std::lock_guard<std::mutex> lock(stringMutex);
        
        std::string temp = logString;
        logString.clear();
        
        return temp;
    }
    
    virtual bool logsToBuffer() const final override {
        return true;
    }
    
    virtual std::string getLogBuffer() const final override {
        std::lock_guard<std::mutex> lock(stringMutex);
        
        return logString;
    }
    
    virtual void clearLogBuffer() final override {
        std::lock_guard<std::mutex> lock(stringMutex);
        
        logString.clear();
    }
private:
    mutable std::mutex stringMutex;
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

    virtual void output(const std::string &message) final override {
        std::lock_guard<std::mutex> lock(fileMutex);
        file << message << "\n";
        file.flush();
    }

    virtual ~FileLoggerOutput() {
        file.close();
    };
    
    virtual bool logsToBuffer() const final override {
        return false;
    }
    
    virtual std::string getAndClearLogBuffer() final override {
        throw std::logic_error("Can't get and clear a log buffer from FileLoggerOutput because it doesn't have one");
    }
    
    virtual std::string getLogBuffer() const final override {
        throw std::logic_error("Can't get a log buffer from FileLoggerOutput because it doesn't have one");
    }
    
    virtual void clearLogBuffer() final override {
        throw std::logic_error("Can't clear a log buffer in FileLoggerOutput because it doesn't have one");
    }
private:
    std::mutex fileMutex;
    std::ofstream file;
};

class LogSplitter : public LoggerOutput {
public:
    LogSplitter(LoggerOutput* logOut1, LoggerOutput* logOut2) : logOut1(logOut1), logOut2(logOut2) {
        if (logOut1 == nullptr || logOut2 == nullptr) {
            throw std::runtime_error("Log splitter inputs can't be nullptr");
        }
    }
    
    ~LogSplitter() {
        delete logOut1;
        delete logOut2;
    }
    
    virtual void output(const std::string &message) override final {
        logOut1->output(message);
        logOut2->output(message);
    }
    
    LoggerOutput* getObserverToLog1() {
        return logOut1;
    }
    
    LoggerOutput* getObserverToLog2() {
        return logOut2;
    }
    
    virtual bool logsToBuffer() const final override {
        return logOut1->logsToBuffer() || logOut2->logsToBuffer();
    }
    
    virtual std::string getAndClearLogBuffer() final override {
        if (logOut1->logsToBuffer()) {
            return logOut1->getAndClearLogBuffer();
        } else if (logOut2->logsToBuffer()) {
            return logOut2->getAndClearLogBuffer();
        } else {
            throw std::logic_error("Neither child of the LogSplitter has a backing buffer");
        }
    }
    
    virtual std::string getLogBuffer() const final override {
        if (logOut1->logsToBuffer()) {
            return logOut1->getLogBuffer();
        } else if (logOut2->logsToBuffer()) {
            return logOut2->getLogBuffer();
        } else {
            throw std::logic_error("Neither child of the LogSplitter has a backing buffer");
        }
    }
    
    virtual void clearLogBuffer() final override {
        if (logOut1->logsToBuffer()) {
            return logOut1->clearLogBuffer();
        } else if (logOut2->logsToBuffer()) {
            return logOut2->clearLogBuffer();
        } else {
            throw std::logic_error("Neither child of the LogSplitter has a backing buffer");
        }
    }
private:
    LoggerOutput* logOut1;
    LoggerOutput* logOut2;
};

Logger& DefaultLog() {
#if defined(IYF_LOG_TO_BOTH)
    static LoggerOutput* stringLogger = new StringLoggerOutput();
    static LoggerOutput* fileLogger = new FileLoggerOutput("program.log");
    
    static Logger l(new LogSplitter(stringLogger, fileLogger));
#elif defined(IYF_LOG_TO_STRING)
    static Logger l(new StringLoggerOutput());
#else //IYF_LOG_TO_FILE
    static Logger l(new FileLoggerOutput("program.log"));
#endif
    return l;
}

Logger::Logger(LoggerOutput* logOut) : output(logOut) {
    if (logOut == nullptr) {
        throw new std::runtime_error("Logger output can't be nullptr");
    }
}

Logger::~Logger() {
    delete output;
}

void Logger::operator() (const std::string& logMessage,
                    LogLevel logLevel,
                    const char* functionName,
                    const char* fileName,
                    int fileLine)
{
    char out[25];
    std::time_t timeobj = std::time(nullptr);
    std::strftime(out, 25, "%Y-%m-%d %H:%M:%S", std::localtime(&timeobj));

    switch (logLevel) {
        case LogLevel::Verbose :
            output->output(fmt::format("{} {}:\n\t{}", out, LOG_LVL_VERBOSE, logMessage));

            break;
        case LogLevel::Info :
            output->output(fmt::format("{} {}:\n\t{}", out, LOG_LVL_INFO, logMessage));

            break;
        case LogLevel::Debug :
            output->output(fmt::format("{} {} in FUNCTION {}, FILE {}, LINE {}:\n\t{}\n", out, LOG_LVL_DEBUG, functionName, fileName, fileLine, logMessage));

            break;
        case LogLevel::Warning :
            output->output(fmt::format("{} {}:\n\t{}", out, LOG_LVL_WARNING, logMessage));

            break;
        case LogLevel::Error :
            output->output(fmt::format("{} {} in FUNCTION {}, FILE {}, LINE {}:\n\t{}\n", out, LOG_LVL_ERROR, functionName, fileName, fileLine, logMessage));

            break;
    }
}

}
