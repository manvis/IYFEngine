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

#ifndef CHUNKED_VECTOR_HPP
#define CHUNKED_VECTOR_HPP

#include <vector>
#include <stdexcept>
#include <cassert>

#ifdef CREATION_MODE
#include <iostream>
#endif

#include "ChunkedVectorIterator.hpp"

namespace iyf {

/// Unlike std::vector, this container isn't completely contiguous. It allocates additional space in discrete
/// chunkSize chunks, which allows it to avoid expensive reallocations on growth. This also means that growing does not
/// invalidate any existing references or iterators since chunks never move in memory.
///
/// However, this container has some drawbacks. First of all, since growth happens in chunkSize intervals, it can potentially waste
/// memory if chunkSize is too big. Next, setting chunkSize too big or too small may also impact performance in some cases. Make sure you
/// always profile. Finally, random access via operator[] or at() is really, REALLY slow when compared to std::vector. The reason is
/// that these functions need to find a chunk in the chunk vector before they can return a reference to an object you seek.
/// This screws with the cache badly. Therefore, if you need to iterate over a range of elements as quickly as possible,
/// make sure to ALWAYS use the ChunkedVectorIterator that caches some state internally and avoids expensive chunk lookups.
/// You may also use getChunkStart() and getChunkEnd() to access chunk memory directly. That's what the ChunkedVectorIterator uses
/// under the hood.
template <typename T, size_t chunkSize>
class ChunkedVector { // TODO  : private NonCopyable
public:
    inline constexpr ChunkedVector() : capacityVal(0), sizeVal(0) {}
    
    using iterator = ChunkedVectorIterator<ChunkedVector<T, chunkSize>, T, chunkSize>;
    using const_iterator = ChunkedVectorIterator<const ChunkedVector<T, chunkSize>, const T, chunkSize>;
    
    friend iterator;
    friend const_iterator;
    
    inline iterator begin() {
        return iterator(0, this);
    }
    
    inline const_iterator begin() const {
        return const_iterator(0, this);
    }
    
    inline iterator end() {
        return iterator(size(), this);
    }
    
    inline const_iterator end() const {
        return const_iterator(size(), this);
    }
    
    inline std::size_t size() const {
        return sizeVal;
    }
    
    inline std::size_t capacity() const {
        return capacityVal;
    }
    
    inline void push_back(const T& value) {
        std::size_t newSize = sizeVal + 1;
        reserve(newSize);
        
        T* destination = getPtr(sizeVal);
        new (destination) T(value);
        
        sizeVal = newSize;
    }
    
    inline void push_back(T&& value) {
        emplace_back(std::move(value));
    }
    
    template<typename... Args>
    T& emplace_back(Args&&... args) {
        
        std::size_t newSize = sizeVal + 1;
        reserve(newSize);
        
        T* destination = getPtr(sizeVal);
        T* result = new(destination) T(args...);
        
        assert(destination == result);
        
        sizeVal = newSize;
        
        return *result;
    }
    
    inline void reserve(std::size_t newCapacity) {
        while (capacityVal < newCapacity) {
            constexpr std::size_t size = sizeof(T);
            const std::size_t arraySize = size * chunkSize;
            
            #ifdef CREATION_MODE
            //std::cout << "NEW ChunkedVector. New Call: " << size << " " << alignment << " " << finalSize << " " << arraySize << "\n";
            std::cout << "NEW ChunkedVector. New Call: " << size << " " << arraySize << "\n";
            #endif
            
            char* newChunk = new char[arraySize];
            chunks.push_back(newChunk);
            capacityVal += chunkSize;
        }
    }
    
    inline void resize(std::size_t newSize, const T& value) {
        if (newSize < sizeVal) {
            throw std::logic_error("ChunkedVector can't shrink... yet");
        } else if (newSize == sizeVal) {
            return;
        }
        
        reserve(newSize);
        
        std::size_t currentChunkID = sizeVal / chunkSize;
        T* current = getPtr(sizeVal);
        T* nextSwitch = getChunkEnd(currentChunkID);
        T* lastElement = getPtr(newSize - 1) + 1;
        
        while (current != lastElement) {
            if (current == nextSwitch) {
                currentChunkID++;
                current = getChunkStart(currentChunkID);
                
                nextSwitch = getChunkEnd(currentChunkID);
            }
            
            T* result = new(current) T(value);
            assert(current == result);
            
            current++;
        }
        
        sizeVal = newSize;
    }
    
