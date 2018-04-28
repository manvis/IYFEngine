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
#include "assets/loaders/MeshLoader.hpp"
#include "graphics/MeshComponent.hpp"
#include "core/Constants.hpp"
#include "core/Logger.hpp"
#include "core/Engine.hpp"
#include "core/filesystem/FileSystem.hpp"
#include "core/serialization/VirtualFileSystemSerializer.hpp"
#include "assets/typeManagers/FontTypeManager.hpp"
#include "assets/typeManagers/MeshTypeManager.hpp"
#include "assets/typeManagers/ShaderTypeManager.hpp"
#include "assets/typeManagers/TextureTypeManager.hpp"
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
    typeManagers[static_cast<std::size_t>(AssetType::Texture)] = std::unique_ptr<TextureTypeManager>(new TextureTypeManager(this));
    typeManagers[static_cast<std::size_t>(AssetType::Font)] = std::unique_ptr<FontTypeManager>(new FontTypeManager(this));
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

/// A helper class used when building the manifest.
struct FileDiscoveryResults {
    enum class Field {
        FilePath,
        TextMetadataPath,
        BinaryMetadataPath
    };
    
    inline FileDiscoveryResults() : nameHash(0) {}
    inline FileDiscoveryResults(hash32_t nameHash) : nameHash(nameHash) {}
    
    inline bool isComplete() const {
        return (!filePath.empty()) && wasMetadataFound();
    }
    
    inline bool wasMetadataFound() const {
        // We need a metadata file of ONE type. If no metadata files were found, we can't add the asset to the manifest.
        // If both types of metadata files are found, we don't know which one is valid.
        return (textMetadataPath.empty() && !binaryMetadataPath.empty()) || (!textMetadataPath.empty() && binaryMetadataPath.empty());
    }
    
    hash32_t nameHash;
    fs::path filePath;
    fs::path textMetadataPath;
    fs::path binaryMetadataPath;
};

inline void findAndSetFileDiscoveryResults(std::unordered_map<hash32_t, FileDiscoveryResults>& results, hash32_t nameHash, FileDiscoveryResults::Field field, const fs::path& path) {
    auto result = results.find(nameHash);
    
    if (result == results.end()) {
        FileDiscoveryResults fdr(nameHash);
        
        switch (field) {
            case FileDiscoveryResults::Field::FilePath:
                fdr.filePath = path;
                break;
            case FileDiscoveryResults::Field::TextMetadataPath:
                fdr.textMetadataPath = path;
                break;
            case FileDiscoveryResults::Field::BinaryMetadataPath:
                fdr.binaryMetadataPath = path;
                break;
        }
        
        results[nameHash] = std::move(fdr);
    } else {
        assert(result->second.nameHash == nameHash);
        
        switch (field) {
            case FileDiscoveryResults::Field::FilePath:
                result->second.filePath = path;
                break;
            case FileDiscoveryResults::Field::TextMetadataPath:
                result->second.textMetadataPath = path;
                break;
            case FileDiscoveryResults::Field::BinaryMetadataPath:
                result->second.binaryMetadataPath = path;
                break;
        }
    }
}

/// Checks if the name of the asset or metadata file is valid
inline bool isFileNameValid(const std::string& name) {
    for (char c : name) {
        if (c < 48 || c > 57) {
            return false;
        }
    }
    
    return true;
}

template <typename T>
inline T loadMetadata(const fs::path& path, bool isJSON) {
    T metadata;
    
    if (isJSON) {
        File jsonFile(path, File::OpenMode::Read);
        const auto wholeFile = jsonFile.readWholeFile();
        
        rj::Document document;
        document.Parse(wholeFile.first.get(), wholeFile.second);
        metadata.deserializeJSON(document);
    } else {
        VirtualFileSystemSerializer file(path, File::OpenMode::Read);
        metadata.deserialize(file);
    }
    
    assert(metadata.isComplete());
    
    return metadata;
}

