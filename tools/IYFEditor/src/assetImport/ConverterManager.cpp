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

#include "assetImport/ConverterManager.hpp"
#include "logging/Logger.hpp"

#include "assetImport/converters/MaterialInstanceConverter.hpp"
#include "assetImport/converters/MaterialTemplateConverter.hpp"
#include "assetImport/converters/MeshConverter.hpp"
#include "assetImport/converters/TextureConverter.hpp"
#include "assetImport/converters/FontConverter.hpp"
#include "assetImport/converters/ShaderConverter.hpp"
#include "assetImport/converters/LocalizationStringConverter.hpp"

#include <algorithm>
#include <variant>

#include "core/Constants.hpp"
#include "io/FileSystem.hpp"
#include "io/File.hpp"
#include "core/filesystem/VirtualFileSystem.hpp"
#include "assets/AssetManager.hpp"

#include "utilities/Regexes.hpp"

#include "fmt/ostream.h"

#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"

namespace iyf {
namespace editor {
ConverterManager::ConverterManager(const VirtualFileSystem* fileSystem, Path assetDestination) 
    : fileSystem(fileSystem), assetDestination(assetDestination) {
    typeToConverter[AssetType::Mesh] = std::make_unique<MeshConverter>(this);
    typeToConverter[AssetType::Texture] = std::make_unique<TextureConverter>(this);
    typeToConverter[AssetType::Font] = std::make_unique<FontConverter>(this);
    typeToConverter[AssetType::Shader] = std::make_unique<ShaderConverter>(this);
    typeToConverter[AssetType::Strings] = std::make_unique<LocalizationStringConverter>(this);
    typeToConverter[AssetType::MaterialTemplate] = std::make_unique<MaterialTemplateConverter>(this);
    typeToConverter[AssetType::MaterialInstance] = std::make_unique<MaterialInstanceConverter>(this);
}

ConverterManager::~ConverterManager() {}

std::unique_ptr<ConverterState> ConverterManager::initializeConverter(const Path& sourcePath, PlatformIdentifier platformID) const {
    if (!fileSystem->exists(sourcePath)) {
        LOG_W("Cannot initialize the converter because file \"{}\" does not exist.", sourcePath)
        return nullptr;
    }
    
    AssetType type = getAssetType(sourcePath);
    
    auto result = typeToConverter.find(type);
    assert(result != typeToConverter.end());
    
    auto converterState = result->second->initializeConverter(sourcePath, platformID);
    if (converterState == nullptr) {
        return nullptr;
    }
    
    assert(type == converterState->getType());
    
    deserializeSettings(*converterState);
    return converterState;
}

Path ConverterManager::makeLocaleStringPath(const Path& sourcePath, const Path& directory, PlatformIdentifier platformID) const {
    if (!std::regex_match(sourcePath.filename().getGenericString(), SystemRegexes().LocalizationFileNameValidationRegex)) {
        LOG_E("String files need to match a specific pattern filename.[LOCALE].csv, where [LOCALE] is en_US, lt_LT, etc.");
        throw std::runtime_error("Invalid source path format (check log)");
    }
    
    std::string locale = sourcePath.stem().extension().getGenericString().substr(1);
    const StringHash nameHash = AssetManager::ComputeNameHash(sourcePath);
    return getAssetDestinationPath(platformID) / directory / (locale + "." + std::to_string(nameHash));
}

Path ConverterManager::makeFinalPathForAsset(const Path& sourcePath, AssetType type, PlatformIdentifier platformID) const {
    if (type == AssetType::Strings) {
        return makeLocaleStringPath(sourcePath, con::AssetTypeToPath(type), platformID);
    } else {
        const StringHash nameHash = AssetManager::ComputeNameHash(sourcePath);
        return getAssetDestinationPath(platformID) / con::AssetTypeToPath(type) / std::to_string(nameHash);
    }
}

Path ConverterManager::makeFinalPathForSystemStrings(const Path& sourcePath, PlatformIdentifier platformID) const {
    return makeLocaleStringPath(sourcePath, con::SystemStringPath(), platformID);
}

Path ConverterManager::getRealPath(const Path& path) const {
    return fileSystem->getRealDirectory(path);
}

const Path ConverterManager::getAssetDestinationPath(PlatformIdentifier platformID) const {
    if (platformID == con::GetCurrentPlatform()) {
        return assetDestination;
    }

    return assetDestination / con::PlatformIdentifierToName(platformID);
}

AssetType ConverterManager::getAssetType(const Path& sourcePath) const {
    return AssetManager::GetAssetTypeFromExtension(sourcePath);
}

Path ConverterManager::makeImporterSettingsFilePath(const Path& sourcePath) const {
    assert(sourcePath.extension() != con::ImportSettingsExtension());
    
    Path pathJSON = sourcePath;
    pathJSON += con::ImportSettingsExtension();
    return pathJSON;
}

bool ConverterManager::deserializeSettings(ConverterState& state) const {
    const Path settingsPath = makeImporterSettingsFilePath(state.getSourceFilePath());
    
    if (fileSystem->exists(settingsPath)) {
        auto jsonFile = VirtualFileSystem::Instance().openFile(settingsPath, FileOpenMode::Read);
        const auto wholeFile = jsonFile->readWholeFile();
        
        rj::Document document;
        document.Parse(wholeFile.first.get(), wholeFile.second);
        state.deserializeJSON(document);
        
        return true;
    } else {
        return false;
    }
}

bool ConverterManager::serializeSettings(ConverterState& state) const {
    rj::StringBuffer buffer;
    rj::PrettyWriter<rj::StringBuffer> writer(buffer);
    writer.SetIndent('\t', 1);
    
    writer.StartObject();
    state.serializeJSON(writer);
    writer.EndObject();
    
    const char* finalJSON = buffer.GetString();
    const std::size_t bufferSize = buffer.GetLength();
    
    const Path pathJSON = makeImporterSettingsFilePath(state.getSourceFilePath());
    auto file = VirtualFileSystem::Instance().openFile(pathJSON, FileOpenMode::Write);
    file->writeBytes(finalJSON, bufferSize);
    file->close();
    
    return true;
}

bool ConverterManager::convert(ConverterState& state) const {
    std::vector<ImportedAssetData>& importedAssets = state.getImportedAssets();
    
    assert(state.isConversionComplete() == (importedAssets.size() != 0));
    if (importedAssets.size() != 0) {
        importedAssets.clear();
        state.setConversionComplete(false);
    }
    
    bool conversionSucceeded = state.getInternalState()->getConverter()->convert(state);
    if (!conversionSucceeded) {
        return false;
    } else {
        state.setConversionComplete(true);
    }
    
    serializeSettings(state);
    
    assert(conversionSucceeded == (importedAssets.size() != 0));
    assert(state.isConversionComplete());
    
    for (const auto& asset : importedAssets) {
        assert(!asset.getDestinationPath().empty());
        
        rj::StringBuffer sb;
        PrettyStringWriter pw(sb);
        
        pw.SetIndent('\t', 1);
        asset.getMetadata().getBase().serializeJSON(pw);
        
        const char* jsonString = sb.GetString();
        std::size_t jsonByteCount = sb.GetSize();
        
        assert(jsonString != nullptr && jsonByteCount != 0);
        
        Path metadataPath = asset.getDestinationPath();
        metadataPath += Path(con::TextMetadataExtension());
        
        auto metadataOutput = VirtualFileSystem::Instance().openFile(metadataPath, FileOpenMode::Write);
        metadataOutput->writeBytes(jsonString, jsonByteCount);
    }
    
    return true;
}

}
}
