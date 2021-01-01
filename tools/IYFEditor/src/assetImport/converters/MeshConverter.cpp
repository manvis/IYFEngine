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

#include <sstream>
#include <bitset>
#include <unordered_map>
#include <SDL2/SDL.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "glm/gtc/packing.hpp"
#include "glm/vec4.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "glm/gtx/string_cast.hpp"

#include "assets/metadata/AnimationMetadata.hpp"
#include "assets/metadata/MeshMetadata.hpp"

#include "assetImport/converters/MeshConverter.hpp"
#include "assetImport/converterStates/MeshConverterState.hpp"
#include "graphics/VertexDataLayouts.hpp"
#include "graphics/AnimationDataStructures.hpp"
#include "graphics/culling/BoundingVolumes.hpp"
#include "graphics/MeshFormats.hpp"

#include "logging/Logger.hpp"
#include "core/Constants.hpp"
#include "core/filesystem/VirtualFileSystem.hpp"
#include "io/File.hpp"
#include "io/FileSystem.hpp"
#include "io/serialization/FileSerializer.hpp"
#include "io/serialization/MemorySerializer.hpp"

#include "utilities/hashing/Hashing.hpp"

#include "assetImport/ConverterManager.hpp"

#include "fmt/ostream.h"

namespace iyf {
namespace editor {

class MeshConverterInternalState : public InternalConverterState {
public:
    MeshConverterInternalState(const Converter* converter) : InternalConverterState(converter) {}
    
    std::unique_ptr<Assimp::Importer> importer;
};

struct MeshVertexBoneDataImport {
    std::uint8_t boneIDs[4] = {0, 0, 0, 0};
    float boneWeights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    
    std::uint8_t currentID = 0;
};
    
// TODO more possible root names from other modelling software. "Armature" is how Blender names it by default
std::array<std::string, 1> POSSIBLE_ROOT_NAMES = {"Armature"};

MeshConverter::MeshConverter(const ConverterManager* manager) : Converter(manager) {
    //
}

std::unique_ptr<ConverterState> MeshConverter::initializeConverter(const Path& inPath, PlatformIdentifier platformID) const {
    std::unique_ptr<MeshConverterInternalState> internalState = std::make_unique<MeshConverterInternalState>(this);
    internalState->importer = std::make_unique<Assimp::Importer>();
    
    const FileHash sourceFileHash = manager->getFileSystem()->computeFileHash(inPath);
    return std::unique_ptr<MeshConverterState>(new MeshConverterState(platformID, std::move(internalState), inPath, sourceFileHash));
}

bool MeshConverter::convert(ConverterState& state) const {
    return convertV1(state);
    // If (once) we have different mesh file versions
//    switch (versionNumber) {
//        case 1:
//            return convertV1(inFile, importedAssets);
//        default:
//            LOG_E("Format version " << versionNumber << " is not supported")
//            return false;
//    }
}

void MeshConverter::writeHeader(Serializer& fw, std::uint16_t versionNumber) const {
    fw.writeBytes(mf::MagicNumber, sizeof(char) * 4);
    fw.writeUInt16(versionNumber);
}

void MeshConverter::writeAnimationHeader(Serializer& fw, std::uint16_t versionNumber) const {
    fw.writeBytes(af::MagicNumber, sizeof(char) * 4);
    fw.writeUInt16(versionNumber);
}

static void printNodes(const aiNode* node, std::size_t depth, std::stringstream& ss) {
    ss << depth << " ";
    
    for (std::size_t d = 0; d < depth; ++d) {
        ss << " ";
    }

    ss << node->mName.C_Str() << " meshes: " << node->mNumMeshes << " meta: " << node->mMetaData << "\n\t";
    depth++;

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        printNodes(node->mChildren[i], depth, ss);
    }
}

static void printMeshBones(const aiNode* node, const aiMesh* mesh) {
    std::stringstream ss;
    ss << "Mesh: " << mesh->mName.C_Str() << "; Bones:";
    for (std::size_t i = 0; i < mesh->mNumBones; ++i) {
        
        const aiNode* boneNode = node->FindNode(mesh->mBones[i]->mName);
        bool found = boneNode != nullptr;
        ss << "\n\t\t" << mesh->mBones[i]->mName.C_Str() << "; Found: " << found << "; Node name " << boneNode->mName.C_Str();
    }
    
    LOG_D("{}", ss.str())
}

static glm::mat4 aiMatToGLMMat(aiMatrix4x4 aiMat) {
    glm::mat4 glmMat(aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
                     aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
                     aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
                     aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4);
    return glmMat;
}

static void makeV1Skeleton(const aiNode* node, std::vector<Bone>& bones, std::unordered_map<StringHash, std::uint8_t>& nameHashToID, std::uint8_t parentID = 0) {
    std::uint8_t id = bones.size();
    assert(bones.size() <= mf::v1::MaxBones);

    glm::mat4 transform = aiMatToGLMMat(node->mTransformation);

    std::string name = node->mName.C_Str();
    StringHash nameHash = HS(name.c_str());

    if (nameHashToID.find(nameHash) != nameHashToID.end()) {
        // TODO localize
        throw std::runtime_error("Unhandled bone configuration.");
    } else {
        nameHashToID[nameHash] = id;
    }

    bones.push_back({name, transform, nameHash, parentID});

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        makeV1Skeleton(node->mChildren[i], bones, nameHashToID, id);
    }
}

static const aiNode* determineSkeletonRoot(const aiNode* sceneRoot) {
    for (const auto& n : POSSIBLE_ROOT_NAMES) {
        const aiNode* skeletonRoot = sceneRoot->FindNode(n.c_str());
        
        if (skeletonRoot != nullptr) {
            return skeletonRoot;
        }
    }
    
    return nullptr;
}

