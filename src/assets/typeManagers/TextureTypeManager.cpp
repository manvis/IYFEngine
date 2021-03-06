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
#include "assets/metadata/TextureMetadata.hpp"
#include "core/Engine.hpp"
#include "core/filesystem/VirtualFileSystem.hpp"
#include "io/File.hpp"
#include "logging/Logger.hpp"
#include "assets/loaders/TextureLoader.hpp"
#include "graphics/GraphicsAPI.hpp"

namespace iyf {
TextureTypeManager::TextureTypeManager(AssetManager* manager) : ChunkedVectorTypeManager(manager) {
    engine = manager->getEngine();
    gfx = engine->getGraphicsAPI();
}

void TextureTypeManager::performFree(Texture& assetData) {
    if (!gfx->destroyImage(assetData.image)) {
        LOG_W("Failed to destroy an image that was loaded from a file with hash: {}", assetData.getNameHash());
    }
}

std::unique_ptr<LoadedAssetData> TextureTypeManager::readFile(StringHash, const Path& path, const Metadata& meta, Texture& assetData) {
    auto file = VirtualFileSystem::Instance().openFile(path, FileOpenMode::Read);
    return std::make_unique<LoadedAssetData>(meta, assetData, file->readWholeFile());
}

void TextureTypeManager::enableAsset(std::unique_ptr<LoadedAssetData> loadedAssetData, bool canBatch) {
    const TextureMetadata& textureMeta = loadedAssetData->metadata.get<TextureMetadata>();
    Texture& assetData = static_cast<Texture&>(loadedAssetData->assetData);
    
    DeviceMemoryManager* memoryManager = gfx->getDeviceMemoryManager();
    
    const TextureLoader loader;
    TextureData textureData;
    
    TextureLoader::Result result = loader.load(loadedAssetData->rawData.first.get(), loadedAssetData->rawData.second, textureData);
    if (result != TextureLoader::Result::LoadSuccessful) {
        throw std::runtime_error("Failed to load a texture");
    }
    
    ImageCreateInfo ici = gfx->buildImageCreateInfo(textureData);
    
    const std::string textureName = fmt::format("Managed texture from {}", textureMeta.getSourceAssetPath().getGenericString());
    assetData.image = gfx->createImage(ici, textureName.c_str());
    if (canBatch) {
        memoryManager->updateImage(MemoryBatch::TextureAssetData, assetData.image, textureData);
    } else {
        memoryManager->updateImage(MemoryBatch::Instant, assetData.image, textureData);
    }
    
    assert(assetData.image.getExtent().x == textureMeta.getHeight());
    assert(assetData.image.getExtent().y == textureMeta.getWidth());
    assert(assetData.image.getMipLevels() == textureMeta.getLevels());
    assert(textureData.size == textureMeta.getSize());
    
    assetData.setLoaded(true);
}

void TextureTypeManager::executeBatchOperations() {
    gfx->getDeviceMemoryManager()->beginBatchUpload(MemoryBatch::TextureAssetData);
}

void TextureTypeManager::initMissingAssetHandle() {
    // TODO load a missing texture
    missingAssetHandle = AssetHandle<Texture>::CreateInvalid();
}

AssetsToEnableResult TextureTypeManager::hasAssetsToEnable() const {
    if (toEnable.empty()) {
        return AssetsToEnableResult::NoAssetsToEnable;
    }
    
    assert(toEnable.front().future.valid());
    if (toEnable.front().future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        DeviceMemoryManager* manager = gfx->getDeviceMemoryManager();
        
        if (!manager->canBatchFitData(MemoryBatch::MeshAssetData, Bytes(toEnable.front().estimatedSize))) {
            return AssetsToEnableResult::Busy;
        } else {
            return AssetsToEnableResult::HasAssetsToEnable;
        }
    } else {
        return AssetsToEnableResult::NoAssetsToEnable;
    }
}

std::uint64_t TextureTypeManager::estimateUploadSize(const Metadata& meta) const {
    const TextureMetadata& textureMeta = meta.get<TextureMetadata>();
    return textureMeta.getSize();
}
}
