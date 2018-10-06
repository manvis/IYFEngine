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

#include "assets/typeManagers/TextureTypeManager.hpp"
#include "core/Engine.hpp"
#include "core/filesystem/File.hpp"
#include "core/Logger.hpp"
#include "graphics/GraphicsAPI.hpp"

namespace iyf {
TextureTypeManager::TextureTypeManager(AssetManager* manager) : TypeManager(manager) {
    engine = manager->getEngine();
    gfx = engine->getGraphicsAPI();
}

void TextureTypeManager::performFree(Texture& assetData) {
    if (!gfx->destroyImage(assetData.image)) {
        LOG_W("Failed to destroy an image that was loaded from a file with hash: " << assetData.getNameHash());
    }
}

std::unique_ptr<LoadedAssetData> TextureTypeManager::readFile(hash32_t nameHash, const fs::path& path, const Metadata& meta, Texture& assetData) {
    File file(path, File::OpenMode::Read);
    return std::make_unique<LoadedAssetData>(meta, assetData, file.readWholeFile());
}

void TextureTypeManager::enableAsset(std::unique_ptr<LoadedAssetData> loadedAssetData, bool canBatch) {
    const TextureMetadata& textureMeta = std::get<TextureMetadata>(loadedAssetData->metadata);
    Texture& assetData = static_cast<Texture&>(loadedAssetData->assetData);
    
    if (canBatch) {
        throw std::runtime_error("Batching not yet implemented");
    } else {
        assetData.image = gfx->createCompressedImage(loadedAssetData->rawData.first.get(), loadedAssetData->rawData.second);
    }
    
    assert(assetData.image.height == textureMeta.getHeight());
    assert(assetData.image.width == textureMeta.getWidth());
    assert(assetData.image.layers == textureMeta.getLayers());
    assert(assetData.image.levels == textureMeta.getLevels());
    assetData.setLoaded(true);
}

void TextureTypeManager::executeBatchOperations() {
    gfx->getDeviceMemoryManager()->beginBatchUpload(MemoryBatch::TextureAssetData);
}

void TextureTypeManager::initMissingAssetHandle() {
    // TODO load a missing texture
    missingAssetHandle = AssetHandle<Texture>();
}
}
