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

#include "MetadataSerializationTests.hpp"

#include "core/serialization/MemorySerializer.hpp"

#include "assets/metadata/AnimationMetadata.hpp"
#include "assets/metadata/MeshMetadata.hpp"
#include "assets/metadata/TextureMetadata.hpp"
#include "assets/metadata/FontMetadata.hpp"
#include "assets/metadata/AudioMetadata.hpp"
#include "assets/metadata/VideoMetadata.hpp"
#include "assets/metadata/ScriptMetadata.hpp"
#include "assets/metadata/ShaderMetadata.hpp"
#include "assets/metadata/StringMetadata.hpp"
#include "assets/metadata/CustomMetadata.hpp"
#include "assets/metadata/MaterialTemplateMetadata.hpp"
#include "assets/metadata/MaterialInstanceMetadata.hpp"

#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"

namespace iyf::test {
const std::vector<std::string> tags = {"TagA", "TagB", "TagC"};

MetadataSerializationTests::MetadataSerializationTests(bool verbose) : iyf::test::TestBase(verbose) {}
MetadataSerializationTests::~MetadataSerializationTests() {}

void MetadataSerializationTests::initialize() {}

#define RUN_META_TEST(Type, ...)\
    {\
        Type meta(FileHash(256), fs::path("asset/test/path.ast"), FileHash(1024), false, __VA_ARGS__);\
        \
        MemorySerializer ms(16384);\
        meta.serialize(ms);\
        ms.seek(0);\
        \
        Type binaryDeserialization;\
        binaryDeserialization.deserialize(ms);\
        \
        if (meta != binaryDeserialization) {\
            return TestResults(false, "Failed to perfrom binary deserialization of " #Type);\
        }\
        \
        const std::string JSONString = meta.getJSONString();\
        rj::Document document;\
        document.Parse(JSONString);\
        \
        Type jsonDeserialization;\
        jsonDeserialization.deserializeJSON(document);\
        \
        if (meta != jsonDeserialization) {\
            return TestResults(false, "Failed to perfrom JSON deserialization of " #Type);\
        }\
    }

TestResults MetadataSerializationTests::run() {
    RUN_META_TEST(AnimationMetadata, tags, 1, 20.0f, 1.5f);
    RUN_META_TEST(MeshMetadata, tags, 1, 8, false, true, 2048, 2048 * 3, 0, 0, 0, 1);
    RUN_META_TEST(TextureMetadata, tags, 1024, 1024, 1, 1, 1, 9, 3, TextureFilteringMethod::Trilinear, TextureTilingMethod::MirroredRepeat, TextureTilingMethod::MirroredRepeat, 0, TextureCompressionFormat::BC1, false, 9600016);
    RUN_META_TEST(FontMetadata, tags);
    RUN_META_TEST(AudioMetadata, tags);
    RUN_META_TEST(VideoMetadata, tags);
    RUN_META_TEST(ScriptMetadata, tags);
    RUN_META_TEST(ShaderMetadata, tags, ShaderStageFlagBits::Compute);
    RUN_META_TEST(StringMetadata, tags, "en_US", 1);
    RUN_META_TEST(CustomMetadata, tags);
    
    MaterialInputVariable miv0("miv0", glm::vec4(1.0f, 2.0f, 3.0f, 0.5f), 4);
    MaterialInputVariable miv1("miv1", glm::vec4(0.6f, 0.2f, 0.8f, 0.0f), 3);
    MaterialInputTexture mit0("mit0", StringHash(58815182));
    MaterialInputTexture mit1("mit1", StringHash(266628));
    MaterialInputTexture mit2("mit2", StringHash(949122056));
    RUN_META_TEST(MaterialTemplateMetadata, tags, MaterialFamily::Toon, FileHash(115616), StringHash(51191912), 90, {miv0, miv1}, {mit0, mit1, mit2});
    RUN_META_TEST(MaterialInstanceMetadata, tags, StringHash(1998005152));

    // TODO add a set of checked elements
    return TestResults(true, "");
}

void MetadataSerializationTests::cleanup() {
    //
}

}

