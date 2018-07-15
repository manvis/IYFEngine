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

#ifndef SHADINGPIPELINE_HPP
#define SHADINGPIPELINE_HPP

#include "core/interfaces/Serializable.hpp"

#include <vector>

namespace iyf {
class VertexShader {
public:
    // Vertex input id
    // vertex output id
    // code? 
};

class FragmentShader {
public:
};

class ShadingPipelineManager : public Serializable  {
public:
    ShadingPipelineManager(bool separateShadingPassEnabled) : separateShadingPassEnabled(separateShadingPassEnabled) {}
    
    inline bool isSeparateShadingPassEnabled() const {
        return separateShadingPassEnabled;
    }
    // TODO supports instanced or not. Data can be drawn from push constants, push constants may have the mesh id and data would be drawn from buffers OR per instance vertex data
    // vertex input variations: regular, bones
//    std::vector<???> lightingModels;
    virtual void serialize(File& fw) const override {
        //
    }
    
    virtual void deserialize(File& fr) override {
        //
    }
protected:
    bool separateShadingPassEnabled;
};

}

#endif /* SHADINGPIPELINE_HPP */

