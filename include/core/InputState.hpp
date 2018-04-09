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

#ifndef INPUTSTATE_H
#define INPUTSTATE_H

#include <memory>
#include <array>
#include <SDL2/SDL.h>

#include "core/InputMappings.hpp"
#include "core/interfaces/Configurable.hpp"
#include "utilities/NonCopyable.hpp"

namespace iyf {

using namespace std;

class Engine;
class InputListener;
class InputState : public Configurable, private NonCopyable {
    // Allow Engine class to poll the input and construct the object
    friend Engine;

    public:
        ~InputState();

        inline int getMouseDeltaX() const {
            return mouseDeltaX;
        }

        inline int getMouseDeltaY() const {
            return mouseDeltaY;
        }

        inline int getMouseWheelDeltaX() const {
            return mouseWheelDeltaX;
        }

        inline int getMouseWheelDeltaY() const {
            return mouseWheelDeltaY;
        }

        inline bool isKeyPressed(SDL_Scancode key) const {
            return keyStates[key];
        }

        inline bool isKeyPressed(SDL_Keycode key) const {
            return keyStates[SDL_GetScancodeFromKey(key)];
        }

        inline bool isMouseClicked(MouseButton button) const {
            return mouseButtonStates[static_cast<unsigned int>(button)];
        }

        inline int getMouseX() const {
            return mouseX;
        }
        
        inline int getMouseY() const {
            return mouseY;
        }
        
        inline bool getKeyModifierState(KeyModifierFlags modifier) const {
            return static_cast<std::uint32_t>(SDL_GetModState()) & static_cast<std::uint32_t>(modifier);
        }
        
        inline void setClipboardValue(const std::string& value) {
            SDL_SetClipboardText(value.c_str());
        }
        
        inline void setClipboardValue(const char* value) {
            SDL_SetClipboardText(value);
        }
        
        inline void setMouseRelativeMode(bool enabled) {
            if (SDL_SetRelativeMouseMode(enabled ? SDL_TRUE : SDL_FALSE) != 0) {
                // TODO to handle or not to handle?
            }
        }

        inline std::string getClipboardValue() {
            char* value = SDL_GetClipboardText();
            if (value == nullptr) {
                return "";
            }
            
            std::string valueStr(value);
            SDL_free(value);
            
            return valueStr;
        }
        
        void handleConfigChange(const ConfigurationValueMap& changedValues) override;

        bool addInputListener(InputListener* inputListener);
        bool removeInputListener(InputListener* inputListener);
    protected:
    private:
        InputState(Engine* engine, Configuration* config);
        
        void pollInput();
        void updateMouse();

        Engine* engine;
        
        bool noMouseMovement;
        bool noMouseWheelMovement;
        int mouseDeltaX, mouseDeltaY, mouseX, mouseY;
        std::int32_t screenWidth;
        std::int32_t screenHeight;
        int mouseWheelDeltaX, mouseWheelDeltaY;
        array<bool, SDL_Scancode::SDL_NUM_SCANCODES> keyStates;
        array<bool, static_cast<unsigned int>(MouseButton::COUNT)> mouseButtonStates;
        array<bool, SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX> controllerButtonStates;
        std::vector<InputListener*> inputListeners;
};
}
#endif // INPUTSTATE_H
