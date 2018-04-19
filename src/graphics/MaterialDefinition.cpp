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

#include "graphics/MaterialDefinition.hpp"
#include "core/filesystem/File.hpp"
#include "utilities/hashing/HashCombine.hpp"

#include <cassert>

namespace iyf {

const std::array<LocalizationHandle, static_cast<std::size_t>(MaterialRenderMode::COUNT)> MaterialRenderModeNames = {
    LH("opaqueRenderMode"),      // 0
    LH("transparentRenderMode"), // 1
};

void MaterialDefinition::serialize(Serializer& fw) const {
    assert(components.size() <= 255);
    
    if (idNeedsRecompute) {
        throw std::runtime_error("ID must be computed (by calling getID()) before serialization can begin");
    }
    
    if (components.size() > con::MaxMaterialComponents) {
        throw std::runtime_error("Can't have more than con::MaxMaterialComponents in a MaterialDefinition called " + name);
    }
    
    fw.writeUInt32(id);
    fw.writeUInt32(pipelineID.value());
    fw.writeUInt32(pipelineVariant.value());
    fw.writeUInt8(static_cast<std::uint8_t>(renderMode));
    fw.writeUInt8(components.size());
    
    for (const auto& d : components) {
        fw.writeUInt8(static_cast<std::uint8_t>(d.first));
        fw.writeUInt32(d.second);
    }
}

void MaterialDefinition::deserialize(Serializer& fr) {
    id = hash32_t(fr.readUInt32());
    idNeedsRecompute = false;
    
    pipelineID = hash32_t(fr.readUInt32());
    pipelineVariant = hash32_t(fr.readUInt32());
    renderMode = static_cast<MaterialRenderMode>(fr.readUInt8());
    
    std::size_t elementCount = fr.readUInt8();
    components.clear();
    components.reserve(elementCount);
    
    for (std::size_t i = 0; i < elementCount; ++i) {
        components.push_back(std::make_pair(static_cast<DataType>(fr.readUInt8()), fr.readUInt32()));
    }
}

void MaterialDefinition::serializeName(Serializer& fw) const {
    assert(name.size() + 1 <= 255);
    
    fw.writeUInt8(name.size() + 1);
    fw.writeBytes(name.c_str(), sizeof(char) * (name.size() + 1));
}

void MaterialDefinition::deserializeName(Serializer& fr) {
    std::size_t charCount = fr.readUInt8();
    
    std::vector<char> nameVec;
    nameVec.resize(charCount);
    
    fr.readBytes(nameVec.data(), sizeof(char) * charCount);
    
    name = std::string(nameVec.data());
}

void MaterialDefinition::recomputeID() {
    if (components.size() > con::MaxMaterialComponents) {
        throw std::runtime_error("Can't have more than con::MaxMaterialComponents in a MaterialDefinition called " + name);
    }
    
    hash32_t seed(0);
    
    util::HashCombine(seed, pipelineID);
    util::HashCombine(seed, pipelineVariant);
    util::HashCombine(seed, hash32_t(static_cast<std::uint32_t>(renderMode))); // Is this OK?
    
    for (const auto& d : components) {
        util::HashCombine(seed, hash32_t(d.second));
    }
    
    id = seed;
    
    idNeedsRecompute = false;
}

}
