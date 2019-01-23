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
#include "assets/metadata/AnimationMetadata.hpp"
#include "assets/metadata/MeshMetadata.hpp"
#include "assets/metadata/TextureMetadata.hpp"
#include "assets/metadata/FontMetadata.hpp"
#include "assets/metadata/AudioMetadata.hpp"
#include "assets/metadata/VideoMetadata.hpp"
#include "assets/metadata/ScriptMetadata.hpp"
#include "assets/metadata/ShaderMetadata.hpp"
#include "assets/metadata/StringMetadata.hpp"
#include "assets/metadata/CustomMetadata.hpp"
#include "assets/metadata/MaterialTemplateMetadata.hpp"
#include "assets/metadata/MaterialInstanceMetadata.hpp"

#include "graphics/MeshComponent.hpp"
#include "core/Constants.hpp"
#include "core/Logger.hpp"
#include "core/Platform.hpp"
#include "core/Engine.hpp"
#include "core/filesystem/FileSystem.hpp"
#include "core/serialization/VirtualFileSystemSerializer.hpp"
#include "core/interfaces/TextSerializable.hpp"
#include "assets/typeManagers/TypeManager.hpp"
#include "assets/typeManagers/FontTypeManager.hpp"
#include "assets/typeManagers/MeshTypeManager.hpp"
#include "assets/typeManagers/ShaderTypeManager.hpp"
#include "assets/typeManagers/TextureTypeManager.hpp"
#include "utilities/DataSizes.hpp"
#include "utilities/FileInDir.hpp"

#include <climits>

namespace iyf {
namespace con {
const std::chrono::milliseconds MinAsyncLoadWindow = std::chrono::milliseconds(2);
const std::chrono::milliseconds MaxAsyncLoadWindow = std::chrono::milliseconds(60);
}

using namespace iyf::literals;

AssetManager::AssetManager(Engine* engine) : engine(engine), asyncLoadWindow(con::MinAsyncLoadWindow), isInit(false) {
    if (std::atomic<std::uint32_t>::is_always_lock_free) {
        LOG_V("std::uint32_t is lock free on this system");
    } else {
        LOG_W("std::uint32_t is NOT lock free on this system");
    }
    
    AssetType biggestAssetMetadata = AssetType::COUNT;
    std::size_t maxSize = 0;
    
    std::stringstream ss;
    ss << "Metadata type sizes: ";
    for (std::uint64_t i = 0; i < static_cast<std::uint64_t>(AssetType::COUNT); ++i) {
        const AssetType type = static_cast<AssetType>(i);
        const std::size_t size = Metadata::GetAssetMetadataSize(type);
        
        ss << "\n\t" << con::AssetTypeToTranslationString(type) << " " << size;
        
        if (size > maxSize) {
            maxSize = size;
            biggestAssetMetadata = type;
        }
    }
    ss << "\n\tBIGGEST METADATA OBJECT: " << con::AssetTypeToTranslationString(biggestAssetMetadata) << ", " << maxSize << " bytes.";
    LOG_D(ss.str())
    
    editorMode = engine->isEditorMode();
}

AssetManager::~AssetManager() { }

AssetType AssetManager::GetAssetTypeFromExtension(const fs::path& pathToFile) {
    static const std::unordered_map<std::string, AssetType> ExtensionToType = {
        {".ttf", AssetType::Font},
        {".otf", AssetType::Font},
        {".bmp", AssetType::Texture},
        {".psd", AssetType::Texture},
        {".png", AssetType::Texture},
        {".gif", AssetType::Texture},
        {".tga", AssetType::Texture},
        {".jpg", AssetType::Texture},
        {".jpeg", AssetType::Texture},
        {".hdr", AssetType::Texture},
        {".pic", AssetType::Texture},
        {".ppm", AssetType::Texture},
        {".pgm", AssetType::Texture},
        // TODO. Test and enable more formats. We're using Assimp. It suports MANY different mesh formats, however, I've only done extensive testing with Blender exported fbx and dae files.
        {".fbx", AssetType::Mesh},
        {".dae", AssetType::Mesh},
    //    {".3ds", AssetType::Mesh},
    //    {".blend", AssetType::Mesh},
        // TODO. support (some of?) these. Currently, the engine isn't able to convert anything and, to be honest, I don't know much about audio format patents and licences. 
        // I know that MP3 is patent protected and that you need to pay for a licence if you use it. I also know that ogg is just a container and that, while it was designed to
        // be used with free codecs, it's possible to squeeze in things like mp3s into it. 
        // I don't know if you need to get a the license if you only convert mp3 to vorbis (opus?). I do know that converting from one compressed format to another is a bad idea because quality
        // suffers, so I should at least try to add support free uncompressed formats (e.g., FLAC).
        // And seriously, what about opus? Should I use it instead of vorbis?
    //     {".opus", AssetType::Audio},
    //    {".flac", AssetType::Audio},
    //    {".mp3", AssetType::Audio}, // Probably not.
    //    {".waw", AssetType::Audio},
    //    {".ogg", AssetType::Audio},
        {".vert", AssetType::Shader},
        {".tesc", AssetType::Shader},
        {".tese", AssetType::Shader},
        {".geom", AssetType::Shader},
        {".frag", AssetType::Shader},
        {".comp", AssetType::Shader},
        {".csv", AssetType::Strings},
        {con::MaterialTemplateFormatExtension(), AssetType::MaterialTemplate},
    };
    std::string extension = pathToFile.extension().string();
    
    if (extension.empty()) {
        return AssetType::Custom;
    }
    
    // TODO can extensions be not ASCII? Can we mess something up here?
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    const auto it = ExtensionToType.find(extension);
    if (it == ExtensionToType.end()) {
        return AssetType::Custom;
    } else {
        return it->second;
    }
}

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
    inline FileDiscoveryResults(StringHash nameHash) : nameHash(nameHash) {}
    
