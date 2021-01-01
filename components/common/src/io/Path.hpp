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

#ifndef IYF_PATH_HPP
#define IYF_PATH_HPP

#include <memory>
#include <filesystem>
#include <string>
#include <string_view>
#include "fmt/format.h"

namespace iyf {
class Path {
public:
    Path() {}
    Path(const Path& p) = default;
    Path(Path&& p) : path(std::move(p.path)) {
        p.path.clear();
    }

    template <typename T>
    Path(const T& p) : path(p) {}

    Path& operator=(const Path& p) {
        if (&p == this) {
            return *this;
        }

        path = p.path;
            
        return *this;
    }
    
    Path& operator=(Path&& p) noexcept {
        if (&p == this) {
            return *this;
        }

        path = std::move(p.path);
        p.clear();
        return *this;
    }

    template <typename T>
    Path& operator=(T&& p) {
        return *this = Path(std::move(p));
    }

    Path& operator/=(const Path& src) {
        path /= src.path;
        return *this;
    }

    template <class T>
    Path& operator/=(const T& src) {
        this->path /= src;
        return *this;
    }
    Path& operator+=(const Path& src) {
        path += src.path;
        return *this;
    }

    template <class T>
    Path& operator+=(const T& src) {
        this->path += src;
        return *this;
    }

    template <class T>
    Path& concat(const T& other) {
        path.concat(other);
        return *this;
    }

    Path parentPath() const {
        return Path(path.parent_path());
    }

    Path stem() const {
        return Path(path.stem());
    }

    Path filename() const {
        return Path(path.filename());
    }

    Path extension() const {
        return Path(path.extension());
    }

    std::string getGenericString() const {
        return path.generic_u8string();
    }

    const std::filesystem::path::string_type& getNativeString() const {
        return path.native();
    }

    const std::filesystem::path::value_type* getCString() const {
        return path.c_str();
    }

    void clear() {
        path.clear();
    }

    bool empty() const {
        return path.empty();
    }

    Path lexicallyNormal() const {
        return Path(path.lexically_normal());
    }

    Path lexicallyRelative(const Path& base) const {
        return Path(path.lexically_relative(base.path));
    }

    Path lexicallyProximate(const Path& base) const {
        return Path(path.lexically_proximate(base.path));
    }

    class Iterator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = Path;
        using reference = const Path&;
        using pointer = const Path*;
        using iterator_category	= std::bidirectional_iterator_tag;

        Iterator() {}

        Iterator(const Iterator&) = default;
        Iterator& operator=(const Iterator&) = default;

        value_type operator*() const {
            return *iter;
        }

        Iterator& operator++() {
            iter++;
            return *this;
        }

        Iterator operator++(int) {
            auto temp = *this;
            ++*this;
            return temp;
        }

        Iterator& operator--() {
            iter--;
            return *this;
        }

        Iterator operator--(int) {
            auto temp = *this;
            --*this;
            return temp;
        }

        friend bool operator==(const Iterator& a, const Iterator& b) {
            return a.iter == b.iter;
        }

        friend bool operator!=(const Iterator& a, const Iterator& b) {
            return a.iter != b.iter;
        }

    private:
        friend class Path;
        Iterator(std::filesystem::path::const_iterator iter) :
            iter(iter) {}

        std::filesystem::path::const_iterator iter;
    };

    Iterator begin() const {
        return Iterator(path.begin());
    }

    Iterator end() const {
        return Iterator(path.end());
    }

    friend Path operator/(const Path& a, const Path& b);
    friend bool operator==(const Path& a, const Path& b);
    friend bool operator!=(const Path& a, const Path& b);
    friend bool operator<(const Path& a, const Path& b);
    friend bool operator<=(const Path& a, const Path& b);
    friend bool operator>(const Path& a, const Path& b);
    friend bool operator>=(const Path& a, const Path& b);
    friend std::ostream& operator<<(std::ostream& os, const Path& p);
private:
    friend class DefaultFileSystem;

    std::filesystem::path path;
};
}

template<>
struct fmt::formatter<iyf::Path>: formatter<std::string> {
    template <typename FormatContext>
    auto format(const iyf::Path& p, FormatContext& ctx) {
        return formatter<std::string>::format(p.getGenericString(), ctx);
    }
};

#endif // IYF_PATH_HPP