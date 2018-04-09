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

#ifndef MESHCONVERTER_H
#define MESHCONVERTER_H

#include <cstdint>
#include <memory>

#include "Converter.hpp"

class aiMesh;

namespace iyf {
class FileWriter;

namespace editor {

/// Wraps mesh import using Assimp library
class MeshConverter : public Converter {
    public:
        MeshConverter(const ConverterManager* manager);
        
        virtual std::unique_ptr<ConverterState> initializeConverter(const fs::path& inPath, PlatformIdentifier platformID) const final override;
        
        /// \brief Reads a mesh file and converts it to a format optimized for this engine.
        ///
        /// This function uses the Assimp library, so the number of possible input formats is really big.
        /// However, as of March 2017, only FBX has been tested extensively. Moreover, Assimp currently
        /// seems to be unable to import morph-target animations.
        /// 
        /// \todo we should store more metadata about animations. We also need to create some sort of a "skeleton fingerprint" to determine if different animations could be shared
        /// 
        /// \param[in] importedAssets [in,out] used to store data and metadata describing the imported assets.
        /// \return status of the operation
        virtual bool convert(ConverterState& state) const final override;

        virtual ~MeshConverter();
    private:
        
        /// V1 format consists of header (see MeshConverter::writeHeader),
        /// 8 bit number of sub-meshes
        /// \param[in] inPath path of the input file
        /// \param[in] outPath path of the output file
        bool convertV1(ConverterState& state) const;
        
        /// Writes the header of a mesh file.
        /// It consists of 4 chars 'I', 'Y', 'F', 'M' and a 16 bit version number.
        /// \param[in] fw output file handle
        /// \param[in] versionNumber unique numerical format id
        void writeHeader(Serializer& fw, std::uint16_t versionNumber) const;
        
        /// Writes the header of an animation file.
        /// It consists of 4 chars 'I', 'Y', 'F', 'A' and a 16 bit version number.
        /// \param[in] fw output file handle
        /// \param[in] versionNumber unique numerical format id
        void writeAnimationHeader(Serializer& fw, std::uint16_t versionNumber) const ;
        
        // Material is only generated when we create an actual instance of the mesh
//        bool generateMaterial();

        void outputError(const fs::path& inPath, const std::string &lacking, const aiMesh* mesh) const;
};

}
}
#endif // MESHCONVERTER_H
