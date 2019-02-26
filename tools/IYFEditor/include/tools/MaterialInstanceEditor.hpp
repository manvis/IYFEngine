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

#ifndef IYF_MATERIAL_INSTANCE_EDITOR_HPP
#define IYF_MATERIAL_INSTANCE_EDITOR_HPP

#include <string>
#include <unordered_map>
#include "graphics/materials/MaterialInputs.hpp"
#include "graphics/materials/MaterialRenderMode.hpp"
#include "core/filesystem/cppFilesystem.hpp"

namespace iyf {
class Engine;
class AssetManager;
class MaterialInstanceDefinition;

namespace util {
struct DragDropAssetPayload;
}

}

namespace iyf::editor {

class MaterialInstanceEditor {
public:
    MaterialInstanceEditor(Engine* engine);
    
    void show(bool* showing);
    void setFilePath(const fs::path& path);
    inline const fs::path& getFilePath() const {
        return filePath;
    }
    
    static std::string CreateNew();
protected:
    void changeAsset(const util::DragDropAssetPayload& payload);
    
    StringHash materialTemplateAsset;
    
    fs::path filePath;
    Engine* engine;
    AssetManager* assetManager;
    std::unique_ptr<MaterialInstanceDefinition> instanceDefinition;
    std::unordered_map<StringHash, glm::vec4> instanceVariables;
    std::unordered_map<StringHash, StringHash> instanceTextures;
    std::vector<std::pair<MaterialRenderMode, std::string>> materialRenderModeNames;
    MaterialRenderMode materialRenderMode;
};

}

#endif // IYF_MATERIAL_INSTANCE_EDITOR_HPP

