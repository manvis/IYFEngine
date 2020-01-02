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

#ifndef IYF_SYSTEM_ASSET_PACKER_HPP
#define IYF_SYSTEM_ASSET_PACKER_HPP

#include <memory>

#include "core/Constants.hpp"
#include "core/Platform.hpp"
#include "core/filesystem/cppFilesystem.hpp"

namespace iyf {
class FileSystem;

namespace editor {
class ConverterManager;
}

class SystemAssetPacker {
public:
    SystemAssetPacker(int argc, char* argv[]);
    ~SystemAssetPacker();
    
    int pack();
    void recursiveExport(const fs::path& path, const editor::ConverterManager& cm, PlatformIdentifier platformID);
private:
    fs::path makeArchiveName() const;
    
    std::unique_ptr<FileSystem> filesystem;
    fs::path outputDir;
    bool isValid;
};

}
#endif // IYF_SYSTEM_ASSET_PACKER_HPP
