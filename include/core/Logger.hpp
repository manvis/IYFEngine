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
#include <sstream>

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


#define LOG_V(Message_) LOG(iyf::DefaultLog(), iyf::LogLevel::Verbose, Message_)
#define LOG_I(Message_) LOG(iyf::DefaultLog(), iyf::LogLevel::Info, Message_)
#define LOG_W(Message_) LOG(iyf::DefaultLog(), iyf::LogLevel::Warning, Message_)
#define LOG_E(Message_) LOG(iyf::DefaultLog(), iyf::LogLevel::Error, Message_)

// Debug logging gets disabled in release builds that have assertions disabled
#ifdef NDEBUG
    #define LOG_D(_) do {} while(0);
#else
    #define LOG_D(Message_) LOG(iyf::DefaultLog(), iyf::LogLevel::Debug, Message_)
#endif // NDEBUG
//
//// Localized logger
//#define LLOG_V(Message_)

#endif // IYF_LOGGER_HPP
