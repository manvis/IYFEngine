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

#include "MeshImporterState.h"
#include "core/InputState.hpp"
#include "core/filesystem/File.hpp"

namespace iyf {
namespace editor {

MeshImporterState::MeshImporterState(Engine* engine) : GameState(engine) {
    //ctor
}

void MeshImporterState::initialize() {
//    VC vc(VC::Name::Position, VC::Size::Three, VC::Type::Float);
//    VC vc2(VC::Name::UVCoord0, VC::Size::Two, VC::Type::Float);
//    VC vc3(VC::Name::Normal, VC::Size::Two, VC::Type::Float);
//    VF vf({vc2, vc, vc3});

//    auto const &c = vf.getComponents();
//    for (size_t i = 0; i < c.size(); ++i) {
////        LOG_D(i << "--" << static_cast<uint32_t>(c[i].getName()) << " " << 
////                static_cast<uint32_t>(c[i].getSize()) << " " <<
////                static_cast<uint32_t>(c[i].getType()) << " " <<
////                static_cast<uint32_t>(c[i].getMode()) << " ");
//    }
    
    const char MAGIC_NUMBER[4] = {'I', 'Y', 'F', 'M'};
    
    File fw("test", File::OpenMode::Write);
    fw.writeBytes(MAGIC_NUMBER, sizeof(char) * 4);
    fw.writeInt8(64);
    fw.writeUInt8(200);
    fw.writeUInt64(12345678901234567890ull);
    
    File fr("test", File::OpenMode::Read);
    std::int8_t r1;
    std::uint8_t r2;
    std::uint64_t r3;
    char mNum[4];
    
    fr.readBytes(mNum, sizeof(char) * 4);
    fr.readInt8(&r1);
    fr.readUInt8(&r2);
    fr.readUInt64(&r3);
    
//    LOG_D(mNum[0] << mNum[1] << mNum[2] << mNum[3] << " " << (r1 == 64) << " " << (r2 == 200) << " " << r3)
}

void MeshImporterState::pause() {
    //
}

void MeshImporterState::resume() {
    //
}

void MeshImporterState::dispose() {
    //
}

void MeshImporterState::step() {
    InputState* is = engine->getInputState();
    if(is->isKeyPressed(SDL_SCANCODE_ESCAPE)) {
        engine->quit();
    }
}

void MeshImporterState::frame(float delta) {
    //
}

}
}
