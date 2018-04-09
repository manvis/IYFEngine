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

#ifndef INPUTLISTENER_H
#define INPUTLISTENER_H

#include "core/InputMappings.hpp"

namespace iyf {
class InputListener {
    public:
        virtual void onMouseMoved(int cursorXPos, int cursorYPos, int deltaX, int deltaY, bool windowHasFocus) = 0;
        virtual void onMouseWheelMoved(int deltaX, int deltaY) = 0;
        virtual void onMouseButtonDown(int cursorXPos, int cursorYPos, int clickCount, MouseButton button) = 0;
        virtual void onMouseButtonUp(int cursorXPos, int cursorYPos, int clickCount, MouseButton button) = 0;
        virtual void onKeyPressed(SDL_Keycode keycode, SDL_Scancode scancode, KeyModifierFlags keyModifier) = 0;
        virtual void onKeyReleased(SDL_Keycode keycode, SDL_Scancode scancode, KeyModifierFlags keyModifier) = 0;
        virtual void onTextInput(const char* text) = 0;

        virtual ~InputListener();
    protected:
    private:
};
}
#endif // INPUTLISTENER_H
