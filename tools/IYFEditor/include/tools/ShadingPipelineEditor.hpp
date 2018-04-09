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

#ifndef SHADINGPIPELINEEDITOR_HPP
#define SHADINGPIPELINEEDITOR_HPP

#include <string>
#include <vector>

#include "graphics/ShaderConstants.hpp"

namespace iyf {
    class Renderer;
    class Engine;
    class FileSystem;
}

namespace iyf::editor {
class EditorState;

class ShadingPipelineEditor {
public:
    ShadingPipelineEditor(const Engine* engine, const Renderer* renderer, EditorState* editorState);
    
    void show(bool* open);
private:
    void generate();
    
    char pipelineNameBuf[con::MaxPipelineNameLength];
    char newPipelineNameBuf[con::MaxPipelineNameLength];
    
    std::string currentErrorText;
    
    std::vector<std::string> humanReadableBindingNames;
    std::vector<std::string> humanReadableAttributeNames;
    
    std::vector<std::string> pipelineNames;
    
    EditorState* editorState;
    const Renderer* renderer;
    const Engine* engine;
    int currentPipeline;
    int chosenTemplate;
    bool complete;
    bool wasShownLastTime;
};
}
#endif /* SHADINGPIPELINEEDITOR_HPP */

