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

#ifndef INPUTMAPPINGS_H_INCLUDED
#define INPUTMAPPINGS_H_INCLUDED

#include "utilities/Flags.hpp"
#include "utilities/enum_flag_ops.hpp"

#define USING_SDL

#ifdef USING_SDL
#include <SDL2/SDL.h>

namespace iyf {
enum class MouseButton {
    Left   = SDL_BUTTON_LEFT,
    Middle = SDL_BUTTON_MIDDLE,
    Right  = SDL_BUTTON_RIGHT,
    Extra1 = SDL_BUTTON_X1,
    Extra2 = SDL_BUTTON_X2,
    /// \warning Used to determine array size. DO NOT TOUCH
    COUNT  = 5
};

enum class KeyModifierFlagBits : std::uint32_t {
    LeftShift  = KMOD_LSHIFT,
    RightShift = KMOD_RSHIFT,
    LeftCtrl   = KMOD_LCTRL,
    RightCtrl  = KMOD_RCTRL,
    LeftAlt    = KMOD_LALT,
    RightAlt   = KMOD_RALT,
    LeftGUI    = KMOD_LGUI,
    RightGUI   = KMOD_RGUI,
    NumLock    = KMOD_NUM,
    CapsLock   = KMOD_CAPS,
    AltGr      = KMOD_MODE,
    Ctrl       = LeftCtrl  | RightCtrl,
    Shift      = LeftShift | RightShift,
    Alt        = LeftAlt   | RightAlt,
    GUI        = LeftGUI   | RightGUI,
};

// TODO map key and scancode? values to our own enum.

} 
// #elif Some other library

#endif

namespace iyf {
using KeyModifierFlags = Flags<KeyModifierFlagBits>;
IYF_DEFINE_ENUM_FLAG_OPS(KeyModifierFlagBits)
}

#endif // INPUTMAPPINGS_H_INCLUDED
