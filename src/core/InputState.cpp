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

#include "core/InputState.hpp"

#include <algorithm>
#include <unordered_map>

#include "core/configuration/Configuration.hpp"
#include "core/Logger.hpp"
#include "core/InputListener.hpp"
#include "core/Engine.hpp"

namespace iyf
{
InputState::InputState(Engine* engine, Configuration* config) : Configurable(config), engine(engine) {
    keyStates.fill(false);
    mouseButtonStates.fill(false);
    controllerButtonStates.fill(false);
    mouseDeltaX = 0;
    mouseDeltaY = 0;
    mouseWheelDeltaX = 0;
    mouseWheelDeltaY = 0;
    noMouseMovement = true;
    noMouseWheelMovement = true;
    mouseX = 0;
    mouseY = 0;
    
    screenWidth = config->getValue(HS("width"), ConfigurationValueFamily::Graphics);
    screenHeight = config->getValue(HS("height"), ConfigurationValueFamily::Graphics);
//     screenWidth = config->getIntValue("width");
//     screenHeight = config->getIntValue("height");
}

void InputState::pollInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
                keyStates[event.key.keysym.scancode] = event.key.state;
                
                for (const auto& l : inputListeners) {
                    l->onKeyPressed(event.key.keysym.sym, event.key.keysym.scancode, static_cast<KeyModifierFlagBits>(event.key.keysym.mod));
                }
                break;
            case SDL_KEYUP:
                keyStates[event.key.keysym.scancode] = event.key.state;
                
                for (const auto& l : inputListeners) {
                    l->onKeyReleased(event.key.keysym.sym, event.key.keysym.scancode, static_cast<KeyModifierFlagBits>(event.key.keysym.mod));
                }
                break;
            case SDL_MOUSEWHEEL:
                noMouseWheelMovement = false;
                mouseWheelDeltaX = event.wheel.x;
                mouseWheelDeltaY = event.wheel.y;
                if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                    mouseWheelDeltaX *= -1;
                    mouseWheelDeltaY *= -1;
                }
                
                for (const auto& l : inputListeners) {
                    l->onMouseWheelMoved(mouseWheelDeltaX, mouseWheelDeltaY);
                }
                
                break;
            case SDL_MOUSEMOTION:
                noMouseMovement = false;
                mouseDeltaX = event.motion.xrel;
                mouseDeltaY = event.motion.yrel;
                
                //mouseX += event.motion.x;
                //mouseY += event.motion.y;
                if (engine->isEditorMode()) {
                    mouseX = event.motion.x;
                    mouseY = event.motion.y;
                } else {
                    mouseX += event.motion.xrel;
                    mouseY += event.motion.yrel;
                }
                //LOG_D(event.motion.x << " " << event.motion.y)
                
                updateMouse();                
                break;
            case SDL_MOUSEBUTTONDOWN:
                switch (event.button.button) {
                case SDL_BUTTON_LEFT:
                    mouseButtonStates[static_cast<int>(MouseButton::Left)] = true;
                    break;
                case SDL_BUTTON_MIDDLE:
                    mouseButtonStates[static_cast<int>(MouseButton::Middle)] = true;
                    break;
                case SDL_BUTTON_RIGHT:
                    mouseButtonStates[static_cast<int>(MouseButton::Right)] = true;
                    break;
                case SDL_BUTTON_X1:
                    mouseButtonStates[static_cast<int>(MouseButton::Extra1)] = true;
                    break;
                case SDL_BUTTON_X2:
                    mouseButtonStates[static_cast<int>(MouseButton::Extra2)] = true;
                    break;
                }
                
                for (const auto& l : inputListeners) {
                    l->onMouseButtonDown(event.button.x, event.button.y, event.button.clicks, static_cast<MouseButton>(event.button.button));
                }
                
                break;
            case SDL_MOUSEBUTTONUP:
                switch (event.button.button) {
                case SDL_BUTTON_LEFT:
                    mouseButtonStates[static_cast<int>(MouseButton::Left)] = false;
                    break;
                case SDL_BUTTON_MIDDLE:
                    mouseButtonStates[static_cast<int>(MouseButton::Middle)] = false;
                    break;
                case SDL_BUTTON_RIGHT:
                    mouseButtonStates[static_cast<int>(MouseButton::Right)] = false;
                    break;
                case SDL_BUTTON_X1:
                    mouseButtonStates[static_cast<int>(MouseButton::Extra1)] = false;
                    break;
                case SDL_BUTTON_X2:
                    mouseButtonStates[static_cast<int>(MouseButton::Extra2)] = false;
                    break;
                }
                
                for (const auto& l : inputListeners) {
                    l->onMouseButtonUp(event.button.x, event.button.y, event.button.clicks, static_cast<MouseButton>(event.button.button));
                }
                
                break;
            case SDL_TEXTINPUT:
                for (const auto& l : inputListeners) {
                    l->onTextInput(event.text.text);
                }
                
                break;
            case SDL_CONTROLLERDEVICEADDED:
                //AddController( sdlEvent.cdevice );
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                //RemoveController( sdlEvent.cdevice );
                break;
            case SDL_CONTROLLERAXISMOTION:
                //OnControllerAxis( sdlEvent.caxis );
                break;
            case SDL_CONTROLLERBUTTONUP:
                controllerButtonStates[event.cbutton.button] = false;
                break;
            case SDL_CONTROLLERBUTTONDOWN:
                //std::stringstream ss;
                //ss << event.cbutton.button;
                //Logger::log(ss.str());
                controllerButtonStates[event.cbutton.button] = true;
                break;
            default:
                break;
        }
    }

    if (noMouseMovement) {
        mouseDeltaX = 0;
        mouseDeltaY = 0;
    } else {
        noMouseMovement = true;
    }

    if (noMouseWheelMovement) {
        mouseWheelDeltaX = 0;
        mouseWheelDeltaY = 0;
    } else {
        noMouseWheelMovement = true;
    }
}

void InputState::updateMouse() {
    if (mouseX < 0) {
        mouseX = 0;
    } else if (mouseX > screenWidth) {
        mouseX = screenWidth;
    }

    if (mouseY < 0) {
        mouseY = 0;
    } else if (mouseY > screenHeight) {
        mouseY = screenHeight;
    }
    
    for (const auto& l : inputListeners) {
        // TODO Is the last param really OK? We only have a single window, so it SHOULD
        l->onMouseMoved(mouseX, mouseY, mouseDeltaX, mouseDeltaY, true);
    }
}
void InputState::handleConfigChange(const ConfigurationValueMap& changedValues) {
    bool updateMousePosition = false;
    
    auto result = changedValues.find({HS("width"), ConfigurationValueFamily::Graphics});
    if (result != changedValues.end()) {
        screenWidth = result->second;
    }
    
    result = changedValues.find({HS("height"), ConfigurationValueFamily::Graphics});
    if (result != changedValues.end()) {
        screenHeight = result->second;
    }
    
    if (updateMousePosition) {
        updateMouse();
    }
}

bool InputState::addInputListener(InputListener* inputListener) {
    auto it = std::find(inputListeners.begin(), inputListeners.end(), inputListener);
    if (it != inputListeners.end()) {
        return false;
    }
    
    inputListeners.push_back(inputListener);
    
    return true;
}

bool InputState::removeInputListener(InputListener* inputListener) {
    auto it = std::find(inputListeners.begin(), inputListeners.end(), inputListener);
    if (it == inputListeners.end()) {
        return false;
    }
    
    inputListeners.erase(it);
    return true;
}

InputState::~InputState() {
    //dtor
}
}
