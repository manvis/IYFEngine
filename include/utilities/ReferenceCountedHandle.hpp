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

#ifndef REFERENCE_COUNTED_HANDLE_HPP
#define REFERENCE_COUNTED_HANDLE_HPP

#include <utility>

namespace iyf {
template <typename T, typename RefCounter>
class ReferenceCountedHandle {
public:
    inline ReferenceCountedHandle(T* item = nullptr, RefCounter* counter = nullptr) : item(item), counter(counter) {
        if (isValid()) {
            (*counter)++;
        }
    }
    
    inline ReferenceCountedHandle(const ReferenceCountedHandle& other) : item(other.item), counter(other.counter) {
        if (isValid()) {
            (*counter)++;
        }
    }
    
    inline ReferenceCountedHandle(ReferenceCountedHandle&& other) : ReferenceCountedHandle() {
        swap(*this, other);
    }
    
    inline ReferenceCountedHandle& operator=(ReferenceCountedHandle other) {
        swap(*this, other);
        return *this;
    }
    
    inline ~ReferenceCountedHandle() {
        if (isValid()) {
            (*counter)--;
        }
    }
    
    inline bool isValid() const {
        return (item != nullptr) && (counter != nullptr);
    }
    
    inline T* operator->() {
        return item;
    }
    
    inline const T* operator->() const {
        return item;
    }
    
    inline T* operator*() {
        return item;
    }
    
    inline const T* operator*() const {
        return item;
    }
    
    inline T& operator&() {
        return *item;
    }
    
    inline const T& operator&() const {
        return *item;
    }
    
    inline std::size_t getCount() {
        return *counter;
    }
    
    inline bool release() {
        if (isValid()) {
            (*counter)--;
            
            item = nullptr;
            counter = nullptr;
            
            return true;
        } else {
            return false;
        }
    }
    
    /// Swaps the contents of two ReferenceCountedHandle
    friend void swap(ReferenceCountedHandle& first, ReferenceCountedHandle& second) {
        // Good explanation on why a swap function is needed
        // https://stackoverflow.com/a/3279550/1723459
        
        std::swap(first.item, second.item);
        std::swap(first.counter, second.counter);
    }
    
private:
    T* item;
    RefCounter* counter;
};
}

#endif // REFERENCE_COUNTED_HANDLE_HPP
