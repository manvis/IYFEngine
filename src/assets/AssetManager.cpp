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

#include "assets/AssetManager.hpp"
#include "graphics/MeshLoader.hpp"
#include "graphics/MeshComponent.hpp"
#include "core/Constants.hpp"
#include "core/Logger.hpp"
#include "core/Engine.hpp"
#include "core/filesystem/FileSystem.hpp"
#include "assets/typeManagers/MeshTypeManager.hpp"
#include "assets/typeManagers/ShaderTypeManager.hpp"
#include "utilities/DataSizes.hpp"

namespace iyf {
using namespace iyf::literals;
void TypeManagerBase::logLeakedAsset(std::size_t id, hash32_t nameHash, std::uint32_t count) {
    LOG_W("Asset with id " << id << " loaded from path " << manager->getAssetPath(nameHash) << " still has " << count << " live references. ")
}

AssetManager::AssetManager(Engine* engine) : engine(engine), isInit(false) {
    if (std::atomic<std::uint32_t>::is_always_lock_free) {
        LOG_V("std::uint32_t is lock free on this system");
    } else {
        LOG_W("std::uint32_t is NOT lock free on this system");
    }
}

AssetManager::~AssetManager() { }

void AssetManager::initialize() {
    if (isInit) {
        return;
    }
    
    //LOG_D(sizeof(MeshMetadata) << " " << sizeof(TextureMetadata) << " " << sizeof(Metadata) << " " << sizeof(ManifestElement) << " l " << sizeof(MeshVertex))
    //LOG_D("MAH_MESH" << sizeof(Mesh) << " " << sizeof(std::size_t))
    BufferRangeSet brs(Bytes(64));
    brs.getFreeRange(Bytes(8), Bytes(2));
    auto fr = brs.getFreeRange(Bytes(10), Bytes(5));
    LOG_D("Ranges " << fr.completeRange.offset.count() << " " << fr.completeRange.size.count() << " " << fr.startPadding)
    
    typeManagers[static_cast<std::size_t>(AssetType::Mesh)] = std::unique_ptr<MeshTypeManager>(new MeshTypeManager(this, 2_MiB, 1_MiB));
    typeManagers[static_cast<std::size_t>(AssetType::Shader)] = std::unique_ptr<ShaderTypeManager>(new ShaderTypeManager(this));
    //AssetHandle<Mesh> r = load<Mesh>(HS("nano"));
    
    // TODO Load the metadata into the manifest
    buildManifestFromFilesystem();
    
    loadSystemAssets();
    
    for (auto& tm : typeManagers) {
        if (tm != nullptr) {
            tm->initMissingAssetHandle();
        }
    }
    
    isInit = true;
}

void AssetManager::dispose() {
    for (auto& tm : typeManagers) {
        if (tm != nullptr) {
            tm->collectGarbage(GarbageCollectionRunPolicy::FullCollectionDuringDestruction);
            tm = nullptr;
        }
    }
    
    isInit = false;
}

void addFilesToManifest(FileSystem* filesystem, AssetType type, std::unordered_map<hash32_t, AssetManager::ManifestElement>& manifest) {
    const fs::path& baseDir = con::AssetTypeToPath(type);
    const auto contents = filesystem->getDirectoryContents(baseDir);
    
    for (const auto& p : contents) {
        AssetManager::ManifestElement me;
        me.type = type;
        me.path = baseDir / p;
        LOG_D(me.path);
    }
}

void AssetManager::buildManifestFromFilesystem() {
    FileSystem* filesystem = engine->getFileSystem();
    
    for (std::size_t i = 0; i < static_cast<std::size_t>(AssetType::COUNT); ++i) {
        addFilesToManifest(filesystem, static_cast<AssetType>(i), manifest);
    }
}

bool AssetManager::serializeMetadata(hash32_t nameHash, Serializer& file) {
    auto result = manifest.find(nameHash);
    
    if (result == manifest.end()) {
        return false;
    }
    
    const Metadata& meta = (*result).second.metadata;
    switch ((*result).second.type) {
        case AssetType::Animation:
            std::get<AnimationMetadata>(meta).serialize(file);
            break;
        case AssetType::Font:
            std::get<FontMetadata>(meta).serialize(file);
            break;
        case AssetType::Mesh:
            std::get<MeshMetadata>(meta).serialize(file);
            break;
        case AssetType::Audio:
            std::get<AudioMetadata>(meta).serialize(file);
            break;
        case AssetType::Texture:
            std::get<TextureMetadata>(meta).serialize(file);
            break;
        case AssetType::Material:
            std::get<CustomMetadata>(meta).serialize(file);
            break;
        case AssetType::ANY:
            throw std::logic_error("Any is not a type");
    }
    
    
    return true;
}

void AssetManager::collectGarbage() {
    for (auto& tm : typeManagers) {
        if (tm != nullptr) {
            tm->collectGarbage(GarbageCollectionRunPolicy::FullCollection);
        }
    }
}

void AssetManager::loadSystemAssets() {
    //
//     std::vector<int> v;
//     v.push_back(42);
}

void AssetManager::appendAssetToManifest(hash32_t nameHash, const fs::path& path, const Metadata& metadata) {
    if (!engine->isEditorMode()) {
        throw std::logic_error("This method can't be called when the engine is running in game mode.");
    }
    
    manifest[nameHash] = {path, static_cast<AssetType>(metadata.index()), false, metadata};
}

void AssetManager::removeAssetFromManifest(hash32_t nameHash) {
    if (!engine->isEditorMode()) {
        throw std::logic_error("This method can't be called when the engine is running in game mode.");
    }
    
    std::size_t elementsRemoved = manifest.erase(nameHash);
    const auto& loadedAsset = loadedAssets.find(nameHash);
    
    if (elementsRemoved > 0 && loadedAsset != loadedAssets.end()) {
        // TODO implement
        throw std::runtime_error("Removal of hot assets not yet implemented");
    }
}

void AssetManager::removeNonSystemAssetsFromManifest() {
    if (!engine->isEditorMode()) {
        throw std::logic_error("This method can't be called when the engine is running in game mode.");
    }
    
    // C++14 allows to erase individual elements when iterating through the container:
    // http://en.cppreference.com/w/cpp/container/unordered_map/erase
    for (auto it = manifest.begin(); it != manifest.end(); ) {
        const auto& loadedAsset = loadedAssets.find(it->first);
        
        if (loadedAsset != loadedAssets.end()) {
            // TODO implement
            throw std::runtime_error("Removal of hot assets not yet implemented");
        }
        
        if(it->second.systemAsset) {
            ++it;
        } else {
            it = manifest.erase(it);
        }
    }
}

const std::vector<MaterialPipelineDefinition> AssetManager::getMaterialPipelineDefinitions() const {
    std::vector<MaterialPipelineDefinition> pipelines;
    pipelines.reserve(availablePipelines.size());
    
    for (const auto& p : availablePipelines) {
        pipelines.push_back(p.second);
    }
    
    return pipelines;
}

void AssetManager::addOrUpdatePipeline(const MaterialPipelineDefinition& pipelineDefinition) {
    hash32_t nameHash = HS(pipelineDefinition.name.c_str());
    
    auto result = availablePipelines.find(nameHash);
    if (result != availablePipelines.end()) {
        LOG_W("No pipeline updates took place - the function has not been implemented yet!!!");
    }
    
    availablePipelines[nameHash] = pipelineDefinition;
}

bool AssetManager::removePipeline(hash32_t nameHash) {
    auto result = availablePipelines.find(nameHash);
    if (result != availablePipelines.end()) {
        availablePipelines.erase(result);
        LOG_W("No pipeline unloading took place - the function has not been implemented yet!!!");
        
        return true;
    } else {
        return false;
    }
}

void AssetManager::removeNonSystemPipelines() {
    LOG_W("No pipeline unloading took place - the function has not been implemented yet!!!");
}
}
