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

#include "utilities/BufferRangeSet.hpp"

#include <stdexcept>

namespace iyf {
bool BufferRangeSet::insert(const BufferRange& value) {
    if (value.offset + value.size > totalSpace) {
        return false;
    }
    
    auto beginning = data.begin();
    auto ending = data.end();
    
    auto it = std::lower_bound(beginning, ending, value, CompareType());
    
    freeSpace += value.size;
    
    if (it != beginning && it != ending) { // Safe to check both
        BufferRange& previous = *(std::prev(it));
        BufferRange& next = *it;
        
        bool resizePrevious = (previous.offset + previous.size) == value.offset;
        bool resizeNext = value.offset + value.size == next.offset;
        
        if (resizePrevious && resizeNext) {
            previous.size += (value.size + next.size);
            data.erase(it);
        } else if (resizePrevious) {
            previous.size += value.size;
        } else if (resizeNext) {
            next.offset -= value.size;
            next.size += value.size;
        } else {
            data.insert(it, value);
        }
        
        
    } else if (it == ending && it != beginning) { // Safe to check the previous one
        BufferRange& previous = *(std::prev(it));
        
        if (previous.offset + previous.size == value.offset) {
            previous.size += value.size;
        } else {
            data.insert(it, value);
        }
        
    } else if (it == beginning && it != ending) { // Safe to check the next one
        BufferRange& next = *it;
        
        if (value.offset + value.size == next.offset) {
            next.offset -= value.size;
            next.size += value.size;
        } else {
            data.insert(it, value);
        }
    } else { // Just insert and that's it
        data.insert(it, value);
    }
    
    return true;
}

bool BufferRangeSet::insert(const BufferRange&& value) {
    if (value.offset + value.size > totalSpace) {
        return false;
    }
    
    auto beginning = data.begin();
    auto ending = data.end();
    
    auto it = std::lower_bound(beginning, ending, value, CompareType());
    
    freeSpace += value.size;
    
    if (it != beginning && it != ending) { // Safe to check both
        BufferRange& previous = *(std::prev(it));
        BufferRange& next = *it;
        
        bool resizePrevious = (previous.offset + previous.size) == value.offset;
        bool resizeNext = value.offset + value.size == next.offset;
        
        if (resizePrevious && resizeNext) {
            previous.size += (value.size + next.size);
            data.erase(it);
        } else if (resizePrevious) {
            previous.size += value.size;
        } else if (resizeNext) {
            next.offset -= value.size;
            next.size += value.size;
        } else {
            data.insert(it, std::move(value));
        }
        
        
    } else if (it == ending && it != beginning) { // Safe to check the previous one
        BufferRange& previous = *(std::prev(it));
        
        if (previous.offset + previous.size == value.offset) {
            previous.size += value.size;
        } else {
            data.insert(it, std::move(value));
        }
        
    } else if (it == beginning && it != ending) { // Safe to check the next one
        BufferRange& next = *it;
        
        if (value.offset + value.size == next.offset) {
            next.offset -= value.size;
            next.size += value.size;
        } else {
            data.insert(it, std::move(value));
        }
    } else { // Just insert and that's it
        data.insert(it, std::move(value));
    }
    
    return true;
}

BufferRangeSet::FreeRange BufferRangeSet::getFreeRange(Bytes size, Bytes alignment) {
    if (size % alignment != 0) {
        throw std::logic_error("size must be a multiple of alignment");
    }
    
    if (size > totalSpace) {
        return {BufferRange(), 0, false};
    }
    
    for (auto it = data.begin(); it < data.end(); ++it) {
        BufferRange& r = *it;
        
        if (r.size >= size) {
            Bytes offset = r.offset;
            
            std::uint32_t padding = 0;
            // The offset is not aligned to what we need. We'll have to add some padding
            if (offset % alignment != 0) {
                padding = alignment - (offset % alignment);
                
                // We will also need to increase the size of the allocation and this may render it invalid
                size += Bytes(padding);
                
                if (r.size < size) {
                    continue;
                }
            }
            
            r.size -= size;
            r.offset += size;
            freeSpace -= size;
            
            if (r.size == Bytes(0)) {
                // This should be safe since we're returning immediately
                data.erase(it);
            }
            
            return {BufferRange(offset, size), padding, true};
        }
    }
    
    return {BufferRange(), 0, false};
}
}
