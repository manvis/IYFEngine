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

#include "assets/typeManagers/FontTypeManager.hpp"
#include "core/Engine.hpp"
#include "core/filesystem/File.hpp"
#include "core/Logger.hpp"

namespace iyf {
FontTypeManager::FontTypeManager(AssetManager* manager) : TypeManager(manager) {
    engine = manager->getEngine();
    api = engine->getGraphicsAPI();
}

void FontTypeManager::performLoad(hash32_t, const fs::path& path, const Metadata&, Font& assetData) {
//     const FontMetadata& fontMetadata = std::get<FontMetadata>(meta);
    
    File file(path, File::OpenMode::Read);
    auto wholeFile = file.readWholeFile();
    
    assetData.data = wholeFile.first.release();
    assetData.size = wholeFile.second;
}

void FontTypeManager::performFree(Font& assetData) {
    delete[] assetData.data;
}

void FontTypeManager::initMissingAssetHandle() {
    // Yeah, this will crash everything if used, however, I don't think I can add a "missing" shader. Unlike meshes or textures
    // that are easy to replace, shaders may be from different stages, have different inputs/outputs/etc.
    missingAssetHandle = AssetHandle<Font>();
}
}

