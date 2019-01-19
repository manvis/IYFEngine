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

#ifndef CHUNKED_VECTOR_ITERATOR_HPP
#define CHUNKED_VECTOR_ITERATOR_HPP

#include <iterator>

namespace iyf {
template <typename T, size_t chunkSize>
class ChunkedVector;

/// Iterator for ChunkedVector
///
/// \todo Maybe merge this with ChunkedVectorConstIterator using the magic of templates.
template <typename A, typename T, size_t chunkSize>
class ChunkedVectorIterator {
public:
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using iterator_category = std::forward_iterator_tag;
    
    ChunkedVectorIterator(std::size_t pos = static_cast<std::size_t>(-1), A* container = nullptr) : container(container) {
        if (container != nullptr && pos != static_cast<std::size_t>(-1)) {
            const std::size_t containerSize = container->size();
            if (containerSize == 0 || pos == containerSize) {
                current = nullptr;
                
                return;
            }
            
            currentChunkID = pos / chunkSize;
            nextSwitch = container->getChunkEnd(currentChunkID);
            
            current = container->getPtr(pos);
        }
    }
    
    ChunkedVectorIterator& operator++() {
        current++;
        if (current == nextSwitch) {
            currentChunkID++;
            current = container->getChunkStart(currentChunkID);
            
            nextSwitch = container->getChunkEnd(currentChunkID);
        }
        
        return *this;
    }
    
    ChunkedVectorIterator operator++(int) {
        ChunkedVectorIterator temp = *this;
        ++(*this);
        return temp;
    }
    
    bool operator==(const ChunkedVectorIterator& other) const {
        return current == other.current;
    }
    
    bool operator!=(const ChunkedVectorIterator& other) const {
        return !(*this == other);
    }
    
    T& operator*() {
        return *current;
    }
private:
    std::size_t currentChunkID;
    A* container;
    T* current;
    T* nextSwitch;
};
}

#endif // CHUNKED_VECTOR_ITERATOR_HPP