// TODO FIXME this function does not clean up after a failed import. It returns false, so broken files will not end up in the asset database, however, they will remain on the hard drive.
bool MeshConverter::convertV1(ConverterState& state) const {
    MeshConverterInternalState* internalState = dynamic_cast<MeshConverterInternalState*>(state.getInternalState());
    assert(internalState != nullptr);
    
    std::vector<ImportedAssetData>& importedAssets = state.getImportedAssets();
    
    Assimp::Importer* importer = internalState->importer.get();
    
    const Path inFile = manager->getFileSystem()->getRealDirectory(state.getSourceFilePath());
    
    // aiProcessPreset_TargetRealtime_Quality does all the required optimizations, it even limits bone weights to 4
    const aiScene *scene = importer->ReadFile(inFile.getCString(), aiProcessPreset_TargetRealtime_Quality);
    int property = importer->GetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS);
    if (property != 4 && property != static_cast<int>(0xffffffff)) {
        LOG_I("Assimp mesh import bone limit property must be 4 or unset (0xffffffff), was {}. It was reset back to 4", property)
        importer->SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 4);
    }
    
    static_assert(aiProcessPreset_TargetRealtime_Quality & aiProcess_LimitBoneWeights, "Flag does not limit bone weights.");
    static_assert((AI_LMW_MAX_WEIGHTS  == 4), "Assimp does not limit bone weights to 4.");

    if (!scene) {
        LOG_E("Assimp error: {}", importer->GetErrorString())
        return false;
    }

    // Iterate through nodes, find first one that has a mesh
    // Store names to find root bone?
    
    unsigned int firstMeshNode = 0;
    bool hasMeshes = false;
    bool hasVertexColors = false;
    unsigned int totalMeshNodes = 0;
    const aiNode* root = scene->mRootNode;
    
    for (unsigned int i = 0; i < root->mNumChildren; ++i) {
        if (root->mChildren[i]->mNumMeshes >= 1) {
            if (!hasMeshes) {
                firstMeshNode = i;
                hasMeshes = true;
            }
            
            ++totalMeshNodes;
        }
    }
    
//    std::stringstream ss;
//    printNodes(root, 0, ss);
//    LOG_D(ss.str());
    
    if (totalMeshNodes > 1) {
        LOG_W("File {} contains more than one top level mesh "
                "node. Importing a whole scene is not supported. Only the "
                "first node will be processed. If these meshes should be a part of a "
                "single object, use a mesh joining command in your modeling "
                "software.", inFile)
    }
    
    unsigned int numSubMeshes = root->mChildren[firstMeshNode]->mNumMeshes;