/// Adds all assets of the specified type to the provided manifest
inline void addFilesToManifest(FileSystem* filesystem, AssetType type, std::unordered_map<hash32_t, AssetManager::ManifestElement>& manifest) {
    const fs::path& baseDir = con::AssetTypeToPath(type);
    const auto contents = filesystem->getDirectoryContents(baseDir);
    
    // Used to detect errors.
    // TODO Return this as well. It may be useful for debugging
    std::unordered_map<hash32_t, FileDiscoveryResults> results;
    
    for (const auto& p : contents) {
        const fs::path fullPath = baseDir / p;
        const std::string stem = fullPath.stem().generic_string();
        
        const bool validName = isFileNameValid(stem);
        if (!validName) {
            LOG_W("Found a file with a non-numeric name: " << fullPath << ". Skipping it.");
            continue;
        }
        
        // Can't check for other errors here. I'm quite certain that both 0 and ULLONG_MAX can be valid hash values.
        // I wish this had an error_code parameter, just like filesystem stuff.
        //
        // WARNING: Don't replace this with stoull. It won't solve the problem because it simply wraps strtoull.
        // Source: http://en.cppreference.com/w/cpp/string/basic_string/stoul
        const std::uint64_t parsedNumber = std::strtoull(stem.c_str(), nullptr, 10);
        if (parsedNumber > std::numeric_limits<std::uint32_t>::max()) {
            LOG_W("When converted to an unsigned integer, the numeric filename of " << fullPath << " does not fit in hash32_t. Skipping it.");
            continue;
        }
        
        hash32_t nameHash(parsedNumber);
        
        // We found an asset file, however, we only care about the metadata at the moment.
        if (fullPath.extension().empty()) {
            findAndSetFileDiscoveryResults(results, nameHash, FileDiscoveryResults::Field::FilePath, fullPath);
        } else if (fullPath.extension() == con::MetadataExtension) {
            findAndSetFileDiscoveryResults(results, nameHash, FileDiscoveryResults::Field::BinaryMetadataPath, fullPath);
        } else if (fullPath.extension() == con::TextMetadataExtension) {
            findAndSetFileDiscoveryResults(results, nameHash, FileDiscoveryResults::Field::TextMetadataPath, fullPath);
        } else {
            LOG_W("Found a file with an unexpected extension: " << fullPath << ". Skipping it.");
        }
    }
    
    std::size_t count = 0;
    for (const auto& result : results) {
        const bool hasFile = !result.second.filePath.empty();
        const bool hasTextMetadata = !result.second.textMetadataPath.empty();
        const bool hasBinaryMetadata = !result.second.binaryMetadataPath.empty();
        
        if (!result.second.isComplete()) {
            LOG_W("Couldn't add an item to the manifest. File + ONE type of metadata required, found this instead:\n\t" <<
                  "\n\tFile: " << (hasFile ? result.second.filePath : "NOT FOUND") <<
                  "\n\tText metadata:"  << (hasTextMetadata ? result.second.textMetadataPath : "NOT FOUND") <<
                  "\n\tBinary metadata: " << (hasBinaryMetadata ? result.second.binaryMetadataPath : "NOT FOUND"));
        }
        
        const bool isJSON = hasTextMetadata;
        const fs::path metadataPath = isJSON ? result.second.textMetadataPath : result.second.binaryMetadataPath;
        
        AssetManager::ManifestElement me;
        me.type = type;
        me.path = result.second.filePath;
        
        switch (type) {
            case AssetType::Animation:
                me.metadata = loadMetadata<AnimationMetadata>(metadataPath, isJSON);
                break;
            case AssetType::Mesh:
                me.metadata = loadMetadata<MeshMetadata>(metadataPath, isJSON);
                break;
            case AssetType::Texture:
                me.metadata = loadMetadata<TextureMetadata>(metadataPath, isJSON);
                break;
            case AssetType::Font:
                me.metadata = loadMetadata<FontMetadata>(metadataPath, isJSON);
                break;
            case AssetType::Audio:
                me.metadata = loadMetadata<AudioMetadata>(metadataPath, isJSON);
                break;
            case AssetType::Video:
                me.metadata = loadMetadata<VideoMetadata>(metadataPath, isJSON);
                break;
            case AssetType::World:
                me.metadata = loadMetadata<WorldMetadata>(metadataPath, isJSON);
                break;
            case AssetType::Script:
                me.metadata = loadMetadata<ScriptMetadata>(metadataPath, isJSON);
                break;
            case AssetType::Shader:
                me.metadata = loadMetadata<ShaderMetadata>(metadataPath, isJSON);
                break;
            case AssetType::Pipeline:
                me.metadata = loadMetadata<PipelineMetadata>(metadataPath, isJSON);
                break;
            case AssetType::Strings:
                me.metadata = loadMetadata<StringMetadata>(metadataPath, isJSON);
                break;
            case AssetType::Custom:
                me.metadata = loadMetadata<CustomMetadata>(metadataPath, isJSON);
                break;
            case AssetType::Material:
                me.metadata = loadMetadata<MaterialMetadata>(metadataPath, isJSON);
                break;
            case AssetType::COUNT:
                throw std::runtime_error("COUNT is not an asset type");
        }
        
        std::visit([&me](auto&& arg){me.systemAsset = arg.isSystemAsset();}, me.metadata);
        
        auto manifestItem = manifest.find(result.second.nameHash);
        if (manifestItem != manifest.end()) {
            throw std::runtime_error("Not implemented yet");
        }
        
        manifest[result.second.nameHash] = std::move(me);
        count++;
    }
    
    LOG_V("Added " << count << " " << con::AssetTypeToTranslationString(type) << " file(s) to the manifest.\n\tIt now stores metadata of " << manifest.size() << " file(s).");
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
        
        if (it->second.systemAsset) {
            ++it;
        } else {
            if (loadedAsset != loadedAssets.end()) {
                // TODO implement
                throw std::runtime_error("Removal of hot assets not yet implemented");
            }
            
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
