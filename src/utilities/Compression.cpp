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

#include "utilities/Compression.hpp"
#include "miniz/miniz.h"
#include "core/Logger.hpp"

#include "fmt/ostream.h"

namespace iyf::util {
inline mz_uint OurCompressionLevelToMinizCompressionLevel(CompressionLevel level) {
    switch (level) {
        case CompressionLevel::None:
            return MZ_NO_COMPRESSION;
        case CompressionLevel::Fast:
            return MZ_BEST_SPEED;
        case CompressionLevel::Normal:
            return MZ_DEFAULT_LEVEL;
        case CompressionLevel::Best:
            return MZ_BEST_COMPRESSION;
    }
    
    throw std::runtime_error("An invalid CompressionLevel was provided");
}

bool CompressDirectoryToZip(const fs::path& dir, const fs::path& zipPath, CompressionLevel level) {
    mz_zip_archive archive;
    mz_zip_zero_struct(&archive);
    
    const mz_uint compressionLevel = OurCompressionLevelToMinizCompressionLevel(level);
    
    if (!mz_zip_writer_init_file_v2(&archive, zipPath.c_str(), 0, compressionLevel)) {
        LOG_E("Failed to create a zip file located in {}. Miniz error: {}", zipPath, mz_zip_get_error_string(archive.m_last_error));
        return false;
    }
    
    bool status = true;
    for (const auto& d : fs::recursive_directory_iterator(dir)) {
        if (!fs::is_directory(d)) {
            if (d == zipPath) {
                LOG_V("The current zip file ({}) was found in the file list. Skipping it.", zipPath);
                continue;
            }
            
            const fs::path relativePath = d.path().lexically_relative(dir);
            status = mz_zip_writer_add_file(&archive, relativePath.c_str(), d.path().c_str(), nullptr, 0, compressionLevel);
            
            if (!status) {
                LOG_E("Failed to write {} to zip {}. Miniz error: {}", d, zipPath, mz_zip_get_error_string(archive.m_last_error));
                break;
            }
        }
    }
    
    if (!mz_zip_writer_finalize_archive(&archive)) {
        LOG_E("Failed to finalize a {} written zip file {}. It is likely it won't be readable. Miniz error: {}", (status ? " completely " : "n incompletely "), zipPath, mz_zip_get_error_string(archive.m_last_error));
        
        status = false;
    }

    if (!mz_zip_writer_end(&archive)) {
        LOG_E("Failed to end the writer for a zip file located in {}. Miniz error: {}", zipPath, mz_zip_get_error_string(archive.m_last_error));
        
        status = false;
    }
    
    return status;
}

bool CompressFileListToZip(const std::vector<PathToCompress>& paths, const fs::path& zipPath, CompressionLevel level) {
    mz_zip_archive archive;
    mz_zip_zero_struct(&archive);
    
    const mz_uint compressionLevel = OurCompressionLevelToMinizCompressionLevel(level);
    
    if (!mz_zip_writer_init_file_v2(&archive, zipPath.c_str(), 0, compressionLevel)) {
        LOG_E("Failed to create a zip file located in {}. Miniz error: {}", zipPath, mz_zip_get_error_string(archive.m_last_error));
        return false;
    }
    
    bool status = true;
    for (const auto& p : paths) {
        auto stat = fs::status(p.filePath);
        if (!fs::exists(stat)) {
            LOG_E("Failed to compress {} to {}. This file does not exist.", p.filePath, zipPath);
            status = false;
            break;
        }
        
        if (!fs::is_directory(p.filePath)) {
            if (p.filePath == zipPath) {
                LOG_V("The current zip file ({}) was found in the file list. Skipping it.", zipPath);
                continue;
            }
            
            status = mz_zip_writer_add_file(&archive, p.archivePath.c_str(), p.filePath.c_str(), nullptr, 0, compressionLevel);
            
            if (!status) {
                LOG_E("Failed to write {} to zip {}. Miniz error: {}", p.filePath, zipPath, mz_zip_get_error_string(archive.m_last_error));
                break;
            }
        } else {
            LOG_E("Failed to compress {}. It is a directory and CompressFileListToZip only supports files.", p.filePath);
            
            status = false;
            break;
        }
    }
    
    if (!mz_zip_writer_finalize_archive(&archive)) {
        LOG_E("Failed to finalize a {} written zip file {}. It is likely it won't be readable. Miniz error: {}", (status ? " completely " : "n incompletely "), zipPath, mz_zip_get_error_string(archive.m_last_error));
        
        status = false;
    }

    if (!mz_zip_writer_end(&archive)) {
        LOG_E("Failed to end the writer for a zip file located in {}. Miniz error: {}", zipPath, mz_zip_get_error_string(archive.m_last_error));
        
        status = false;
    }
    
    return status;
}

}
