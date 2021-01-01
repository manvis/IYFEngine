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

#include <sstream>
#include <algorithm>
#include "utilities/StringUtilities.hpp"

namespace iyf::util {
std::vector<std::string_view> splitString(const std::string_view& str, const std::string_view& delimiters, std::size_t expectedTokenCount) {
    // Very fast pointer based string splitter based on
    // https://github.com/fenbf/StringViewTests/blob/master/StringViewTest.cpp
    std::vector<std::string_view> out;
    
    if (expectedTokenCount != 0) {
        out.reserve(expectedTokenCount);
    }

    const char* a = str.data();
    const char* b = str.data();
    const char* end = a + str.size();
    for (; b != end && a != end; a = b + 1) {
        b = std::find_first_of(a, end, std::cbegin(delimiters), std::cend(delimiters));

        if (a != b) {
            out.emplace_back(a, b - a);
        }
    }

    return out;
}

std::vector<std::string_view> splitStringOnString(const std::string_view& str, const std::string_view& searchString, std::size_t expectedTokenCount) {
    std::vector<std::string_view> out;
    if (str.empty() || searchString.empty()) {
        return out;
    }

    if (expectedTokenCount != 0) {
        out.reserve(expectedTokenCount);
    }

    std::size_t pos = 0;
    std::size_t offset = 0;
    do {
        pos = str.find(searchString, offset);

        const char* a;
        const char* b;

        if (pos != std::string_view::npos) {
            a = str.data() + offset;
            b = str.data() + pos;
        } else {
            a = str.data() + offset;
            b = str.data() + str.size();
        }

        if ((b - a) != 0) {
            out.emplace_back(a, b - a);
        }

        offset = pos + searchString.length();
    } while (pos != std::string_view::npos && offset < str.size());

    return out;
}
}
