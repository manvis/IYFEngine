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

#ifndef IYF_MATERIAL_INPUTS_HPP
#define IYF_MATERIAL_INPUTS_HPP

#include "glm/vec4.hpp"
#include "utilities/hashing/Hashing.hpp"

#include <string>
#include <cstdint>

namespace iyf {

class MaterialInputVariable {
public:
    MaterialInputVariable() : componentCount(0) {}
    MaterialInputVariable(std::string name, const glm::vec4& defaultValue, std::uint8_t componentCount) 
        : defaultValue(defaultValue), name(std::move(name)), componentCount(componentCount) {}
    
    inline const glm::vec4& getDefaultValue() const {
        return defaultValue;
    }
    
    inline const std::string& getName() const {
        return name;
    }
    
    inline std::uint8_t getComponentCount() const {
        return componentCount;
    }
    
    /// \warning If this returns false, no other getters can be used safely and this MaterialInputVariable should just be ignored
    inline bool isValid() const {
        return componentCount != 0;
    }
private:
    glm::vec4 defaultValue;
    std::string name;
    std::uint8_t componentCount;
};

class MaterialInputTexture {
public:
    MaterialInputTexture() : defaultTexture(0) {}
    MaterialInputTexture(std::string name, StringHash defaultTexture) 
        : name(std::move(name)), defaultTexture(defaultTexture) {}
    
    inline const StringHash getDefaultTexture() const {
        return defaultTexture;
    }
    
    inline const std::string& getName() const {
        return name;
    }
    
    /// \warning If this returns false, no other getters can be used safely and this MaterialInputVariable should just be ignored
    inline bool isValid() const {
        return (defaultTexture != 0) && (!name.empty());
    }
private:
    std::string name;
    StringHash defaultTexture;
};

}

#endif // IYF_MATERIAL_INPUTS_HPP
