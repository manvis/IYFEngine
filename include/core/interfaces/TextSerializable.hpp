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

#ifndef IYF_TEXT_SERIALIZABLE_HPP
#define IYF_TEXT_SERIALIZABLE_HPP

#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"

namespace rj = rapidjson;

namespace iyf {
using PrettyStringWriter = rj::PrettyWriter<rj::StringBuffer>;
using JSONObject = rj::Document::ValueType;

/// This interface is used by classes that are capable of storing the representation of their state to pretty printed JSON, fit for
/// (quite) efficient storage in version control systems or (minor - JSON isn't all that comfortable for manual manipulation) editing
/// by hand.
///
/// \remark This class utilizes the RapidJSON library
class TextSerializable {
public:
    /// Serializes the object to JSON.
    virtual void serializeJSON(PrettyStringWriter& pw) const = 0;
    
    /// Deserializes the data into the current object.
    virtual void deserializeJSON(JSONObject& jo) = 0;
};

}

#endif // IYF_TEXT_SERIALIZABLE_HPP