    inline bool isComplete() const {
        return (!filePath.empty()) && wasMetadataFound();
    }
    
    inline bool wasMetadataFound() const {
        // We need a metadata file of ONE type. If no metadata files were found, we can't add the asset to the manifest.
        // If both types of metadata files are found, we don't know which one is valid.
        return (textMetadataPath.empty() && !binaryMetadataPath.empty()) || (!textMetadataPath.empty() && binaryMetadataPath.empty());
    }
    
    StringHash nameHash;
    fs::path filePath;
    fs::path textMetadataPath;
    fs::path binaryMetadataPath;
};

inline void findAndSetFileDiscoveryResults(std::unordered_map<StringHash, FileDiscoveryResults>& results, StringHash nameHash, FileDiscoveryResults::Field field, const fs::path& path) {
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
    if (name.empty()) {
        return false;
    }

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

/// Loads the metadata and creates a AssetManager::ManifestElement from it
inline AssetManager::ManifestElement buildManifestElement(AssetType type, bool isJSON, const fs::path& metadataPath, const fs::path& filePath) {
    AssetManager::ManifestElement me;
    me.type = type;
    me.path = filePath;
    
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
        case AssetType::Script:
            me.metadata = loadMetadata<ScriptMetadata>(metadataPath, isJSON);
            break;
        case AssetType::Shader:
            me.metadata = loadMetadata<ShaderMetadata>(metadataPath, isJSON);
            break;
        case AssetType::Strings:
            me.metadata = loadMetadata<StringMetadata>(metadataPath, isJSON);
            break;
        case AssetType::Custom:
            me.metadata = loadMetadata<CustomMetadata>(metadataPath, isJSON);
            break;
        case AssetType::MaterialTemplate:
            me.metadata = loadMetadata<MaterialTemplateMetadata>(metadataPath, isJSON);
            break;
        case AssetType::MaterialInstance:
            me.metadata = loadMetadata<MaterialInstanceMetadata>(metadataPath, isJSON);
            break;
        case AssetType::COUNT:
            throw std::runtime_error("COUNT is not an asset type");
    }
    
    me.systemAsset = me.metadata.getBase().isSystemAsset();
    
    return me;
}