//    LOG_D("First ID of node with meshes is " << firstMeshNode << ". It has " << numSubMeshes << " sub-meshes.")
    if (numSubMeshes > mf::v1::MaxSubmeshes) {
        LOG_W("File {} contains more than {} sub-mesh nodes, only the first {} will be processed.", inFile, mf::v1::MaxSubmeshes, mf::v1::MaxSubmeshes)
                
        numSubMeshes = mf::v1::MaxSubmeshes;
    }
    
    std::uint32_t totalVertices = 0;
    std::uint32_t totalIndices = 0;
    
    std::bitset<mf::v1::MaxSubmeshes> hasBones;
    for (unsigned int i = 0; i < numSubMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[root->mChildren[firstMeshNode]->mMeshes[i]];
        
        totalVertices += mesh->mNumVertices;
        totalIndices += mesh->mNumFaces * 3;
        
        if (!mesh->HasFaces()) {
            // According to Assimp's docs this should NEVER happen unless someone messes with the importer's flags
            LOG_E("File {} has a mesh without faces. Did someone mess up Assimp's import flags?", inFile)
            throw std::runtime_error("Can't process a mesh without faces");
        }
        
        if (mesh->mPrimitiveTypes & aiPrimitiveType_POINT ||
            mesh->mPrimitiveTypes & aiPrimitiveType_LINE ||
            mesh->mPrimitiveTypes & aiPrimitiveType_POLYGON) {
            LOG_E("File {} has a mesh with non-triangle elements", inFile)
        }
        
        if (mesh->mNumVertices > mf::v1::MaxVertices) {
            LOG_E("File {} has a mesh with more than {} vertices", inFile, mf::v1::MaxVertices)
            return false;
        }
        
        if (!mesh->HasPositions()) {
            outputError(inFile, "vertex positions", mesh);
            return false;
        }
        
        if (!mesh->HasNormals()) {
            outputError(inFile, "vertex normals", mesh);
            return false;
        }
        
        if (!mesh->HasTangentsAndBitangents()) {
            outputError(inFile, "vertex tangents and bitangents", mesh);
            return false;
        }
        
        if (mesh->GetNumUVChannels() == 0) {
            outputError(inFile, "UV coordinates", mesh);
            return false;
        } else if (mesh->GetNumUVChannels() > mf::v1::MaxTextureChannels) {
            LOG_W("File {} contains more than {} UV channels, only the first {} will be processed.", inFile, mf::v1::MaxTextureChannels, mf::v1::MaxTextureChannels)
        }
        
        // Default texture is not necessary
//        unsigned int mi = mesh->mMaterialIndex;
//        aiMaterial* m = scene->mMaterials[mi];
//        if (m->GetTextureCount(aiTextureType_DIFFUSE) == 0) {
//            outputError(inFile, "a diffuse texture", mesh);
//            return false;
//        }
        
       if (mesh->GetNumColorChannels() > 0) {
            hasVertexColors = true;
           
            if (mesh->GetNumColorChannels() > mf::v1::MaxColorChannels) {
                LOG_W("File {} contains more than {} color channel(s), only the  first {} will be processed.", inFile, mf::v1::MaxColorChannels, mf::v1::MaxColorChannels)
            }
       }
        
        if (mesh->HasBones()) {
            hasBones.set(i);
            
//            printMeshBones(root, mesh);
        }
    }
    
    if (totalVertices > con::MaxVerticesPerMesh) {
        LOG_W("A mesh can't have more than {} vertices. It has {}", con::MaxVerticesPerMesh, totalVertices);
        return false;
    }
    
    std::vector<Bone> bones;
    std::unordered_map<StringHash, std::uint8_t> nameHashToID;
    // TODO. IMPORTANT!! INITIAL TRANSFORMATION. E.g., even if we don't have bones, mesh instances DO get transformed by assimp and we need to take care of that.
    
    std::size_t numAnimations = 0;
    
    if (hasBones.any() && hasBones.count() != numSubMeshes) {
        LOG_W("All sub-meshes must either have bones or not. Cases when only some have bones are not supported. In this file {} sub-meshes out of{} have bones", hasBones.count(), numSubMeshes);
        return false;
    } else if (hasBones.any()) {
        bones.reserve(mf::v1::MaxBones);
        nameHashToID.reserve(mf::v1::MaxBones);
        
        const aiNode* armatureRoot = determineSkeletonRoot(root);
        
        if (armatureRoot == nullptr) {
            std::stringstream ss;
            
            for (const auto& s : POSSIBLE_ROOT_NAMES) {
                ss << "\n\t\t" << s;
            }
            LOG_W("Failed to find the root node of the skeleton. It MUST have one of these names: {}", ss.str())
            return false;
        }

        makeV1Skeleton(armatureRoot, bones, nameHashToID);
        
        if (bones.size() > mf::v1::MaxBones) {
            LOG_W("Too many bones in mesh {}. Max allowed is {}, was: {}", inFile, mf::v1::MaxBones, bones.size());
            return false;
        }
        
//        LOG_D("Number of bones in skeleton: " << bones.size())
        
        // TODO TEST THIS
        const aiNode* temp = armatureRoot;
        std::vector<aiMatrix4x4> transformations;
        std::size_t depth = 0;
        
        while (temp->mParent != nullptr) {
            transformations.push_back(temp->mParent->mTransformation);
            temp = temp->mParent;
            depth++;
        }
        
//        LOG_D("Skeleton root node is in depth " << depth)

        // We need to iterate backwards, assemble the transformations for the parent nodes of the armature and store them in the root bone
        aiMatrix4x4 transform;
        for (auto it = transformations.rbegin(); it != transformations.rend(); ++it) {
            transform = *it * transform;
        }
        
        assert(bones.size() > 0);
        bones[0].transform = aiMatToGLMMat(transform) * bones[0].transform;
    }

    if (bones.size() > 0 && scene->mNumAnimations == 0) {
        LOG_E("Can't import mesh file {} because the mesh has bones but no animations.", inFile)
        return false;
    } else if (bones.size() > 0 && scene->mNumAnimations > mf::v1::MaxAnimations) {
        LOG_W("Mesh file {} has {} however, engine only supports up to {} animations for each mesh. Only the first {} will be imported.", inFile, scene->mNumAnimations, mf::v1::MaxAnimations, scene->mNumAnimations);
        numAnimations = mf::v1::MaxAnimations;
    } else if (bones.size() > 0 && scene->mNumAnimations > 0) {
        numAnimations = scene->mNumAnimations;
    }
    
    // WARNING
    // SECTIONS that write something are marked with "WRITE: *something*" comments.
    // If changing something here, make sure that mesh importers match what's being
    // done here. Moreover, INCREASE VERSION NUMBER.
    
    const std::uint16_t versionNumber = 1;
    // WRITE: First - magic number, second - version number, then - number of sub-meshes, total number of vertices, total number of indices
    // and, finally, the number of bones
    //iyf::MemorySerializer fw(1024 * 512);
    const Path meshOutputPath = manager->makeFinalPathForAsset(state.getSourceFilePath(), AssetType::Mesh, state.getPlatformIdentifier());
    iyf::MemorySerializer fw(1024 * 512);
    writeHeader(fw, versionNumber);
    fw.writeUInt8(numSubMeshes);
    // This is 4 bytes despite con::MaxVerticesPerMesh limit
    fw.writeUInt32(totalVertices);
    fw.writeUInt32(totalIndices);
    fw.writeUInt8(bones.size());
    fw.writeUInt8(hasVertexColors ? 1 : 0);
    
    std::vector<AABB> aabbs;
    aabbs.reserve(numSubMeshes);

    for (unsigned int i = 0; i < numSubMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[root->mChildren[firstMeshNode]->mMeshes[i]];
        
        // TODO move to another format. v1 supports only a single UV channel and no color channels, so these are useless
        const unsigned int uvsInMesh = mesh->GetNumUVChannels();
        const std::uint8_t uvChannels = (uvsInMesh > mf::v1::MaxTextureChannels) ? mf::v1::MaxTextureChannels : uvsInMesh;
        
//        const unsigned int colInMesh = mesh->GetNumColorChannels();
//        const std::uint8_t colorChannels = (colInMesh > mf::v1::MaxColorChannels) ? mf::v1::MaxColorChannels : colInMesh;
        
        // WRITE: the number of vertices in mesh
        fw.writeUInt16(mesh->mNumVertices);
        
        // TODO move to another format. v1 supports only a single UV channel and no color channels, so these are useless
        // WRITE: the number of UV channels and the number of vertex color channels
//        fw.writeUInt8(uvChannels);
//        fw.writeUInt8(colorChannels);
        
        // Process bone data (if any)
        std::vector<MeshVertexBoneDataImport> boneData;
        if (mesh->HasBones()) {
            // First, we need to obtain per vertex data from per bone data
            boneData.resize(mesh->mNumVertices);
            
            for (unsigned int b = 0; b < mesh->mNumBones; ++b) {
                const aiBone* bone = mesh->mBones[b];
                
                auto idIt = nameHashToID.find(HS(bone->mName.C_Str()));
                assert(idIt != nameHashToID.end());
                
                for (unsigned int w = 0; w < bone->mNumWeights; ++w) {
                    const aiVertexWeight weight = bone->mWeights[w];
                    
                    MeshVertexBoneDataImport& d = boneData[weight.mVertexId];
                    
                    assert(d.currentID < mf::v1::MaxBonesPerVertex);
                    assert(weight.mWeight > 0.0f);
                    
                    d.boneIDs[d.currentID] = idIt->second;
                    d.boneWeights[d.currentID] = weight.mWeight;
                    d.currentID = d.currentID + 1;
                }
            }
        }
        
        // TODO how does this affect bones? Is the order correct?
        glm::mat4 rootTransformation = aiMatToGLMMat(root->mChildren[firstMeshNode]->mTransformation) * aiMatToGLMMat(root->mTransformation);

//         glm::vec3 scale;
//         glm::quat orientation;
//         glm::vec3 translation;
//         glm::vec3 skew;
//         glm::vec4 perspective;
//         glm::decompose(rootTransformation, scale, orientation, translation, skew, perspective);
//         LOG_D(scale.x << " " << scale.y << " " << scale.z)
//         
//         glm::decompose(aiMatToGLMMat(root->mChildren[firstMeshNode]->mTransformation), scale, orientation, translation, skew, perspective);
//         LOG_D(scale.x << " " << scale.y << " " << scale.z)
//         
//         glm::decompose(aiMatToGLMMat(root->mTransformation), scale, orientation, translation, skew, perspective);
//         LOG_D(scale.x << " " << scale.y << " " << scale.z)
        
        // For AABB and bounding sphere construction
        glm::vec3 minPos = rootTransformation * glm::vec4(mesh->mVertices[0].x, mesh->mVertices[0].y, mesh->mVertices[0].z, 1.0f);
        glm::vec3 maxPos = rootTransformation * glm::vec4(mesh->mVertices[0].x, mesh->mVertices[0].y, mesh->mVertices[0].z, 1.0f);
        
        for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
            const aiVector3D* p = mesh->mVertices;
            glm::vec3 pos = rootTransformation * glm::vec4(p[j].x, p[j].y, p[j].z, 1.0f);
            
            // WRITE: vertex position values
            fw.writeFloat(pos.x);
            fw.writeFloat(pos.y);
            fw.writeFloat(pos.z);
            
            // Building the AABB by finding the largest and smallest values in all axes
            minPos.x = std::min(pos.x, minPos.x);
            minPos.y = std::min(pos.y, minPos.y);
            minPos.z = std::min(pos.z, minPos.z);
            
            maxPos.x = std::max(pos.x, maxPos.x);
            maxPos.y = std::max(pos.y, maxPos.y);
            maxPos.z = std::max(pos.z, maxPos.z);
            
            // Normalize normals, tangents and bitangents. Haven't run into this, but Assimp docs say they
            // may NOT be normalized if they're read directly from the file.
            aiVector3D* t = mesh->mTangents;
            t[j].Normalize();
            aiVector3D* b = mesh->mBitangents;
            b[j].Normalize();
            aiVector3D* n = mesh->mNormals;
            n[j].Normalize();
            
            // WRITE: Tangents, normals and bitangents are packed into unsigned 32 bit integers and then written
            // Such precision is often sufficient, however, if any defects appear in renders, changes may need to be made here.
            // Moreover, this can be shrunken even more by only saving normals, tangents and a bias. Bias (1 or -1) goes into the last component 
            // of one of the two remaining vectors and bitangent is recovered in shader. However, for the time being, current solution is more 
            // than sufficient and I see no need to prematurely optimize.
            fw.writeUInt32(glm::packSnorm3x10_1x2(glm::vec4(n[j].x, n[j].y, n[j].z, 0.0f)));
            fw.writeUInt32(glm::packSnorm3x10_1x2(glm::vec4(t[j].x, t[j].y, t[j].z, 0.0f)));
            fw.writeUInt32(glm::packSnorm3x10_1x2(glm::vec4(b[j].x, b[j].y, b[j].z, 0.0f)));
            
            // TODO move to another format. v1 supports only a single UV channel and no color channels, so these are useless
//            for (unsigned int k = 0; k < colorChannels; ++k) {
//                const aiColor4D* c = mesh->mColors[k];
//                // WRITE: vertex colors are packed into a single 32 bit integer and then written
//                // Honestly, I've never used vertex colors all that much, so I'm not sure if this precision is sufficient
//                // for most use cases.
//                fw.writeUInt32(glm::packUnorm4x8(glm::vec4(c[i].r, c[i].g, c[i].b, c[i].a)));
//            }
            
            for (unsigned int k = 0; k < uvChannels; ++k) {
                const aiVector3D* uv = mesh->mTextureCoords[k];
                // WRITE: UV coordinates as two floats
                // TODO Will we need the Z coordinate for UVs?
                fw.writeFloat(uv[j].x);
                fw.writeFloat(uv[j].y);
            }
            
            // WRITE: vertex bone data (if any)
            if (mesh->HasBones()) {
                fw.writeUInt8(boneData[j].boneIDs[0]);
                fw.writeUInt8(boneData[j].boneIDs[1]);
                fw.writeUInt8(boneData[j].boneIDs[2]);
                fw.writeUInt8(boneData[j].boneIDs[3]);
                
                fw.writeUInt32(glm::packUnorm4x8(glm::vec4(boneData[j].boneWeights[0], boneData[j].boneWeights[1], boneData[j].boneWeights[2], boneData[j].boneWeights[3])));
            }
            
            if (hasVertexColors && mesh->GetNumColorChannels() > 0) {
                aiColor4D* c = mesh->mColors[0];
                
                fw.writeUInt32(glm::packUnorm4x8(glm::vec4(c->r, c->g, c->b, c->a)));
            } else if (hasVertexColors && mesh->GetNumColorChannels() == 0) {
                fw.writeUInt32(glm::packUnorm4x8(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f)));
            }
        }
        
        // WRITE: number of indices
        fw.writeUInt32(mesh->mNumFaces * 3);
        for (unsigned int j = 0; j < mesh->mNumFaces; ++j) {
            unsigned int* f = mesh->mFaces[j].mIndices;
            // WRITE: mesh indices
            fw.writeUInt16(f[0]);
            fw.writeUInt16(f[1]);
            fw.writeUInt16(f[2]);
        }
        
