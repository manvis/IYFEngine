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

#ifndef IYF_METADATA_BASE_HPP
#define IYF_METADATA_BASE_HPP

#include "assets/AssetType.hpp"
#include "io/interfaces/Serializable.hpp"
#include "io/interfaces/TextSerializable.hpp"
#include "io/Path.hpp"
#include "utilities/hashing/Hashing.hpp"

#include <cstdint>
#include <vector>

namespace iyf {

enum class MetadataSource {
    Constructor,
    JSON,
    BinaryData,
};

class MetadataBase : public Serializable, public TextSerializable {
public:
    inline MetadataBase(AssetType assetType) : assetType(assetType), metadataSource(MetadataSource::Constructor), complete(false), systemAsset(false), fileHash(0) {}
    
    inline MetadataBase(AssetType assetType, FileHash fileHash, const Path& sourceAsset, FileHash sourceFileHash, bool systemAsset, const std::vector<std::string>& tags, bool complete) 
        : assetType(assetType), metadataSource(MetadataSource::Constructor), complete(complete), systemAsset(systemAsset), tags(tags), fileHash(fileHash), sourceAsset(sourceAsset), sourceFileHash(sourceFileHash) {}
    
    virtual ~MetadataBase() { }
    
    AssetType getAssetType() const {
        return assetType;
    }
    
    /// \return true if the instance was initialized using a constructor that initializes all memebers
    /// false otherwise.
    bool isComplete() const {
        return complete;
    }
    
    bool isSystemAsset() const {
        return systemAsset;
    }
    
    FileHash getFileHash() const {
        return fileHash;
    }
    
    const Path& getSourceAssetPath() const {
        return sourceAsset;
    }
    
    /// Displays the values of this Metadata object using ImGui widgets.
    ///
    /// \warning Do not create a window when overriding this - only use regular widgets.
    virtual void displayInImGui() const = 0;
    
    /// Obtain the preferred version for the serialized data. Derived classes should increment this whenever their
    /// serialization format changes. If an older format is provided, reasonable defaults should be set for data
    /// that is not present in it.
    virtual std::uint16_t getLatestSerializedDataVersion() const = 0;
    
    /// Serializes a class derived from MetadataBase into a portable and optimized binary representation.
    ///
    /// This function writes some parameters common to all metadata files and then calls serializeImpl()
    ///
    /// \throws std::logic_error if isComplete() is false. This is done to prevent accidentally serializing default initialized
    /// Metadata that would mess up the loaders.
    virtual void serialize(Serializer& fw) const final override;
    
    /// Derializes a class derived from MetadataBase from a portable and optimized binary representation
    ///
    /// This function reads some parameters common to all metadata files and then calls deserializeImpl()
    virtual void deserialize(Serializer& fr) final override;
    
    /// Writes metadata to a human readable JSON (for debug).
    ///
    /// This function writes some parameters common to all metadata files and then calls serializeJSONImpl()
    ///
    /// \throws std::logic_error if isComplete() is false. This is done to prevent accidentally serializing default initialized
    /// Metadata that would mess up the loaders.
    virtual void serializeJSON(PrettyStringWriter& pw) const final override;
    
    /// Reads debug metadata from a human readable JSON file.
    ///
    /// This function reads some parameters common to all metadata files and then calls deserializeJSONImpl()
    virtual void deserializeJSON(JSONObject& jo) final override;
    
    virtual bool makesJSONRoot() const final override {
        return true;
    }
    
    /// Gets the source of the metadata object
    inline MetadataSource getMetadataSource() const {
        return metadataSource;
    }
protected:
    inline bool equals(const MetadataBase& other) const {
        // WARNING: Do not include metadataSource in comparison.
        return (assetType == other.assetType) &&
               (complete == other.complete) &&
               (systemAsset == other.systemAsset) &&
               (tags == other.tags) &&
               (fileHash == other.fileHash) &&
               (sourceAsset == other.sourceAsset) &&
               (sourceFileHash == other.sourceFileHash);
    }
private:
    friend class AssetManager;
    
    virtual void serializeImpl(Serializer& fw, std::uint16_t version) const = 0;
    virtual void deserializeImpl(Serializer& fr, std::uint16_t version) = 0;
    virtual void serializeJSONImpl(PrettyStringWriter& pw, std::uint16_t version) const = 0;
    virtual void deserializeJSONImpl(JSONObject& jo, std::uint16_t version) = 0;
    
    AssetType assetType;
    MetadataSource metadataSource;
    bool complete;
    bool systemAsset;
    std::vector<std::string> tags;
    FileHash fileHash;
    Path sourceAsset;
    FileHash sourceFileHash;
};
}

#endif // IYF_METADATA_BASE_HPP

