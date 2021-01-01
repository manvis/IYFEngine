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

#ifndef IYF_CONVERTER_STATE_HPP
#define IYF_CONVERTER_STATE_HPP

#include "core/Constants.hpp"
#include "core/Platform.hpp"
#include "io/interfaces/TextSerializable.hpp"
#include "utilities/NonCopyable.hpp"
#include "assetImport/ImportedAssetData.hpp"

namespace iyf::editor {
class Converter;

class InternalConverterState {
public:
    InternalConverterState(const Converter* converter) : converter(converter) {
        if (converter == nullptr) {
            throw std::logic_error("Converter cannot be nullptr");
        }
    }
    
    virtual ~InternalConverterState() = 0;
    
    const Converter* getConverter() const {
        return converter;
    }
private:
    const Converter* converter;
};

inline InternalConverterState::~InternalConverterState() {}

class ConverterState : private NonCopyable, public TextSerializable {
public:
    ConverterState(PlatformIdentifier platformID, std::unique_ptr<InternalConverterState> internalState, const Path& sourcePath, FileHash sourceFileHash)
        : sourcePath(sourcePath), sourceFileHash(sourceFileHash), conversionComplete(false), debugOutputRequested(false), systemAsset(false),
          platformID(platformID), internalState(std::move(internalState)) {}
    
    /// If true, this represents a system asset
    inline bool isSystemAsset() const {
        return systemAsset;
    }
    
    /// \warning This should only be used internally (e.g., in SystemAssetPacker). Do not expose this in the editor
    inline void setSystemAsset(bool systemAsset) {
        this->systemAsset = systemAsset;
    }
    
    inline const std::vector<std::string>& getTags() const {
        return tags;
    }
    
    inline std::vector<std::string>& getTags() {
        return tags;
    }
    
    /// If true, some converters may export additional debug data. This parameter is not serialized
    inline void setDebugOutputRequested(bool requested) {
        debugOutputRequested = requested;
    }
    
    /// If true, some converters may export additional debug data. This parameter is not serialized
    inline bool isDebugOutputRequested() const {
        return debugOutputRequested;
    }
    
    /// \warning This value should only be modified by the ConverterManager, tests or in VERY special cases.
    inline void setConversionComplete(bool state) {
        conversionComplete = state;
    }
    
    inline bool isConversionComplete() const {
        return conversionComplete;
    }
    
    inline const InternalConverterState* getInternalState() const {
        return internalState.get();
    }
    
    inline InternalConverterState* getInternalState() {
        return internalState.get();
    }
    
    inline const Path& getSourceFilePath() const {
        return sourcePath;
    }
    
    virtual AssetType getType() const = 0;
    
    inline const std::vector<ImportedAssetData>& getImportedAssets() const {
        return importedAssets;
    }
    
    /// \warning The contents of this vector should only be modified by the ConverterManager, the Converters or in VERY special cases.
    inline std::vector<ImportedAssetData>& getImportedAssets() {
        return importedAssets;
    }
    
    inline FileHash getSourceFileHash() const {
        return sourceFileHash;
    }
    
    inline PlatformIdentifier getPlatformIdentifier() const {
        return platformID;
    }
    
    /// Derived classes should not create a root object in serializeJSON()
    virtual bool makesJSONRoot() const final override {
        return false;
    }
    
    /// Serializes the conversion settings stored in this ConverterState instance.
    virtual void serializeJSON(PrettyStringWriter& pw) const final override;
    
    /// Deserialized the conversion settings from the provided JSON object and into this one
    virtual void deserializeJSON(JSONObject& jo) final override;
    
    /// Obtain the preferred version for the serialized data. Derived classes should increment this whenever their
    /// serialization format changes. If an older format is provided, reasonable defaults should be set for data
    /// that is not present in it.
    virtual std::uint64_t getLatestSerializedDataVersion() const = 0;
protected:
    virtual void serializeJSONImpl(PrettyStringWriter& pw, std::uint64_t version) const = 0;
    virtual void deserializeJSONImpl(JSONObject& jo, std::uint64_t version) = 0;
private:
    std::vector<ImportedAssetData> importedAssets;
    std::vector<std::string> tags;
    Path sourcePath;
    FileHash sourceFileHash;
    
    bool conversionComplete;
    bool debugOutputRequested;
    bool systemAsset;
    PlatformIdentifier platformID;
    
    /// The internal state of the importer. Calling Converter::initializeConverter() typically loads the file 
    /// into memory (to build initial metadata, importer settings, etc.). The contents of the said file can be
    /// stored in this variable and reused in Converter::convert() to avoid duplicate work.
    ///
    /// Moreover, by using an opaque pointer to a virtual base class, we allow the converters to safely store the 
    /// state of external helper libraries while keeping their includes confined to the converters' cpp files.
    ///
    /// \warning This may depend on the context, Engine version, OS version, etc. and MUST NEVER BE SERIALIZED
    std::unique_ptr<InternalConverterState> internalState;
};

}

#endif // IYF_CONVERTER_STATE_HPP
