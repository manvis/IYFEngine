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

#ifndef IYF_STRING_UTILITIES_HPP
#define IYF_STRING_UTILITIES_HPP

#include <string_view>
#include <string>
#include <vector>

namespace iyf {
namespace util {
inline bool endsWith(const std::string_view& str, const std::string_view& endStr) {
    if (str.length() >= endStr.length()) {
        return str.compare(str.length() - endStr.length(), endStr.length(), endStr) == 0;
    } else {
        return false;
    }
}

inline bool startsWith(const std::string_view& str, const std::string_view& startStr) {
    if (str.length() >= startStr.length()) {
        return str.compare(0, startStr.length(), startStr) == 0;
    } else {
        return false;
    }
}

template <typename T>
bool isAlphaASCII(const T& str) {
    for (char c : str) {
        if (c < 65 || (c > 90 && c < 97) || c > 122) {
            return false;
        }
    }
    return true;
}

template <typename T>
bool isNumericASCII(const T& str) {
    for (char c : str) {
        if (c < 48 || c > 57) {
            return false;
        }
    }
    return true;
}

template <typename T>
bool isAlphanumericASCII(const T& str) {
    for (char c : str) {
        if (c < 48 || (c > 57 && c < 65) || (c > 90 && c < 97) || c > 122) {
            return false;
        }
    }
    return true;
}

/// \brief Very fast pointer based string splitter
/// \warning This function is NOT UTF-8 aware! Use it if you know the input is ASCII only.
/// \param str std::string_view to split
/// \param delimiters delimiters to use
/// \param expectedTokenCount set to the number of tokens you expect or 0 if you don't know and want the vector to handle everything automatically
std::vector<std::string_view> splitString(const std::string_view& str, const std::string_view& delimiters = " ", std::size_t expectedTokenCount = 5);
std::vector<std::string_view> splitStringOnString(const std::string_view& str, const std::string_view& searchString = " ", std::size_t expectedTokenCount = 5);
}
}

#endif // IYF_STRING_UTILITIES_HPP
