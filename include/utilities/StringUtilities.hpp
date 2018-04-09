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

#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <sstream>
//#include <iterator>
#include <vector>

namespace iyf {
namespace util {
inline bool endsWith(const std::string& str, const std::string& endStr) {
    if (str.length() >= endStr.length()) {
        return str.compare(str.length() - endStr.length(), endStr.length(), endStr) == 0;
    } else {
        return false;
    }
}

inline bool startsWith(const std::string& str, const std::string& startStr) {
    if (str.length() >= startStr.length()) {
        return str.compare(0, startStr.length(), startStr) == 0;
    } else {
        return false;
    }
}

inline std::vector<std::string> split(const std::string& input, char delimiter) {
    std::stringstream ss;
    ss.str(input);
    
    std::vector<std::string> results;
    std::string temp;
    
    while (std::getline(ss, temp, delimiter)) {
        results.push_back(temp);
    }

    return results;
}
}
}

#endif
