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

#include "utilities/ReadWholeFile.hpp"

#include <fstream>
#include <stdexcept>

namespace iyf::util {
std::pair<std::unique_ptr<char[]>, std::int64_t> ReadWholeFile(std::ifstream& file) {
    // Don't forget to save the current position. We don't want to mess someone's logic
    const std::int64_t currentPos = file.tellg();
    if (!file.good()) {
        throw std::runtime_error("Failed to tell the current position of a file.");
    }
    
    file.seekg(0, std::ios_base::end);
    if (!file.good()) {
        throw std::runtime_error("Failed to seek to the end of a file.");
    }
    
    std::int64_t size = file.tellg();
    if (!file.good()) {
        throw std::runtime_error("Failed to tell the current position of a file.");
    }
    
    file.seekg(0, std::ios_base::beg);
    if (!file.good()) {
        throw std::runtime_error("Failed to seek to the start of a file.");
    }
    
    auto buffer = std::make_unique<char[]>(size + 1);
    file.read(buffer.get(), size);
    if (!file.good()) {
        throw std::runtime_error("Failed to read the required number of bytes from a file.");
    }
    
    buffer[size] = '\0';
    
    // Don't forget to reset the position.
    file.seekg(currentPos, std::ios_base::beg);
    if (!file.good()) {
        throw std::runtime_error("Failed to seek to the original position from a file.");
    }
    
    return {std::move(buffer), size};
}
}