/// Adds all assets of the specified type to the provided manifest
///
/// \warning Make sure this is always protected by a mutex
inline void addFilesToManifest(FileSystem* filesystem, AssetType type, std::unordered_map<StringHash, AssetManager::ManifestElement>& manifest) {
    const fs::path& baseDir = con::AssetTypeToPath(type);
    LOG_V("Examining contents of \"" << baseDir.generic_string() << "\"");
    const auto contents = filesystem->getDirectoryContents(baseDir);
    
    // Used to detect errors.
    // TODO Return this as well. It may be useful for debugging
    std::unordered_map<StringHash, FileDiscoveryResults> results;
    
    for (const auto& p : contents) {
        const fs::path fullPath = baseDir / p;
        const std::string stem = fullPath.stem().generic_string();
        
        const bool validName = isFileNameValid(stem);
        if (!validName) {
            LOG_W("Found a file with a non-numeric name: " << fullPath << ". Skipping it.");
            continue;
        }
        
        const char* stemPtr = stem.c_str();
        char* end;
        const std::uint64_t parsedNumber = std::strtoull(stemPtr, &end, 10);
                
        if (stemPtr == end) {
            LOG_W("strtoull encountered an error when processing " << fullPath << ". Skipping it.");
        }
        
        if (errno == ERANGE) {
            LOG_W("strtoull encountered an error when processing " << fullPath << ". Skipping it.");
            
            errno = 0;
            continue;
        }
        
        StringHash nameHash(parsedNumber);
        //LOG_V("FOUND file: " << parsedNumber << " " << fullPath);
        
        // We found an asset file, however, we only care about the metadata at the moment.
        if (fullPath.extension().empty()) {
            findAndSetFileDiscoveryResults(results, nameHash, FileDiscoveryResults::Field::FilePath, fullPath);
        } else if (fullPath.extension() == con::MetadataExtension()) {
            findAndSetFileDiscoveryResults(results, nameHash, FileDiscoveryResults::Field::BinaryMetadataPath, fullPath);
        } else if (fullPath.extension() == con::TextMetadataExtension()) {
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
        
        auto me = buildManifestElement(type, isJSON, metadataPath, result.second.filePath);
        
        auto manifestItem = manifest.find(result.second.nameHash);
        if (manifestItem != manifest.end() && !manifestItem->second.systemAsset) {
            throw std::runtime_error("A non-system asset hasn't been cleaned up.");
        }
        
        manifest[result.second.nameHash] = std::move(me);
        count++;
    }
    
    LOG_V("Added " << count << " " << con::AssetTypeToTranslationString(type) << " file(s) to the manifest.\n\tIt now stores metadata of " << manifest.size() << " file(s).");
}

void AssetManager::buildManifestFromFilesystem() {
    FileSystem* filesystem = engine->getFileSystem();
    
    LOG_V("Building the manifest using the following search path:\n\t" << filesystem->logSearchPath("\n\t"));
    
    // TODO automatically convert or delete items that were added or removed while the engine was off (e.g. thanks
    // to version control).
    std::lock_guard<std::mutex> manifestLock(manifestMutex);
    for (std::size_t i = 0; i < static_cast<std::size_t>(AssetType::COUNT); ++i) {
        addFilesToManifest(filesystem, static_cast<AssetType>(i), manifest);
    }
}

bool AssetManager::serializeMetadata(StringHash nameHash, Serializer& file) {
    std::lock_guard<std::mutex> manifestLock(manifestMutex);
    
    auto result = manifest.find(nameHash);
    
    if (result == manifest.end()) {
        return false;
    }
    
    const Metadata& meta = (*result).second.metadata;
    meta.getBase().serialize(file);
    
    return true;
}

void AssetManager::collectGarbage() {
    for (auto& tm : typeManagers) {
        if (tm != nullptr) {
            tm->collectGarbage(GarbageCollectionRunPolicy::FullCollection);
        }
    }
}

void AssetManager::enableLoadedAssets() {
    const std::chrono::nanoseconds window = asyncLoadWindow;
    const auto start = std::chrono::steady_clock::now();
    auto now = start;

    for (auto& tm : typeManagers) {
        if (tm != nullptr) {
            const bool canBatch = tm->canBatchAsyncLoadedAssets();
            
            if (canBatch) {
                while ((tm->hasAssetsToEnable() == AssetsToEnableResult::HasAssetsToEnable) &&
                       ((now + tm->estimateBatchOperationDuration()) - start < window)) {
                    tm->enableAsyncLoadedAsset(true);
                    
                    now = std::chrono::steady_clock::now();
                }
            } else {
                while ((tm->hasAssetsToEnable() == AssetsToEnableResult::HasAssetsToEnable) &&
                       (now - start < window)) {
                    tm->enableAsyncLoadedAsset(false);
                    
                    now = std::chrono::steady_clock::now();
                }
            }
            
            if (canBatch) {
                // This should always happen. If nothing was added, the TypeManager should avoid any expensive 
                // operations.
                tm->executeBatchOperations();
                now = std::chrono::steady_clock::now();
            }
        }
    }
    
    // TODO should I sleep or something to avoid inconsistent frame rates?
}

void AssetManager::setAsyncLoadWindow(std::chrono::milliseconds loadWindow) {
    asyncLoadWindow = std::clamp(loadWindow, con::MinAsyncLoadWindow, con::MaxAsyncLoadWindow);
}

std::chrono::milliseconds AssetManager::getAsyncLoadWindow() const {
    return asyncLoadWindow;
}

void AssetManager::loadSystemAssets() {
    //
//     std::vector<int> v;
//     v.push_back(42);
    // TODO FIXME
    LOG_W("SYSTEM ASSETS NOT PRELOADED");
}

static inline fs::path MakeMetadataPathName(const fs::path& filePath, bool binary) {
    fs::path result = filePath;
    if (binary) {
        result += con::MetadataExtension();
    } else {
        result += con::TextMetadataExtension();
    }
    
    return result;
}

static inline std::optional<StringHash> ValidateAndHashPath(const FileSystem* fs, const fs::path& path) {
    if (path.empty()) {
        LOG_W("Couldn't process the file because an empty path was provided.");
        return std::nullopt;
    }
    
    if (!path.extension().empty()) {
        LOG_W("Couldn't process \"" << path << "\". A numeric filename without extension is required.");
        return std::nullopt;
    }
    
    const std::string stem = path.stem().generic_string();
    const bool validName = isFileNameValid(stem);
    if (!validName) {
        LOG_W("Couldn't process \"" << path << "\". It has a non-numeric name.");
        return std::nullopt;
    }
    
    if (!fs->exists(path)) {
        LOG_W("Couldn't process \"" << path << "\". File does not exist.");
        return std::nullopt;
    }
    

    const char* stemPtr = stem.c_str();
    char* end;
    const std::uint64_t parsedNumber = std::strtoull(stemPtr, &end, 10);
    
    if (stemPtr == end) {
        LOG_W("strtoull encountered an error when processing " << path << ". Skipping it.");
    }
    
    if (errno == ERANGE) {
        LOG_W("When parsed the filename " << path << " does not fit in std::uint64_t. Skipping it.");
        
        errno = 0;
        return std::nullopt;
    }
    
    return StringHash(parsedNumber);
}

void AssetManager::requestAssetRefresh(AssetType type, const fs::path& path) {
    if (!editorMode) {
        throw std::logic_error("This method can't be used when the engine is running in game mode.");
    }
    
    const FileSystem* fs = engine->getFileSystem();
    
    auto validationResult = ValidateAndHashPath(fs, path);
    if (!validationResult) {
        return;
    }
    
    const StringHash nameHash = *validationResult;
    
    const fs::path binaryMetadataPath = MakeMetadataPathName(path, true);
    const fs::path textMetadataPath = MakeMetadataPathName(path, false);
    
    // Don't move these file operations to any other thread. Editor APIs are kept synchronous on purpose
    // to avoid race conditions.
    bool binaryMetadataExists = fs->exists(binaryMetadataPath);
    bool textMetadataExists = fs->exists(textMetadataPath);
    
    if (!(binaryMetadataExists || textMetadataExists)) {
        LOG_W("Couldn't add \"" << path << "\" to the manifest. No metadata exists for it.");
        return;
    }
    
    if (binaryMetadataExists && textMetadataExists) {
        LOG_W("Couldn't add \"" << path << "\" to the manifest. It has both text and binary metadata and it's impossible to know which one is valid.");
        return;
    }
    
    AssetManager::ManifestElement me = buildManifestElement(type, textMetadataExists, textMetadataExists ? textMetadataPath : binaryMetadataPath, path);
    //manifest[nameHash] = std::move(me);
    auto manifestElement = manifest.insert_or_assign(nameHash, std::move(me));
    if (manifestElement.second) {
        LOG_V("Inserted a new element into the manifest");
    } else {
        LOG_V("Updated an existing element of the manifest");
    }
    
    std::lock_guard<std::mutex> manifestLock(manifestMutex);
    std::lock_guard<std::mutex> assetLock(loadedAssetListMutex);
    
    const auto& loadedAsset = loadedAssets.find(nameHash);
    if (loadedAsset != loadedAssets.end()) {
        const AssetType type = loadedAsset->second.first;
        const std::uint32_t id = loadedAsset->second.second;
        const Metadata& metadata = manifestElement.first->second.metadata;
        
        typeManagers[static_cast<std::size_t>(type)]->refresh(nameHash, path, metadata, id);
    }
}

void AssetManager::requestAssetDeletion(const fs::path& path, [[maybe_unused]] bool isDir) {
    if (!editorMode) {
        throw std::logic_error("This method can't be used when the engine is running in game mode.");
    }
    
    std::lock_guard<std::mutex> manifestLock(manifestMutex);
    std::lock_guard<std::mutex> assetLock(loadedAssetListMutex);
    
    const FileSystem* fs = engine->getFileSystem();
    
    if (isDir) {
#ifndef NDEBUG
        // TODO at least so far (on Linux) I get all file deletion events from inotify BEFORE I get the directory deletion
        // event. Unfortunately, I have to store all events I receive in a map because I need to track if a file is still being
        // modified by an external program. That is, for my purposes, a file's STATE is more important than the event order.
        //
        // Since using a map screws up the order, it is possible that a directory deletion event arrives BEFORE file deletion events,
        // which renders the following check useless. However, since directories are NOT assets in my engine, this SHOULDN'T matter, for
        // as long as all file deletion events arrive eventually.
        //
        // What I want you to do future me (or my helper) is to make sure the events ALWAYS arrive on all platforms.
//         for (const auto& me : manifest) {
//             fs::path src;
//             bool inDir = std::visit([&path, &src](auto&& meta) {
//                 src = meta.getSourceAssetPath();
//                 return util::FileInDir(path, src);
//             }, me.second.metadata);
//             
//             if (inDir) {
//                 LOG_V("File " << src << " not removed from the " << path << " directory before its deletion.");
//             }
//         }
#endif // NDEBUG
    } else {
        auto validationResult = ValidateAndHashPath(fs, path);
        if (!validationResult) {
            return;
        }
        
        const StringHash nameHash = *validationResult;
        
        const auto& loadedAsset = loadedAssets.find(nameHash);
        if (loadedAsset != loadedAssets.end()) {
            LOG_W("The file (" << path << ") that was removed had live references. If you don't find "
                "and fix them, they will be replaced with missing assets next time you start the editor.");
        }
        
        // All metadata operations are synchronous in editor mode
        const fs::path binaryMetadataPath = MakeMetadataPathName(path, true);
        const fs::path textMetadataPath = MakeMetadataPathName(path, false);
        
        bool binaryMetadataExists = fs->exists(binaryMetadataPath);
        if (binaryMetadataExists) {
            fs->remove(binaryMetadataPath);
        }
        
        bool textMetadataExists = fs->exists(textMetadataPath);
        if (textMetadataExists) {
            fs->remove(textMetadataPath);
        }
        
        fs->remove(path);
    }
}

void AssetManager::requestAssetMove(const fs::path& sourcePath, const fs::path& destinationPath, [[maybe_unused]] bool isDir) {
    if (!editorMode) {
        throw std::logic_error("This method can't be used when the engine is running in game mode.");
    }
    
    std::lock_guard<std::mutex> manifestLock(manifestMutex);
    std::lock_guard<std::mutex> assetLock(loadedAssetListMutex);
    
    LOG_D("Beginning asset data move for " << (isDir ? "directory" : "file") << ". Moving from " << sourcePath << " to " << destinationPath);
    
    // Can't change the key immediately. It would mess up the iteration order.
    std::vector<std::pair<StringHash, StringHash>> fileMoves;
    fileMoves.reserve(64);
    
    for (auto& me : manifest) {
        MetadataBase& meta = me.second.metadata.getBase();
        assert(meta.getMetadataSource() == MetadataSource::JSON);
        
        const fs::path currentSourceAssetPath = meta.getSourceAssetPath();
        const bool inDir = util::FileInDir(sourcePath, currentSourceAssetPath);
        
        const std::string currentSourceAssetPathString = currentSourceAssetPath.generic_string();
        const std::string sourceDirectoryString = sourcePath.generic_string();
        
        if (inDir) {
            assert(currentSourceAssetPathString.compare(0, sourceDirectoryString.length(), sourceDirectoryString) == 0);
            
            const FileSystem* fs = engine->getFileSystem();
            const fs::path fullNewPath = destinationPath / fs::path(currentSourceAssetPathString.substr(sourceDirectoryString.length()));
            
            const StringHash currentSourceHash = ComputeNameHash(currentSourceAssetPath);
            const StringHash newPathHash = ComputeNameHash(fullNewPath);
            
            assert(currentSourceHash == me.first);
            
            fs::path postMoveAssetPath(me.second.path.parent_path());
            postMoveAssetPath /= std::to_string(newPathHash.value());
            
            if (checkForHashCollisionImpl(newPathHash, fullNewPath)) {
                LOG_W("Couldn't move " << currentSourceAssetPathString << " (a.k.a. " << me.second.path << ") to " <<
                    fullNewPath << " (a.k.a. " << postMoveAssetPath << "). A hash collision has occured. You'll need  "
                    "to rename and re-import the file. Moreover, references to this file won't be updated.");
                continue;
            }
            
            if (!fs->exists(me.second.path)) {
                LOG_W("Couldn't move " << currentSourceAssetPathString << " (a.k.a. " << me.second.path << ") to " <<
                    fullNewPath << " (a.k.a. " << postMoveAssetPath << "). File does not exist.");
                continue;
            }
            
            // The paths in the virtual filesystem are used to retrieve and serialize metadata
            const fs::path moveSourceVirtualFS = me.second.path;
            fs::path moveDestinationVirtualFS = moveSourceVirtualFS.parent_path();
            moveDestinationVirtualFS /= std::to_string(newPathHash.value());
            
            // The paths in the real filesystem are used for actual file moves.
            const fs::path fullMoveSource = fs->getRealDirectory(moveSourceVirtualFS);
            assert(!fullMoveSource.empty());
            
            fs::path fullMoveDestination = fullMoveSource.parent_path();
            fullMoveDestination /= std::to_string(newPathHash.value());
            
            fs->rename(fullMoveSource, fullMoveDestination);
            
            // Retrieve metadata
            const fs::path binaryMetadataPath = MakeMetadataPathName(moveSourceVirtualFS, true);
            const fs::path textMetadataPath = MakeMetadataPathName(moveSourceVirtualFS, false);
            
//             LOG_D(currentSourceAssetPathString << "\n\t" << fullNewPath.generic_string() << "\n\t" << 
//                   me.second.path << "\n\t" << postMoveAssetPath << "\n\t" <<
//                   binaryMetadataPath << "\n\t" << textMetadataPath);
            
            bool binaryMetadataExists = fs->exists(binaryMetadataPath);
            if (binaryMetadataExists) {
                bool removed = fs->remove(binaryMetadataPath);
                if (!removed) {
                    LOG_E("Failed to remove a binary metadata file: " << binaryMetadataPath << "\n\tLast filesystem error: " << fs->getLastErrorText());
                    
                    throw std::logic_error("Failed to remove a metadata file. Check log.");
                }
            }
            
            bool textMetadataExists = fs->exists(textMetadataPath);
            if (textMetadataExists) {
                bool removed = fs->remove(textMetadataPath);
                if (!removed) {
                    LOG_E("Failed to remove a text metadata file: " << textMetadataPath << "\n\tLast filesystem error: " << fs->getLastErrorText());
                    
                    throw std::logic_error("Failed to remove a metadata file. Check log.");
                }
            }
            
            // Update the paths stored in the metadata object
            me.second.path = postMoveAssetPath;
            meta.sourceAsset = fullNewPath;
            
            // Save the updated metadata
            if (textMetadataExists || (!textMetadataExists && !binaryMetadataExists)) {
                const std::string jsonString = meta.getJSONString();
                
                const fs::path newMetadataPath = MakeMetadataPathName(moveDestinationVirtualFS, false);
                File metadataOutput(newMetadataPath, File::OpenMode::Write);
                metadataOutput.writeString(jsonString);
            } else {
                const fs::path newMetadataPath = MakeMetadataPathName(moveDestinationVirtualFS, true);
                VirtualFileSystemSerializer serializer(newMetadataPath, File::OpenMode::Write);
                meta.serialize(serializer);
            }

            auto loadedAsset = loadedAssets.find(currentSourceHash);
            if (loadedAsset != loadedAssets.end()) {
                std::uint32_t loadedAssetID = loadedAsset->second.second;
                
                auto node = loadedAssets.extract(currentSourceHash);
                assert(!node.empty());
                
                node.key() = newPathHash;
                loadedAssets.insert(std::move(node));
                
                typeManagers[static_cast<std::size_t>(me.second.type)]->notifyMove(loadedAssetID, currentSourceHash, newPathHash);
            }
            
            fileMoves.emplace_back(std::make_pair(currentSourceHash, newPathHash));
        }
    }
    
    for (const auto fm : fileMoves) {
        auto node = manifest.extract(fm.first);
        assert(!node.empty());
        
        node.key() = fm.second;
        manifest.insert(std::move(node));
    }
}

void AssetManager::appendAssetToManifest(StringHash nameHash, const fs::path& path, const Metadata& metadata) {
    if (!editorMode) {
        throw std::logic_error("This method can't be used when the engine is running in game mode.");
    }
    
    std::lock_guard<std::mutex> manifestLock(manifestMutex);
    manifest[nameHash] = {path, metadata.getAssetType(), false, metadata};
}

void AssetManager::removeAssetFromManifest(StringHash nameHash) {
    if (!editorMode) {
        throw std::logic_error("This method can't be used when the engine is running in game mode.");
    }
    
    std::lock_guard<std::mutex> manifestLock(manifestMutex);
    std::lock_guard<std::mutex> assetLock(loadedAssetListMutex);
    
    std::size_t elementsRemoved = manifest.erase(nameHash);
    const auto& loadedAsset = loadedAssets.find(nameHash);
    
    if (elementsRemoved > 0 && loadedAsset != loadedAssets.end()) {
        // TODO implement
        throw std::runtime_error("Removal of hot assets not yet implemented");
    }
}

void AssetManager::removeNonSystemAssetsFromManifest() {
    if (!editorMode) {
        throw std::logic_error("This method can't be used when the engine is running in game mode.");
    }
    
    std::lock_guard<std::mutex> manifestLock(manifestMutex);
    std::lock_guard<std::mutex> assetLock(loadedAssetListMutex);
    
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

std::optional<fs::path> AssetManager::checkForHashCollision(StringHash nameHash, const fs::path& checkPath) const {
    if (!editorMode) {
        throw std::logic_error("This method can only be used when the engine is running in editor mode.");
    }
    
    std::unique_lock<std::mutex> lock(manifestMutex);
    
    return checkForHashCollisionImpl(nameHash, checkPath);
}

std::optional<fs::path> AssetManager::checkForHashCollisionImpl(StringHash nameHash, const fs::path& checkPath) const {
    auto result = manifest.find(nameHash);
    if (result != manifest.end()) {
        const fs::path foundPath = result->second.metadata.getBase().getSourceAssetPath();
        if (foundPath != checkPath) {
            return foundPath;
        }
    }
    
    return std::nullopt;
}
}
