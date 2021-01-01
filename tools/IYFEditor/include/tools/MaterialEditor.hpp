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

#ifndef IYF_MATERIAL_EDITOR_HPP
#define IYF_MATERIAL_EDITOR_HPP

#include "graphics/materials/MaterialLogicGraph.hpp"
#include "utilities/logicGraph/LogicGraphEditor.hpp"

namespace iyf::editor {

class MaterialEditor : public LogicGraphEditor<MaterialLogicGraph> {
public:
    MaterialEditor();
    virtual std::string getWindowName() const final override;
    virtual ~MaterialEditor();
    
    void setPath(Path path);
    inline const Path& getPath() const {
        return filePath;
    }
    
    static std::string CreateNewGraph(MaterialFamily family);
protected:
    virtual void onButtonClick(LogicGraphEditorButtonFlagBits button) final override;
    virtual void onDrawButtonRow() final override;
    virtual void drawNodeExtraProperties(MaterialNode& node) final override;
    void drawFilePopup(bool isLoadPopup);
    
    virtual std::unique_ptr<MaterialLogicGraph> makeNewGraph(const NewGraphSettings& settings) final override;
    
    std::vector<char> nameBuffer;
    std::vector<std::string> materialFamilyNames;
    Path filePath;
};

}

#endif // IYF_MATERIAL_EDITOR_HPP
