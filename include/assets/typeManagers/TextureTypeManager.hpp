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

#ifndef IYF_TEXTURE_TYPE_MANAGER_HPP
#define IYF_TEXTURE_TYPE_MANAGER_HPP

#include "assets/AssetManager.hpp"
#include "assets/assetTypes/Texture.hpp"

namespace iyf {
class Engine;
class GraphicsAPI;

class TextureTypeManager : public TypeManager<Texture> {
public:
    TextureTypeManager(AssetManager* manager);
    
    virtual AssetType getType() final override {
        return AssetType::Texture;
    }
protected:
    virtual void initMissingAssetHandle() final override;
    
    virtual std::unique_ptr<LoadedAssetData> readFile(hash32_t nameHash, const fs::path& path, const Metadata& meta, Texture& assetData) final override;
    virtual void enableAsset(std::unique_ptr<LoadedAssetData> loadedAssetData, bool canBatch) final override;
    virtual void performFree(Texture& assetData) final override;
    virtual void executeBatchOperations() final override;
    
    virtual bool canBatchAsyncLoadedAssets() const final override {
        return true;
    }
private:
    GraphicsAPI* gfx;
    Engine* engine;
};
}
#endif // IYF_TEXTURE_TYPE_MANAGER_HPP
