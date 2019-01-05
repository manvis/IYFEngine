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

#ifndef IYF_ASSET_CREATOR_HPP
#define IYF_ASSET_CREATOR_HPP

#include <string>

#include "assets/AssetConstants.hpp"
#include "graphics/materials/MaterialConstants.hpp"

namespace iyf {
class Engine;
}

namespace iyf::editor {
class AssetCreatorWindow {
public:
    static constexpr int MaxNameLength = 128;
    
    /// Create a new directory
    AssetCreatorWindow(Engine* engine, std::string basePath);
    
    /// Create a new Asset
    /// \warning Only some AssetType are supported
    AssetCreatorWindow(Engine* engine, std::string basePath, AssetType type);
    
    bool show();
    void openPopup();
private:
    void executeOperation();
    
    Engine* engine;
    std::string basePath;
    std::string windowName;
    std::string extension;
    char name[MaxNameLength];
    bool directory;
    bool showZeroLengthInfo;
    AssetType type;
    
    /// Used when creating a new material template definition
    MaterialFamily family;
};

}

#endif // IYF_ASSET_CREATOR_HPP