    inline void resize(std::size_t newSize) {
        resize(newSize, T());
    }
    
    inline T& at(std::size_t id) {
        //std::cout << "Regular\n";
        if (id >= sizeVal) {
            throw std::out_of_range("ID is too big.");
        }
        
        return *getPtr(id);
    }
    
    inline const T& at(std::size_t id) const {
        if (id >= sizeVal) {
            throw std::out_of_range("ID is too big.");
        }
        
        return *getPtr(id);
    }
    
    inline T& operator[](std::size_t id) {
        return *getPtr(id);
    }
    
    inline const T& operator[](std::size_t id) const {
        return *getPtr(id);
    }
    
    inline std::size_t chunkCount() const {
        return chunks.size();
    }
    
    /// Access the chunk memory directly. This, together with getChunkEnd() can be useful when
    /// you want to replace expensive operator[] and at() calls with signifficantly faster
    /// pointer increments.
    inline T* getChunkStart(std::size_t chunkID) {
        if (chunkID < chunks.size()) {
            return static_cast<T*>(static_cast<void*>(chunks[chunkID]));
        } else {
            return nullptr;
        }
    }
    
    /// Access the chunk memory directly. This, together with getChunkEnd() can be useful when
    /// you want to replace expensive operator[] and at() calls with signifficantly faster
    /// pointer increments.
    inline const T* getChunkStart(std::size_t chunkID) const {
        if (chunkID < chunks.size()) {
            return static_cast<T*>(static_cast<void*>(chunks[chunkID]));
        } else {
            return nullptr;
        }
    }
    
    /// Access the chunk memory directly. This, together with getChunkStart() can be useful when
    /// you want to replace expensive operator[] and at() calls with signifficantly faster
    /// pointer increments.
    inline T* getChunkEnd(std::size_t chunkID) {
        if (chunkID < chunks.size()) {
            return static_cast<T*>(static_cast<void*>(chunks[chunkID])) + chunkSize;
        } else {
            return nullptr;
        }
    }
    
    /// Access the chunk memory directly. This, together with getChunkStart() can be useful when
    /// you want to replace expensive operator[] and at() calls with signifficantly faster
    /// pointer increments.
    inline const T* getChunkEnd(std::size_t chunkID) const {
        if (chunkID < chunks.size()) {
            return static_cast<T*>(static_cast<void*>(chunks[chunkID])) + chunkSize;
        } else {
            return nullptr;
        }
    }
    
    inline T* data(std::size_t id) {
        return getPtr(id);
    }
    
    inline const T* data(std::size_t id) const {
        return getPtr(id);
    }
    
    inline void clear() {
        // TODO I should be incrementing pointers instead of calling getPtr()
        for (std::size_t i = 0; i < sizeVal; ++i) {
            T* data = getPtr(i);
            data->~T();
        }
        
        for (char* c : chunks) {
            delete[] c;
        }
        
        chunks.clear();
        capacityVal = 0;
        sizeVal = 0;
    }
    
    virtual ~ChunkedVector() {
        clear();
    }
protected:
    
    inline T* getPtr(std::size_t id) {
        char* chunk = chunks[id / chunkSize];
        
        return std::launder(reinterpret_cast<T*>(chunk + (id % chunkSize) * sizeof(T)));
    }
    
    inline const T* getPtr(std::size_t id) const {
        char* chunk = chunks[id / chunkSize];
        
        return std::launder(reinterpret_cast<T*>(chunk + (id % chunkSize) * sizeof(T)));
    }
    
    std::vector<char*> chunks;
    std::size_t capacityVal;
    std::size_t sizeVal;
};

}

#endif // CHUNKED_VECTOR_HPP
