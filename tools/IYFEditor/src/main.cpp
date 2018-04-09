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

//#include "core/Engine.hpp"
#include "MeshImporterState.h"
#include "assetImport/MeshConverter.hpp"
#include "utilities/hashing/crc64.hpp"

#include "utilities/hashing/Hashing.hpp"
#include "states/EditorWelcomeState.hpp"

const char MAGIC_NUMBER[4] = {'I', 'Y', 'F', 'M'};

void recursivePass(const aiScene* scene, const aiNode* node, int depth) {
    for (std::size_t i = 0; i < depth; ++i) {
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
    
    std::unique_ptr<iyf::Engine> gameEngine(new iyf::Engine(argv[0], true));
//    gameEngine->changeState(new MeshImporterState(gameEngine.get()));
    gameEngine->pushState(std::make_unique<iyf::editor::EditorWelcomeState>(gameEngine.get()));
    gameEngine->executeMainLoop();
//    MeshConverter meshConverter;
    //meshLoader.load("inPath");
//    meshConverter.convert("texturedTest.fbx", "texturedTest.out", 1);
    
//    std::stringstream ss;
//    for (auto s : File::getDirectoryContents(".")) {
//        ss << s << "\n";
//    }
//    LOG_D(ss.str())
    
//    LOG_D(iyf::crc32::crc32("trial") << " " << iyf::crc64::crc64("trial") << " " << HS("trial"))
    
//    LOG_D(HS("pussy.jpeg"))
//    LOG_D(crc32("pussy.jpeg"))
//    LOG_D(HS(std::string("pussy.jpeg").c_str()))
    
//    Assimp::Importer importer;
//    const aiScene *scene =
//        importer.ReadFile("testSimple.fbx", aiProcessPreset_TargetRealtime_Quality);
//        importer.ReadFile("testHarder.fbx", aiProcessPreset_TargetRealtime_Quality);
//        importer.ReadFile("testDual.fbx", aiProcessPreset_TargetRealtime_Quality);
//        importer.ReadFile("testBoned.fbx", aiProcessPreset_TargetRealtime_Quality);
//        importer.ReadFile("testBonedDual.fbx", aiProcessPreset_TargetRealtime_Quality);
//        importer.ReadFile("testCpl.fbx", aiProcessPreset_TargetRealtime_Quality);
//    importer.ReadFile("testMax.fbx", aiProcessPreset_TargetRealtime_Quality);
//       importer.ReadFile("meshAnimTest.fbx", aiProcessPreset_TargetRealtime_Quality);
//           importer.ReadFile("meshAnim.dae", aiProcessPreset_TargetRealtime_Quality);

//    if(!scene){
//        throw std::runtime_error(importer.GetErrorString());
//    }
//    
//    std::size_t numNodesWithMeshes = 0;
//    for (std::size_t i = 0; i < scene->mRootNode->mNumChildren; ++i) {
////        unsigned int numMeshes = scene->mRootNode->mChildren[i]->mNumMeshes
////        if ()
//    }
//    
//    recursivePass(scene, scene->mRootNode, 0);
//    
//    std::cout << "Num anim:" << scene->mNumAnimations << "\n";
//    for (std::size_t i = 0; i < scene->mNumAnimations; ++i) {
//        std::cout << i << " " << scene->mAnimations[i]->mName.C_Str() << "\n";
//    }
    
    return 0;
}
