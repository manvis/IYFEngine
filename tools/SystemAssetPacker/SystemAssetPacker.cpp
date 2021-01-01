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
#include "io/DefaultFileSystem.hpp"
#include "core/filesystem/VirtualFileSystem.hpp"
#include "io/File.hpp"
#include "io/serialization/FileSerializer.hpp"
#include "io/serialization/MemorySerializer.hpp"
#include "logging/Logger.hpp"
#include "core/Project.hpp"
#include "utilities/hashing/Hashing.hpp"
#include "utilities/Compression.hpp"

#include "assetImport/ConverterManager.hpp"
#include "assetImport/converterStates/LocalizationStringConverterState.hpp"

#include "fmt/ostream.h"
#include <iostream>

namespace iyf {
namespace fs = std::filesystem;

const std::string WrongDirError = "Failed to find required directories. Are you sure you're running the SystemAssetPacker from the build folder?";

SystemAssetPacker::SystemAssetPacker(int argc, char* argv[]) : isValid(true) {
    if (argc != 3 || argv[1] != std::string_view("--output")) {
        std::cout << "System asset packer must be called like this: ./SystemAssetPacker --output SOME-OUTPUT-PATH\n";
        isValid = false;
        return;
    }
    
    const std::string_view temp = argv[2];
    const std::size_t lastChar = temp.size() - 1;
    const bool startsWithQuotes = temp[0] == '\"';
    const bool endsWithQuotes = temp[lastChar] == '\"';
    
    if (startsWithQuotes != endsWithQuotes) {
        std::cout << "Path needs to be quoted from both sides or not quoted at all\n";
        isValid = false;
        return;
    }

    auto& fs = DefaultFileSystem::Instance();
    
    outputDir = temp;

    FileSystemResult result;
    const bool exists = fs.exists(outputDir, result);
    if (result != FileSystemResult::Success) {
        std::cout << "Failed to determine if output dir " << outputDir << " exists\n";
        isValid = false;
        return;
    }

    if (!exists) {
        std::cout << "Output dir " << outputDir << " does not exist\n";
        isValid = false;
        return;
    }
    
    const bool isDir = fs.isDirectory(outputDir, result);

    if (result != FileSystemResult::Success) {
        std::cout << "Failed to determine if output dir " << outputDir << " is a dir or not\n";
        isValid = false;
        return;
    }

    if (!isDir) {
        outputDir = outputDir.parentPath();
        outputDir /= makeArchiveName();
    }
    
    VirtualFileSystem::argv = argv[0];
    filesystem = std::unique_ptr<VirtualFileSystem>(new VirtualFileSystem());
    filesystem->initialize(nullptr, true, true);
    
    LOG_V("Starting system asset packer. Base dir: {}", filesystem->getBaseDirectory());
    
    Path path(filesystem->getBaseDirectory());
    path /= "../../..";
    path /= "system";
    
    LOG_V("Expected asset dir: {}", path.getNativeString());
    
    if (!fs::exists(path.getNativeString())) {
        throw std::runtime_error(WrongDirError);
    }
    
    filesystem->setWritePath(path);
    filesystem->addReadPath(path, "", true);
}

SystemAssetPacker::~SystemAssetPacker() {}

void SystemAssetPacker::recursiveExport(const Path& path, const editor::ConverterManager& cm, PlatformIdentifier platformID) {
    const auto contents = filesystem->getDirectoryContents(path.getGenericString());
    for (const auto& item : contents) {
        Path sourcePath = path / item;
        
        FileStat stat;
        const auto statResult = filesystem->getStats(sourcePath.getGenericString(), stat);
        if (statResult != FileSystemResult::Success) {
            LOG_E("The virtual file system reported an error when trying to stat \"{}\". Error: {}", item, statResult);
            
            throw std::runtime_error("Failed to convert system assets because of a filesystem error. (Check log)");
        }
        
        if (stat.type == FileType::Directory) {
            LOG_V("Found a system asset subdirectory: {}", sourcePath);
            recursiveExport(sourcePath, cm, platformID);
        } else {
            assert(!sourcePath.empty());
            if (sourcePath.extension() == con::ImportSettingsExtension()) {
                continue;
            }
            
            // Make sure we skip hidden files
            if (sourcePath.filename().getNativeString()[0] == '.') {
                continue;
            }
            
            const AssetType type = cm.getAssetType(sourcePath);
            
            // System localization strings require a different path
            Path destinationPath = (type == AssetType::Strings) ? cm.makeFinalPathForSystemStrings(sourcePath, platformID)
                                                                    : cm.makeFinalPathForAsset(sourcePath, type, platformID);
            
            LOG_V("IMPORTING FILE: {}"
                  "\n\t\tHash: {}"
                  "\n\t\tType: {}"
                  "\n\t\tDestination: {}", sourcePath, HS(sourcePath.getGenericString()), con::AssetTypeToTranslationString(type), destinationPath);
            
            std::vector<editor::ImportedAssetData> iad;
            std::unique_ptr<editor::ConverterState> converterState = cm.initializeConverter(sourcePath, platformID);
            
            if (converterState == nullptr) {
                LOG_E("Failed to initialize the ConversionSettings for a system asset: {}", sourcePath);
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
            
            converterState->setSystemAsset(true);
            if (!cm.convert(*converterState)) {
                LOG_E("Failed to convert a system asset: {}", sourcePath);
                throw std::runtime_error("Failed to convert a system asset (Check log)");
            }
            
            for (const auto& a : converterState->getImportedAssets()) {
                if (a.getMetadata().getBase().getFileHash() == 0) {
                    throw std::runtime_error("Failed to examine the metadata of an imported asset");
                }
            }
        }
    }
}

Path SystemAssetPacker::makeArchiveName() const {
    return ("system" + con::PackFileExtension());
}

int SystemAssetPacker::pack() {
    if (!isValid) {
        return 1;
    }
    
    const Path platformDataBasePath = Path("platforms");
    const editor::ConverterManager cm(filesystem.get(), platformDataBasePath);
    
    // TODO different platforms should use different packages. Linux and Windows share the same assets (e.g. BC compressed textures).
    /// However, once Android is supported, we will need to add support for ETC2 compressed textures.
    /// for (platformID : platforms) ...
    const PlatformIdentifier processedPlatform = PlatformIdentifier::Linux_Desktop_x86_64;
    
    const Path platformDataPath = cm.getAssetDestinationPath(processedPlatform);
    const Path realPlatformDataPath = filesystem->getRealDirectory(platformDataPath.getGenericString());
    
    const Path systemArchiveName = makeArchiveName();
    const Path archivePath = realPlatformDataPath / systemArchiveName;
    
    auto& fs = DefaultFileSystem::Instance();
    if (processedPlatform == con::GetCurrentPlatform()) {
        const Path assetsFolder = realPlatformDataPath / con::BaseAssetPath();
        if (fs.exists(assetsFolder)) {
            LOG_D("Removing the assets folder that was built by a previous run: {}", assetsFolder);
            fs.removeRecursive(assetsFolder);
        }
        
        if (fs.exists(archivePath)) {
            LOG_D("Removing the asset pack that was built by a previous run: {}", archivePath);
            fs.removeRecursive(archivePath);
        }
    } else {
        if (!realPlatformDataPath.empty()) {
            LOG_D("Removing asset data processed by a previous run from {}", realPlatformDataPath);
            fs.removeRecursive(realPlatformDataPath);
        }
    }
    
    const Path pathToCreate = filesystem->getCurrentWriteDirectory() / platformDataBasePath;
    
    LOG_D("Creating asset data directories for current platform: {}", pathToCreate);
    fs.createDirectory(pathToCreate);
    
    if (!Project::CreateImportedAssetDirectories(pathToCreate, processedPlatform)) {
        throw std::runtime_error("Failed to create imported asset directories");
    }
    
    recursiveExport("raw/system", cm, processedPlatform);
    
    std::vector<util::PathToCompress> pathsToCompress;
    pathsToCompress.reserve(100);
    
    const Path pathWithPlatform = (processedPlatform == con::GetCurrentPlatform()) ? pathToCreate : (pathToCreate / Path(con::PlatformIdentifierToName(processedPlatform)));
    for (const auto& d : std::filesystem::recursive_directory_iterator(pathWithPlatform.getNativeString())) {
        if (!fs::is_directory(d)) {
            const Path relativePath = d.path().lexically_relative(pathWithPlatform.getNativeString());
            pathsToCompress.emplace_back(d, relativePath);
            
            //LOG_D("VX:\n\t" << d << "\n\t" << d.path() << "\n\t" << relativePath << "\n\t" << pathWithPlatform);
        }
    }
    
    pathsToCompress.emplace_back(filesystem->getCurrentWriteDirectory() / con::EngineBaseConfigFile(), con::EngineBaseConfigFile());
//     pathsToCompress.emplace_back(filesystem->getCurrentWriteDirectory() / con::ProjectBaseConfigFile, con::ProjectBaseConfigFile);
    
    if (util::CompressFileListToZip(pathsToCompress, archivePath, util::CompressionLevel::Best)) {
        LOG_V("Successfully compressed system files for the {} platform.", con::PlatformIdentifierToName(processedPlatform));
    } else {
        LOG_E("Failed to compress system files for the {} platform.", con::PlatformIdentifierToName(processedPlatform));
        throw std::runtime_error("Failed to compress system files (check log)");
    }
    
    // Copy the files for the current platform next to the executable
    if (processedPlatform == con::GetCurrentPlatform()) {
        LOG_V("Copying the files for current platfom from {} to {}", archivePath, outputDir);

        const auto copyResult = fs.copyFile(archivePath, outputDir / systemArchiveName, FileCopyOption::OverwriteExisting);
        if (copyResult != FileSystemResult::Success) {
            LOG_E("Failed to copy the files to the {}", outputDir);
            return 1;
        }
        
//         // Create a project file for the editor, otherwise, it won't start.
//         if (!Project::CreateProjectFile(filesystem->getBaseDirectory(), "IYFEditor", "The IYFEngine Team", "en_US", con::EditorVersion)) {
//             LOG_E("Failed to create the project file in {}", filesystem->getBaseDirectory());
//             throw std::runtime_error("Failed to create the project file ");
//         }
    }
    
    return 0;
}

}
