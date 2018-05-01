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

#ifndef CONVERTERMANAGER_HPP
#define CONVERTERMANAGER_HPP

#include "Converter.hpp"
#include <unordered_map>
#include <memory>

namespace iyf {
namespace editor {
class MeshConverter;
class TextureConverter;

class ConverterManager {
public:
    /// \param fileSystem Observer pointer to the currently active FileSystem instance.
    /// \param assetDestination Destination of converted assets. This path is relative to fileSystem->getCurrentWriteDirectory() and must be inside it
    ConverterManager(const FileSystem* fileSystem, fs::path assetDestination);
    ~ConverterManager();
    
    inline const PlatformInfo& getPlatformInfo(PlatformIdentifier platformID) const  {
        return con::PlatformIdentifierToInfo(platformID);
    }
    
    inline const fs::path getAssetDestinationPath(PlatformIdentifier platformID) const {
        return assetDestination / con::PlatformIdentifierToName(platformID);
    }
    
    inline const FileSystem* getFileSystem() const {
        return fileSystem;
    }
    
    /// Turns the sourcePath and the AssetType into a final virtual filesystem path where the converted asset will be written to.
    /// The path should be used to create a File object or a VirtualFilesystemSerializer.
    ///
    /// \remark The real path will be: /VirtualFileSystemRoot/getAssetDestinationPath()/con::AssetTypeToPath[type]/HS(sourcePath)
    ///
    /// \param sourcePath a relative path to an Asset awaiting conversion. Must be inside getAssetSourcePath().
    /// \param type the type of the asset that's being imported. Can be determined by calling getAssetType() with the same sourcePath
    fs::path makeFinalPathForAsset(const fs::path& sourcePath, AssetType type, PlatformIdentifier platformID) const;
    
    /// System localization strings need a slightly different path.
    fs::path makeFinalPathForSystemStrings(const fs::path& sourcePath, PlatformIdentifier platformID) const;
    
    /// Turns the sourcePath into an import settings path. Since source files and importer settings files exist side by side, this function
    /// simply replaces the current extension with an appropriate one.
    fs::path makeImporterSettingsFilePath(const fs::path& sourcePath) const;
    
    /// \return a real path that corresponds to the provided virtual file system path or an empty path if nothing was found.
    fs::path getRealPath(const fs::path& path) const;
    
    /// Determines the AssetType by checking the extension of the file. If the format is not in the map of supported formats,
    /// AssetType::Custom will be returned and calling convert() will simply copy the said file to the destination.
    ///
    /// \warning This function DOES NOT access the contents of the file. This leads to some limitations. E.g., since animations are
    /// imported from mesh files, this function will return AssetType::Mesh, even if the file contains nothing but an animation library.
    AssetType getAssetType(const fs::path& sourcePath) const;
    
    /// Creates a ConverterState and loads saved import/conversion settings, if they are present.
    ///
    /// \remark This function accesses the file system and should not be called in the main thread.
    /// 
    /// \param[in] sourcePath a path to the file that you want to convert. Must be relative to the root of the virtual FileSystem
    ///
    /// \return A ConverterState or a nullptr if the file wasn't found or an error occured when reading it
    std::unique_ptr<ConverterState> initializeConverter(const fs::path& sourcePath, PlatformIdentifier platformID) const;
    
    /// \remark This function reads the file and should not be called in the main thread.
    ///
    /// \param[in,out] state ConverterState that will be used to perform conversion. Call initializeConverter() to create this object.
    bool convert(ConverterState& state) const;
    
    /// Deserializes previously saved settings (if they exist) to the provided ConverterState.
    ///
    /// \return If any previous settings existed or not.
    bool deserializeSettings(ConverterState& state) const;
protected:
    fs::path makeLocaleStringPath(const fs::path& sourcePath, const fs::path& directory, PlatformIdentifier platformID) const;
    
    std::unordered_map<AssetType, std::unique_ptr<Converter>> typeToConverter;
    
    const FileSystem* fileSystem;
    fs::path assetDestination;
};
}
}

#endif /* CONVERTERMANAGER_HPP */

