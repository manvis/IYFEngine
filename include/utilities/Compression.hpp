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

#ifndef IYF_COMPRESSION_HPP
#define IYF_COMPRESSION_HPP

#include <vector>
#include "io/Path.hpp"

namespace iyf::util {
enum class CompressionLevel {
    /// Just pack to zip
    None,
    /// High speed, low compression
    Fast,
    /// Medium speed, medium compression
    Normal,
    /// Lowest speed, best compression
    Best
};

struct PathToCompress {
    PathToCompress(Path filePath, Path archivePath) : filePath(std::move(filePath)), archivePath(std::move(archivePath)) {}
    
    /// Real file system path to the file you want to compress
    Path filePath;
    /// A path to the file in the zip archive
    Path archivePath;
};

// Compress the specified directory to zip. If the zip file already exists, it will be overwritten.
bool CompressDirectoryToZip(const Path& dir, const Path& zipPath, CompressionLevel level);

// Compress the specified files to zip. If the zip file already exists, it will be overwritten.
bool CompressFileListToZip(const std::vector<PathToCompress>& paths, const Path& zipPath, CompressionLevel level);
}

#endif // IYF_COMPRESSION_HPP
