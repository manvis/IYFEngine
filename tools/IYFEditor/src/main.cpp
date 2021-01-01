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

#include <iostream>
#include <memory>
#include <cstdint>
#include <sstream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "MeshImporterState.h"
#include "assetImport/converters/MeshConverter.hpp"
#include "utilities/hashing/crc64.hpp"

#include "utilities/hashing/Hashing.hpp"
#include "states/EditorState.hpp"

const char MAGIC_NUMBER[4] = {'I', 'Y', 'F', 'M'};

void recursivePass(const aiScene* scene, const aiNode* node, int depth) {
    for (int i = 0; i < depth; ++i) {
        std::cout << " ";
    }
    
    std::cout << depth << "\t" << node->mName.C_Str() << " " 
            << node->mNumMeshes << "\n";
    
    for (std::size_t i = 0; i < node->mNumMeshes; ++i) {
        bool hasBones = scene->mMeshes[node->mMeshes[i]]->HasBones();
        
        
        std::cout << "\t\t" << node->mMeshes[i] << " " << 
                scene->mMeshes[node->mMeshes[i]]->mName.C_Str() <<
                " c:" << scene->mMeshes[node->mMeshes[i]]->GetNumColorChannels() <<
                " t:" << scene->mMeshes[node->mMeshes[i]]->GetNumUVChannels() <<
                " b:" << hasBones << " am:" << 
                scene->mMeshes[node->mMeshes[i]]->mNumAnimMeshes << "\n";
        
//        if (hasBones) {
//            for (size_t j = 0; j < scene->mMeshes[node->mMeshes[i]]->mNumBones; ++i) {
//                std::cout << "";
//            }
//        }
    }
    
    for (std::size_t i = 0; i < node->mNumChildren; ++i) {
        recursivePass(scene, node->mChildren[i], depth + 1);
    }
}
int main(int argc, char* argv[]) {
    assert(argc >= 1);
    
    std::unique_ptr<iyf::Engine> gameEngine(new iyf::Engine(argc, argv, iyf::EngineMode::Editor));
    gameEngine->pushState(std::make_unique<iyf::editor::EditorState>(gameEngine.get()));
    gameEngine->executeMainLoop();
    
    return gameEngine->getReturnValue();
}
