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

#ifndef BUFFER_RANGE_MANAGER_HPP
#define BUFFER_RANGE_MANAGER_HPP

#include "utilities/DataSizes.hpp"
#include "utilities/FlatSet.hpp"

namespace iyf {
struct BufferRange {
    inline constexpr BufferRange() : offset(0), size(0) {}
    inline constexpr BufferRange(Bytes start, Bytes size) : offset(start), size(size) {}
    
    Bytes offset;
    Bytes size;
    
    friend inline bool operator<(const BufferRange& l, const BufferRange& r) {
        return l.offset < r.offset;
    }
    
    friend inline bool operator>(const BufferRange& l, const BufferRange& r) {
        return l.offset > r.offset;
    }
    
    friend inline bool operator==(const BufferRange& l, const BufferRange& r) {
        return (l.offset == r.offset) && (l.size == r.size);
    }
};

/// This container is used to keep track of free ranges in data buffers for later reuse. It does not
/// store any pointers to actual data buffers and does not perform any memory management on its own,
/// therefore it can be used for buffers that reside in either RAM or VRAM.
///
/// How to use:
///
/// 1. Call getFreeRange() to request a free range that would fit the specified number of bytes. This
/// may fail because of the fragmentation, so always check your return values.
///
/// 2. Call insert() to return a range that you no longer wish to use back into the set. This will
/// automatically merge touching ranges into a bigger one.
///
/// \warning This container assumes that ranges CANNOT overlap.
/// 
/// \todo Test with zeros (e.g. a vertex only mesh has no need for an ibo and would be submitting
/// 0 sized insert() and getFreeRange() calls).
///
/// \todo Implement defragmentation and perform it once the number of fragments reaches
/// a certain threshold.
///
/// \todo For now, getFreeRange() returns the first interval that can fit the size. How does
/// this affect fragmentation? Would it be better to find the smallest interval that can
/// fully contain the size? The biggest?
class BufferRangeSet : public FlatSet<BufferRange> {
public:
    BufferRangeSet(Bytes totalSpace) : totalSpace(totalSpace), freeSpace(totalSpace) {
        data.push_back({Bytes(0), totalSpace});
    }
    
    /// For example, imagine that a buffer already contains four two-byte objects. You need to store two more 5 byte
    /// objects. To do so, you pass 10 (size) and 5 (alignment) to the getFreeRange() function. Assuming that the
    /// amount of free space is sufficient and no other data exists in the buffer, you'll get:
    ///     - completeRange with offset == 8 and size == 12
    ///     - startPadding == 2
    ///     - status = true
    /// When writing the data into the actual buffer, start at the 10 byte offset (completeRange.offset + startPadding)
    struct FreeRange {
        /// The whole allocated range, including startPadding. Because of padding, this may end up being slightly bigger
        /// than what was requested when calling getFreeRange(). A BufferRange object with the exact same offset and size
        /// must be given to the insert() function in order to mark it as free for reuse.
        BufferRange completeRange;
        /// This value will only be non-zero when you try to store objects of different sizes into a single data buffer.
        /// It indicates how much padding is needed at the start of the range in order to have a propper alignment.
        std::uint32_t startPadding;
        /// Was a free range found in this BufferRangeSet or not? If this is false, ignore all other values in this object
        bool status;
    };
    
    /// Used to mark a BufferRange as free for reuse.
    ///
    /// This method hides the one from the base class. The main difference is that this one
    /// checks if surrounding BufferRanges touch the one that's being inserted and, if they are,
    /// merges them.
    ///
    /// \warning This method does not check if the BufferRange being inserted overlaps with others.
    /// It is assumed that no overlap exists. Inserting overlapping BufferRanges results in undefinded
    /// behaviour.
    inline bool insert(const BufferRange& value) {
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
    
    /// Used to mark a BufferRange as free for reuse.
    ///
    /// This method hides the one from the base class. The main difference is that this one
    /// checks if surrounding BufferRanges touch the one that's being inserted and, if they are,
    /// merges them.
    ///
    /// \warning This method does not check if the BufferRange being inserted overlaps with others.
    /// It is assumed that no overlap exists. Inserting overlapping BufferRanges results in undefinded
    /// behaviour.
    inline bool insert(const BufferRange&& value) {
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
    
    /// Request a free BufferRange. Please read the docs of BufferRangeSet::FreeRange in order to know how to interpret
    /// the returned data.
    /// 
    /// \param[in] size Total required size in Bytes
    /// \param[in] alignment Typically, the size of a single object. Used to compute padding when storing objects of
    /// diffrent sizes in a single data buffer.
    inline FreeRange getFreeRange(Bytes size, Bytes alignment) {
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
    
    /// Get the amount of remaining free space
    Bytes getFreeSpace() const {
        return freeSpace;
    }
    
    /// Get the total amount of space
    Bytes getTotalSpace() const {
        return totalSpace;
    }
private:
    Bytes totalSpace;
    Bytes freeSpace;
};
}

#endif //BUFFER_RANGE_MANAGER_HPP