//         glm::vec3 boundingSphereCenter = glm::vec3(minPos.x + (maxPos.x - minPos.x) * 0.5f,
//                                                    minPos.y + (maxPos.y - minPos.y) * 0.5f,
//                                                    minPos.z + (maxPos.z - minPos.z) * 0.5f);
//         float boundingSphereRadius = std::max(std::max((maxPos.x - minPos.x) * 0.5f, (maxPos.y - minPos.y) * 0.5f), (maxPos.z - minPos.z) * 0.5f);
        
        // NO LONGER WRITE: per submesh AABB and bounding sphere data
//         fw.writeFloat(minPos.x);
//         fw.writeFloat(minPos.y);
//         fw.writeFloat(minPos.z);
//         
//         fw.writeFloat(maxPos.x);
//         fw.writeFloat(maxPos.y);
//         fw.writeFloat(maxPos.z);
//         
//         fw.writeFloat(boundingSphereCenter.x);
//         fw.writeFloat(boundingSphereCenter.y);
//         fw.writeFloat(boundingSphereCenter.z);
//         
//         fw.writeFloat(boundingSphereRadius);
        
        aabbs.emplace_back(minPos, maxPos);

        // TODO write a separate mapping file for human readable bone name strings
        
        unsigned int mi = mesh->mMaterialIndex;
        aiMaterial* m = scene->mMaterials[mi];
        
        // Find the name of a single diffuse texture (if there is one). It will be used for default material creation.
        if (m->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString aiPath;
            m->GetTexture(aiTextureType_DIFFUSE, 0, &aiPath);
            
            Path path = aiPath.C_Str();
            
            // Base path that is used for error output
            Path textureFileName = path.filename();
            
            // Where engine is expecting to find the texture. This will be hashed and stored to file
            // TODO full or not full? Not full (like now) doesn't break hashes when defaults are changed. However, maybe there are advantages to keeping it full (like commented out)?
//            Path finalTexturePath = Path(con::TexturePath);
//            finalTexturePath += textureFileName;
            Path finalTexturePath = textureFileName;
            
//            LOG_D(finalTexturePath)
            
            // Just the extension to check if this texture format is supported
            Path extension = textureFileName.extension().getGenericString();

//            std::string textureFileName(path.C_Str());
//            
//            auto dirSeparatorWindowsLocation = textureFileName.find_last_of('\\');
//            auto dirSeparatorUnixLocation = textureFileName.find_last_of('/');
//            
//            if (dirSeparatorWindowsLocation != std::string::npos && dirSeparatorUnixLocation != std::string::npos) {
//                LOG_E("File " << inFile << "; Found both / and \\ in the texture path, can't determine format. ABORTING");
//                return false;
//            }
//            
//            if (dirSeparatorWindowsLocation != std::string::npos) {
//                LOG_W("File " << inFile << "; Found a \\ in the texture path, truncated to just the name and extension.")
//                textureFileName.erase(0, dirSeparatorWindowsLocation + 1);
//            } else if (dirSeparatorUnixLocation != std::string::npos) {
//                LOG_W("File " << inFile << "; Found a / in the texture path, truncated to just the name and extension.")
//                textureFileName.erase(0, dirSeparatorUnixLocation + 1);
//            }
//            
//            size_t formatSeparator = textureFileName.find_last_of('.');
//            const std::string ext = textureFileName.substr(formatSeparator + 1);
            
            // TODO maybe add a library that would automatically convert all textures that are imported to appropriate formats
            if (extension != ".dds" && extension != ".DDS" && extension != ".ktx" && extension != ".KTX") {
                LOG_W("File {}; Texture name {} doesn't end with .dds, .DDS, .ktx or .KTX; "
                      "This engine can only load compressed DDS or KTX texture formats.", inFile, textureFileName)
            }
//            
//            std::string finalDiffusePath(con::TexturePath);
//            finalDiffusePath.append(textureFileName);
//            
//            std::string finalNormalPath(textureFileName);
//            finalNormalPath.insert(formatSeparator, "_normal");
//            finalNormalPath.insert(0, con::TexturePath);
            
            StringHash diffuseHash(HS(finalTexturePath.getGenericString()));
//            LOG_D("Texture name: " << textureFileName << "; hash: " << diffuseHash.value())
            
            // 0 is reserved in this case
            if (diffuseHash == 0) {
                diffuseHash = StringHash(1);
            }
            
            // NO LONGER WRITE: hashed path to the default diffuse texture which serves as its identifier
//             fw.writeUInt64(diffuseHash.value());
            // Storing metadata instead
            
        } else {
            // No texture to use for default material creation
            // NO LONGER WRITE: 0
//             fw.writeUInt64(0);
            // Storing metadata instead
            
        }
    }
    
    // Build an AABB and a bounding sphere for this mesh from submesh AABB data
    int min = static_cast<int>(AABB::Vertex::Minimum);
    int max = static_cast<int>(AABB::Vertex::Maximum);
    
    glm::vec3 minPos = glm::vec3(aabbs[0].vertices[min].x, aabbs[0].vertices[min].y, aabbs[0].vertices[min].z);
    glm::vec3 maxPos = glm::vec3(aabbs[0].vertices[max].x, aabbs[0].vertices[max].y, aabbs[0].vertices[max].z);
    
    for (const auto& aabb : aabbs) {
        minPos.x = std::min(aabb.vertices[min].x, minPos.x);
        minPos.y = std::min(aabb.vertices[min].y, minPos.y);
        minPos.z = std::min(aabb.vertices[min].z, minPos.z);

        maxPos.x = std::max(aabb.vertices[max].x, maxPos.x);
        maxPos.y = std::max(aabb.vertices[max].y, maxPos.y);
        maxPos.z = std::max(aabb.vertices[max].z, maxPos.z);
    }
    
    glm::vec3 boundingSphereCenter = glm::vec3(minPos.x + (maxPos.x - minPos.x) * 0.5f,
                                               minPos.y + (maxPos.y - minPos.y) * 0.5f,
                                               minPos.z + (maxPos.z - minPos.z) * 0.5f);
    float boundingSphereRadius = std::max(std::max((maxPos.x - minPos.x) * 0.5f, (maxPos.y - minPos.y) * 0.5f), (maxPos.z - minPos.z) * 0.5f);

    // WRITE: per mesh AABB and bounding sphere data
    fw.writeFloat(minPos.x);
    fw.writeFloat(minPos.y);
    fw.writeFloat(minPos.z);

    fw.writeFloat(maxPos.x);
    fw.writeFloat(maxPos.y);
    fw.writeFloat(maxPos.z);

    fw.writeFloat(boundingSphereCenter.x);
    fw.writeFloat(boundingSphereCenter.y);
    fw.writeFloat(boundingSphereCenter.z);

    fw.writeFloat(boundingSphereRadius);
    
    for (const auto& b : bones) {
        // WRITE: parent id (or same id, if no parent) and transformation
        fw.writeUInt8(b.parent);

        fw.writeFloat(b.transform[0][0]);
        fw.writeFloat(b.transform[0][1]);
        fw.writeFloat(b.transform[0][2]);
        fw.writeFloat(b.transform[0][3]);
        fw.writeFloat(b.transform[1][0]);
        fw.writeFloat(b.transform[1][1]);
        fw.writeFloat(b.transform[1][2]);
        fw.writeFloat(b.transform[1][3]);
        fw.writeFloat(b.transform[2][0]);
        fw.writeFloat(b.transform[2][1]);
        fw.writeFloat(b.transform[2][2]);
        fw.writeFloat(b.transform[2][3]);
        fw.writeFloat(b.transform[3][0]);
        fw.writeFloat(b.transform[3][1]);
        fw.writeFloat(b.transform[3][2]);
        fw.writeFloat(b.transform[3][3]);

//        glm::mat4 r(b.transform[0][0], b.transform[0][1], b.transform[0][2], b.transform[0][3],
//                    b.transform[1][0], b.transform[1][1], b.transform[1][2], b.transform[1][3],
//                    b.transform[2][0], b.transform[2][1], b.transform[2][2], b.transform[2][3],
//                    b.transform[3][0], b.transform[3][1], b.transform[3][2], b.transform[3][3]);
//
//        assert(r == b.transform);
    }
    
    // TODO make animations shareable
    // TODO do NOT return false, but start exporting another animation
    LOG_D("AnyBones {}", hasBones.any())
    if (hasBones.any() && scene->HasAnimations()) {
        // WRITE: number of animation handles that will be written in the mesh file
        fw.writeUInt32(numAnimations);
        
        for (unsigned int i = 0; i < numAnimations; ++i) {
            Animation currentAnim;
            const aiAnimation* animation = scene->mAnimations[i];
            
            std::string animationName(animation->mName.C_Str());
            std::size_t pipeLoc = animationName.find_first_of("|");
            
            // Let's remove the pipe symbol so that we could safely use the animation's name in a file name
            // TODO maybe instead of using POSSIBLE_ROOT_NAMES extract the root node's name form this? Is Assimp or Blender responisble for it existing? Or does it only appear in FBX files?
            if (pipeLoc != std::string::npos) {
                animationName = animationName.substr(pipeLoc + 1);
            }
            
            if (animation->mNumChannels != bones.size()) {
                LOG_W("Can't export an animation called \"{}\". Each skeleton node should have an animation channel. This mesh has {} bone(s) and {} channel(s).", animationName, bones.size(), animation->mNumChannels)
                return false;
            }
            
            currentAnim.duration = animation->mDuration;
            currentAnim.ticksPerSecond = animation->mTicksPerSecond;
            currentAnim.animationChannels.resize(animation->mNumChannels);
            
            for (unsigned int j = 0; j < animation->mNumChannels; ++j) {
                const aiNodeAnim* channel = animation->mChannels[j];
                
//                LOG_D(channel->mPreState << " " << channel->mPostState)
                        
                const auto it = nameHashToID.find(HS(channel->mNodeName.C_Str()));
                if (it == nameHashToID.end()) {
                    LOG_W("Can't export an animation called \"{}\". It has a channel called \"{}\" but no bone with the same name exists in the imported mesh", animationName, channel->mNodeName.C_Str())
                    return false;
                }
                
                Channel& currentChannel = currentAnim.animationChannels[it->second];
                
                if (channel->mNumPositionKeys == 0) {
                    LOG_W("Can't export an animation called \"{}\". Each animation channel must have at least 1 location key and channel called \"{}\" doesn't have any", animationName, channel->mNodeName.C_Str())
                    return false;
                } else if (channel->mNumPositionKeys == 1) {
                    const aiVectorKey& key = channel->mPositionKeys[0];
                    currentChannel.locations.push_back({glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z)});
                } else {
                    currentChannel.locations.reserve(channel->mNumPositionKeys);
                    
                    if (channel->mNumPositionKeys != std::floor(animation->mDuration + 1.0)) {
                        LOG_W("Can't export an animation called \"{}\". Each animation channel must have either only 1 key frame (meaning not animated) or std::floor(animation_length) + 1 keyframes (a.k.a. be baked). Channel \"{}\" has {} position key(s).", animationName, channel->mNodeName.C_Str(), channel->mNumPositionKeys)
                        return false;
                    }
                    
                    // TODO these "even spacing out" checks sometimes give false negatives. Find out why
//                    std::int64_t delta = static_cast<std::int64_t>(channel->mPositionKeys[1].mTime) - static_cast<std::int64_t>(channel->mPositionKeys[0].mTime);
                    
                    for (unsigned int k = 0; k < channel->mNumPositionKeys; ++k) {
                        const aiVectorKey& key = channel->mPositionKeys[k];
                        
//                        if ((k >= 1) && ((static_cast<std::int64_t>(key.mTime) - static_cast<std::int64_t>(channel->mPositionKeys[k - 1].mTime)) != delta)) {
//                            LOG_W("Can't export an animation called \"" << animationName << "\". Each key in every animation channel must be evenly spaced out (baked). Position keys in channel \""
//                                  << channel->mNodeName.C_Str() << "\" are not.")
//                                    
//                            return false;
//                        }
                        
                        currentChannel.locations.push_back({glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z)});
                    }
                }
                
                if (channel->mNumRotationKeys == 0) {
                    LOG_W("Can't export an animation called \"{}\". Each animation channel must have at least 1 rotation key and channel called \"{}\" doesn't have any", animationName, channel->mNodeName.C_Str())
                    return false;
                } else if (channel->mNumRotationKeys == 1) {
                    const aiQuatKey& key = channel->mRotationKeys[0];
                    currentChannel.rotations.push_back({glm::quat(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z)});
                } else {
                    currentChannel.rotations.reserve(channel->mNumRotationKeys);
                    
                    if (channel->mNumRotationKeys != std::floor(animation->mDuration + 1.0)) {
                        LOG_W("Can't export an animation called \"{}\". Each animation channel must have either only 1 key frame (meaning not animated) or std::floor(animation_length) + 1 keyframes (a.k.a. be baked). Channel \"{}\" has {} rotation key(s).", animationName, channel->mNodeName.C_Str(), channel->mNumRotationKeys)
                        return false;
                    }
                    
//                    std::int64_t delta = static_cast<std::int64_t>(channel->mRotationKeys[1].mTime) - static_cast<std::int64_t>(channel->mRotationKeys[0].mTime);
                    
                    for (unsigned int k = 0; k < channel->mNumRotationKeys; ++k) {
                        const aiQuatKey& key = channel->mRotationKeys[k];
                        
//                        LOG_V(key.mTime)
//                        if ((k >= 1) && ((static_cast<std::int64_t>(key.mTime) - static_cast<std::int64_t>(channel->mRotationKeys[k - 1].mTime)) != delta)) {
//                            LOG_W("Can't export an animation called \"" << animationName << "\". Each key in every animation channel must be evenly spaced out (baked). Rotation keys in channel \""
//                                  << channel->mNodeName.C_Str() << "\" are not.")
//                            LOG_D(k << " " << key.mTime << " " << channel->mRotationKeys[k - 1].mTime << " " << delta << " " << static_cast<std::size_t>(key.mTime - channel->mRotationKeys[k - 1].mTime))
//                            return false;
//                        }
                        
                        currentChannel.rotations.push_back({glm::quat(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z)});
                    }
                }
                
                if (channel->mNumScalingKeys == 0) {
                    LOG_W("Can't export an animation called \"{}\". Each animation channel must have at least 1 scale key and channel called \"{}\" doesn't have any", animationName, channel->mNodeName.C_Str())
                    return false;
                } else if (channel->mNumScalingKeys == 1) {
                    const aiVectorKey& key = channel->mScalingKeys[0];
                    currentChannel.scales.push_back({glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z)});
                } else {
                    currentChannel.scales.reserve(channel->mNumScalingKeys);
                    
                    if (channel->mNumScalingKeys != std::floor(animation->mDuration + 1.0)) {
                        LOG_W("Can't export an animation called \"{}\". Each animation channel must have either only 1 key frame (meaning not animated) or std::floor(animation_length) + 1 keyframes (a.k.a. be baked). Channel \"{}\" has {} scaling key(s).", animationName, channel->mNodeName.C_Str(), channel->mNumScalingKeys)
                        return false;
                    }
                    
//                    std::int64_t delta = static_cast<std::int64_t>(channel->mScalingKeys[1].mTime) - static_cast<std::int64_t>(channel->mScalingKeys[0].mTime);
                    
                    for (unsigned int k = 0; k < channel->mNumScalingKeys; ++k) {
                        const aiVectorKey& key = channel->mScalingKeys[k];
                        
//                        if ((k >= 1) && ((static_cast<std::int64_t>(key.mTime) - static_cast<std::int64_t>(channel->mScalingKeys[k - 1].mTime)) != delta)) {
//                            LOG_W("Can't export an animation called \"" << animationName << "\". Each key in every animation channel must be evenly spaced out (baked). Scale keys in channel \""
//                                  << channel->mNodeName.C_Str() << "\" are not.")
//                                    
//                            return false;
//                        }
                        
                        currentChannel.scales.push_back({glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z)});
                    }
                }
            }
            
            Path animIn = state.getSourceFilePath();
            animIn += "_";
            animIn += animationName;
            
            const Path animOut = manager->makeFinalPathForAsset(animIn, AssetType::Animation, state.getPlatformIdentifier());
            
            // WRITE: hashed animation name for finding and loading it later
            fw.writeUInt64(HS(animOut.getGenericString()));
            
            const std::uint16_t animationVersionNumber = 1;
            iyf::MemorySerializer aw(1024*512);

            // WRITE: animation file header
            writeAnimationHeader(aw, animationVersionNumber);
            
            // WRITE: duration, ticks per second and animation channel count
            aw.writeFloat(currentAnim.duration);
            aw.writeFloat(currentAnim.ticksPerSecond);
            aw.writeUInt8(currentAnim.animationChannels.size());
            
            for (std::size_t a = 0; a < currentAnim.animationChannels.size(); ++a) {
                const Channel& channel = currentAnim.animationChannels[a];
                
                // WRITE: number of location keyframes and their values
                aw.writeUInt32(channel.locations.size());
                for (std::size_t l = 0; l < channel.locations.size(); ++l) {
                    const LocationKey& lk = channel.locations[l];
                    aw.writeFloat(lk.location.x);
                    aw.writeFloat(lk.location.y);
                    aw.writeFloat(lk.location.z);
                }
                
                // WRITE: number of rotation keyframes and their values
                aw.writeUInt32(channel.rotations.size());
                for (std::size_t r = 0; r < channel.rotations.size(); ++r) {
                    const RotationKey& rk = channel.rotations[r];
                    aw.writeFloat(rk.rotation.w);
                    aw.writeFloat(rk.rotation.x);
                    aw.writeFloat(rk.rotation.y);
                    aw.writeFloat(rk.rotation.z);
                }
                
                // WRITE: number of scaling keyframes and their values
                aw.writeUInt32(channel.scales.size());
                for (std::size_t s = 0; s < channel.scales.size(); ++s) {
                    const ScaleKey& sk = channel.scales[s];
                    aw.writeFloat(sk.scale.x);
                    aw.writeFloat(sk.scale.y);
                    aw.writeFloat(sk.scale.z);
                }
            }
            
            auto animationFile = VirtualFileSystem::Instance().openFile(animOut, FileOpenMode::Write);
            animationFile->writeBytes(aw.data(), aw.size());
            animationFile->close();
            
            FileHash fileHash = HF(aw.data(), aw.size());
            AnimationMetadata animationMeta(fileHash, state.getSourceFilePath(), state.getSourceFileHash(), state.isSystemAsset(),
                                            state.getTags(), animationVersionNumber, animation->mDuration, animation->mTicksPerSecond);
            ImportedAssetData iadAnim(AssetType::Animation, animationMeta, animOut);
            importedAssets.push_back(iadAnim);
        }
    } else if (hasBones.none() && scene->HasAnimations()) {
        // At the time of writing, assimp does not even seem to be able to load shape keys.
        LOG_D("Mesh animation not yet supported")
        // WRITE: number of animation handles that will be written in the mesh file
        fw.writeUInt32(0);
    } else {
        fw.writeUInt32(0);
    }
    
    auto meshFile = VirtualFileSystem::Instance().openFile(meshOutputPath, FileOpenMode::Write);
    meshFile->writeBytes(fw.data(), fw.size());
    meshFile->close();
    
    FileHash fileHash = HF(fw.data(), fw.size());
    MeshMetadata meshMetadata(fileHash, state.getSourceFilePath(), state.getSourceFileHash(), state.isSystemAsset(), state.getTags(),
                              versionNumber, numSubMeshes, hasBones.any(), false, totalVertices, totalIndices, 0, bones.size(), hasVertexColors ? 1 : 0, 1);
    ImportedAssetData iadMesh(AssetType::Mesh, meshMetadata, meshOutputPath);
    importedAssets.push_back(iadMesh);
        
    importer->FreeScene();
    
    LOG_I("Successfully imported mesh from file {}", inFile)
    return true;
}

void MeshConverter::outputError(const Path& inPath, const std::string& lacking, const aiMesh* mesh) const {
    if (mesh->mName.length == 0) {
        LOG_W("File {}, sub-mesh !UNNAMED! is lacking {}", inPath, lacking)
    } else {
        LOG_W("File {}, sub-mesh {} is lacking {}", inPath, mesh->mName.C_Str(), lacking)
    }
}

MeshConverter::~MeshConverter() {
    //dtor
}

}
}
