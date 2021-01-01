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

#ifndef IYF_LOGGER_HPP
#define IYF_LOGGER_HPP

#include <string>

// WARNING: Enabling this definition activates some non portable macros. Moreover, the fmt code that performs
// compile time validation produces a ton of warnings because IYFEngine is compiled with most warning types enabled.
//#define IYF_CHECKED_FMT 

#ifdef IYF_CHECKED_FMT
#include "fmt/format.h"
#else // IYF_CHECKED_FMT
#include "fmt/core.h"
#endif // IYF_CHECKED_FMT

#include "utilities/NonCopyable.hpp"

namespace iyf {

// Inspired by
// http://stackoverflow.com/questions/8337300/c11-how-do-i-implement-convenient-logging-without-a-singleton

class LoggerOutput : private NonCopyable {
public:
    virtual void output(const std::string& message) = 0;
    virtual ~LoggerOutput() {};
    
    /// \return true if this LoggerOutput logs to a memory buffer that can be retrieved and displayed
    virtual bool logsToBuffer() const = 0;
    
    /// Combines getLogBuffer() and clearLogBuffer(). Usually better because you avoid locking the mutex two times.
    /// \warning Calling this function when logsToBuffer() == false is an error.
    /// \return the contents of the log buffer.
    virtual std::string getAndClearLogBuffer() = 0;
    
    /// \warning Calling this function when logsToBuffer() == false is an error.
    /// \return the contents of the log buffer.
    virtual std::string getLogBuffer() const = 0;
    
    /// Clear the log buffer.
    /// \warning Calling this function when logsToBuffer() == false is an error.
    virtual void clearLogBuffer() = 0;
};


enum class LogLevel {
    Verbose, Info, Debug, Warning, Error
};

class Logger : private NonCopyable {
public:
    Logger(LoggerOutput* logOut);
    ~Logger();

    void operator() (const std::string& logMessage,
                     LogLevel logLevel,
                     const char* functionName,
                     const char* fileName,
                     int fileLine);
    
    LoggerOutput* getOutputObserver() {
        return output;
    }
private:
    LoggerOutput* output;
};

Logger& DefaultLog();
}

// Base macro for logging
#ifdef IYF_CHECKED_FMT

#define LOG(Instance_, Level_, Str_, ...)        \
    Instance_(                                       \
        fmt::format(FMT_STRING(Str_), ##__VA_ARGS__),       \
        Level_,                                      \
        __FUNCTION__,                                \
        __FILE__,                                    \
        __LINE__                                     \
    );
#define LOG_V(Str_, ...) LOG(iyf::DefaultLog(), iyf::LogLevel::Verbose, Str_, ##__VA_ARGS__)
#define LOG_I(Str_, ...) LOG(iyf::DefaultLog(), iyf::LogLevel::Info, Str_, ##__VA_ARGS__)
#define LOG_W(Str_, ...) LOG(iyf::DefaultLog(), iyf::LogLevel::Warning, Str_, ##__VA_ARGS__)
#define LOG_E(Str_, ...) LOG(iyf::DefaultLog(), iyf::LogLevel::Error, Str_, ##__VA_ARGS__)

// Debug logging gets disabled in release builds that have assertions disabled
#ifdef NDEBUG
    #define LOG_D(_) do {} while(0);
#else
    #define LOG_D(Str_, ...) LOG(iyf::DefaultLog(), iyf::LogLevel::Debug, Str_, ##__VA_ARGS__)
#endif // NDEBUG
    
#else // IYF_CHECKED_FMT
    
#define LOG(Instance_, Level_, ...)         \
    Instance_(                                       \
        fmt::format(__VA_ARGS__),                    \
        Level_,                                      \
        __FUNCTION__,                                \
        __FILE__,                                    \
        __LINE__                                     \
    );

#define LOG_V(...) LOG(iyf::DefaultLog(), iyf::LogLevel::Verbose, __VA_ARGS__)
#define LOG_I(...) LOG(iyf::DefaultLog(), iyf::LogLevel::Info, __VA_ARGS__)
#define LOG_W(...) LOG(iyf::DefaultLog(), iyf::LogLevel::Warning, __VA_ARGS__)
#define LOG_E(...) LOG(iyf::DefaultLog(), iyf::LogLevel::Error, __VA_ARGS__)

// Debug logging gets disabled in release builds that have assertions disabled
#ifdef NDEBUG
    #define LOG_D(_) do {} while(0);
#else
    #define LOG_D(...) LOG(iyf::DefaultLog(), iyf::LogLevel::Debug, __VA_ARGS__)
#endif // NDEBUG

#endif // IYF_CHECKED_FMT

#endif // IYF_LOGGER_HPP
