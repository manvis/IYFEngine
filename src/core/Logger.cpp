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

#include "core/Logger.hpp"

namespace iyf
{
#define IYF_LOG_TO_BOTH
    
Logger& DefaultLog() {
#if defined(IYF_LOG_TO_BOTH)
    static std::unique_ptr<LoggerOutput> stringLogger = std::unique_ptr<LoggerOutput>(new StringLoggerOutput());
    static std::unique_ptr<LoggerOutput> fileLogger = std::unique_ptr<LoggerOutput>(new FileLoggerOutput("program.log"));
    
    static Logger l(std::unique_ptr<LogSplitter>(new LogSplitter(std::move(stringLogger), std::move(fileLogger))));
#elif defined(IYF_LOG_TO_STRING)
    static Logger l(std::unique_ptr<StringLoggerOutput>(new StringLoggerOutput()));
#else //IYF_LOG_TO_FILE
    static Logger l(std::unique_ptr<FileLoggerOutput>(new FileLoggerOutput("program.log")));
#endif
    return l;
}

}
