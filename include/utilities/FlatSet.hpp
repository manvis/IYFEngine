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

#ifndef FLAT_SET_HPP
#define FLAT_SET_HPP

#include <vector>
#include <algorithm>
#include <functional>

namespace iyf {

template <typename T, class C = std::less<T>>
class FlatSet {
public:
    /// Modifying an object in set may make it invalid, that's why we're using a const_iterator here
    using iterator = typename std::vector<T>::const_iterator;
    using const_iterator = typename std::vector<T>::const_iterator;
    using size_type = typename std::vector<T>::size_type;
    
    inline void reserve(std::size_t newSize) {
        data.reserve(newSize);
    }
    
    inline bool insert(const T& value) {
        //std::cout << "Copied\n";
        auto it = std::lower_bound(data.begin(), data.end(), value, C());
        
        
        if (it != data.end() && *it == value) {
            //std::cout << "e" << *it << " " << value << "\n";
            return false;
        }
        
        data.insert(it, value);
        return true;
    }
    
    inline bool insert(const T&& value) {
        //std::cout << "Moved\n";
        auto it = std::lower_bound(data.begin(), data.end(), value, C());
        
        
        if (it != data.end() && *it == value) {
            //std::cout << "e" << *it << " " << value << "\n";
            return false;
        }
        
        data.insert(it, std::move(value));
        return true;
    }
    
    inline iterator begin() noexcept {
        return data.begin();
    }
    
    inline const_iterator begin() const noexcept {
        return data.begin();
    }
    
    inline const_iterator cbegin() const noexcept {
        return data.cbegin();
    }
    
    inline iterator end() noexcept {
        return data.end();
    }
    
    inline const_iterator end() const noexcept {
        return data.end();
    }
    
    inline const_iterator cend() const noexcept {
        return data.cend();
    }
    
    inline iterator erase(const_iterator pos) {
        return data.erase(pos);
    }
    
    inline iterator erase(const_iterator first, const_iterator last) {
        return data.erase(first, last);
    }
    
    inline size_type size() const noexcept {
        return data.size();
    }
    
    inline size_type capacity() const noexcept {
        return data.capacity();
    }
    
    inline void clear() noexcept {
        data.clear();
    }
    
    inline bool empty() const noexcept {
        return data.empty();
    }
protected:
    using CompareType = C;
    
    std::vector<T> data;
};

}

#endif //FLAT_SET_HPP
