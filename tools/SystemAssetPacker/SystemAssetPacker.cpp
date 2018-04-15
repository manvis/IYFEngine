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

#include "SystemAssetPacker.hpp"
#include "core/Platform.hpp"
#include "core/filesystem/FileSystem.hpp"
#include "core/filesystem/File.hpp"
#include "core/Logger.hpp"
#include "core/Project.hpp"
#include "core/serialization/VirtualFileSystemSerializer.hpp"
#include "core/serialization/MemorySerializer.hpp"
#include "utilities/hashing/Hashing.hpp"
#include "utilities/Compression.hpp"

#include "assetImport/ConverterManager.hpp"

namespace iyf {
const std::string WrongDirError = "Failed to find required directories. Are you sure you're running the SystemAssetPacker from the build folder?";

SystemAssetPacker::SystemAssetPacker(char* argv) {
    filesystem = std::unique_ptr<FileSystem>(new FileSystem(true, argv, true));
    
    LOG_V("Starting system asset packer. Base dir: " << filesystem->getBaseDirectory());
    
    fs::path path(filesystem->getBaseDirectory());
    path /= "..";
    path /= "system";
    
    LOG_V("Expected asset dir: " << path.string());
    
    if (!fs::exists(path)) {
        throw std::runtime_error(WrongDirError);
    }
    
    filesystem->setWritePath(path.string());
    filesystem->addReadPath(path.string(), "", true);
}

SystemAssetPacker::~SystemAssetPacker() {}

void SystemAssetPacker::recursiveExport(const fs::path& path, const editor::ConverterManager& cm, PlatformIdentifier platformID) {
    const auto contents = filesystem->getDirectoryContents(path.generic_string());
    for (const auto& item : contents) {
        fs::path sourcePath = path / item;
        
        PHYSFS_Stat stat;
        if (filesystem->getFileSystemStatistics(sourcePath.generic_string(), stat) == 0) {
            LOG_E("PHYSFS reported an error (" << filesystem->getLastErrorText() << ") when trying to stat \"" << item << "\"");
            
            throw std::runtime_error("Failed to convert system assets because of a filesystem error. (Check log)");
        }
        
        if (stat.filetype == PHYSFS_FileType::PHYSFS_FILETYPE_DIRECTORY) {
            LOG_V("Found a system asset subdirectory: " << sourcePath);
            recursiveExport(sourcePath, cm, platformID);
        } else {
            if (sourcePath.extension() == con::ImportSettingsExtension) {
                continue;
            }
            
            const AssetType type = cm.getAssetType(sourcePath);
            
            // System localization strings require a different path
            fs::path destinationPath = (type == AssetType::Strings) ? cm.makeFinalPathForSystemStrings(sourcePath, platformID)
                                                                    : cm.makeFinalPathForAsset(sourcePath, type, platformID);
            
            LOG_V("IMPORTING FILE: " << sourcePath <<
                  "\n\t\tHash: " << HS(sourcePath.generic_string()) <<
                  "\n\t\tType: " << con::AssetTypeToTranslationString(type) <<
                  "\n\t\tDestination: " << destinationPath);
            
            std::vector<editor::ImportedAssetData> iad;
            std::unique_ptr<editor::ConverterState> converterState = cm.initializeConverter(sourcePath, platformID);
            
            if (converterState == nullptr) {
                LOG_E("Failed to initialize the ConversionSettings for a system asset: " << sourcePath);
                throw std::runtime_error("Failed to initialize the ConversionSettings for a system asset (Check log)");
            }
            
            if (converterState->getType() == AssetType::Strings) {
                editor::LocalizationStringConverterState* lcs = dynamic_cast<editor::LocalizationStringConverterState*>(converterState.get());
                lcs->systemTranslations = true;
            }
//             if (converterState->getType() == AssetType::Texture) {
//                 LOG_V("Requesting debug output of textures");
//                 converterState->setDebugOutputRequested(true);
//                 
// //                 editor::TextureConverterState* tcs = dynamic_cast<editor::TextureConverterState*>(converterState.get());
// //                 assert(tcs != nullptr);
// //                 tcs->sRGBSource = false;
// //                 tcs->importMode = TextureImportMode::NormalMap;
//             }
            
            if (!cm.convert(*converterState)) {
                LOG_E("Failed to convert a system asset: " << sourcePath);
                throw std::runtime_error("Failed to convert a system asset (Check log)");
            }
            
            for (const auto& a : converterState->getImportedAssets()) {
                std::visit([](auto&& f) { if (f.getFileHash() == 0) { throw std::runtime_error("Failed to examine the metadata of an imported asset");}}, a.getMetadata());
            }
        }
    }
}

void SystemAssetPacker::pack() {
    const fs::path platformDataBasePath = fs::path("platforms");
    const editor::ConverterManager cm(filesystem.get(), platformDataBasePath);
    
    // TODO different platforms should use different packages. Linux and Windows share the same assets (e.g. BC compressed textures).
    /// However, once Android is supported, we will need to add support for ETC2 compressed textures.
    /// for (platformID : platforms) ...
    const PlatformIdentifier currentPlatform = PlatformIdentifier::Linux_Desktop_x86_64;
    
    const fs::path platformDataPath = cm.getAssetDestinationPath(currentPlatform);
    
    const fs::path realPlatformDataPath = filesystem->getRealDirectory(platformDataPath.generic_string());
    if (!realPlatformDataPath.empty()) {
        LOG_D("Removing asset data processed by a previous run from " << realPlatformDataPath);
        fs::remove_all(realPlatformDataPath);
    }
    
    const fs::path pathToCreate = filesystem->getCurrentWriteDirectory() / platformDataPath;
    
    LOG_D("Creating asset data directories for current platform: " << pathToCreate);
    fs::create_directories(pathToCreate);
    
    if (!Project::CreateImportedAssetDirectories(pathToCreate)) {
        throw std::runtime_error("Failed to create imported asset directories");
    }
    
    recursiveExport("raw/system", cm, currentPlatform);
    
    const fs::path systemArchiveName = ("system" + con::PackFileExtension);
    const fs::path archivePath = pathToCreate / systemArchiveName;
    
    std::vector<util::PathToCompress> pathsToCompress;
    pathsToCompress.reserve(100);
    
    for (const auto& d : fs::recursive_directory_iterator(pathToCreate)) {
        if (!fs::is_directory(d)) {
            const fs::path relativePath = d.path().lexically_relative(pathToCreate);
            pathsToCompress.emplace_back(d, relativePath);
        }
    }
    
    pathsToCompress.emplace_back(filesystem->getCurrentWriteDirectory() / con::EngineBaseConfigFile, con::EngineBaseConfigFile);
//     pathsToCompress.emplace_back(filesystem->getCurrentWriteDirectory() / con::ProjectBaseConfigFile, con::ProjectBaseConfigFile);
    
    if (util::CompressFileListToZip(pathsToCompress, archivePath, util::CompressionLevel::Best)) {
        LOG_V("Successfully compressed system files for the " << con::PlatformIdentifierToName(currentPlatform) << " platform.");
    } else {
        LOG_E("Failed to compress system files for the " << con::PlatformIdentifierToName(currentPlatform) << " platform.");
        throw std::runtime_error("Failed to compress system files (check log)");
    }
    
    // Copy the files for the current platform next to the executable
    if (currentPlatform == con::GetCurrentPlatform()) {
        LOG_V("Copying the files for current platfom to " << filesystem->getBaseDirectory());
        fs::copy_file(archivePath, filesystem->getBaseDirectory() / systemArchiveName, fs::copy_option::overwrite_if_exists);
        
        // Create a project file for the editor, otherwise, it won't start.
        if (!Project::CreateProjectFile(filesystem->getBaseDirectory(), "IYFEditor", "The IYFEngine Team", "en_US", con::EditorVersion)) {
            LOG_E("Failed to create the project file in " << filesystem->getBaseDirectory());
            throw std::runtime_error("Failed to create the project file ");
        }
    }
}

}
