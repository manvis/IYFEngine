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

// Set in meson.build #define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/fwd.h"

namespace rj = rapidjson;

namespace iyf {
using PrettyStringWriter = rj::PrettyWriter<rj::StringBuffer, rj::UTF8<char>, rj::UTF8<char>, rj::CrtAllocator, 0>;
using JSONObject = rj::Value;//rj::Document::ValueType;

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
    
    /// If this returns true, you don't need to call PrettyStringWriter::StartObject() before calling serializeJSON() and PrettyStringWriter::EndObject()
    /// after calling it.
    ///
    /// TextSerializable objects that return false when makesJSONRoot() is called are useful when the final representation is built bit by bit by derived
    /// classes and it isn't clear when PrettyStringWriter::StartObject() and PrettyStringWriter::EndObject() need to be called.
    virtual bool makesJSONRoot() const = 0;
    
    /// \remark This function performs a copy to make the std::string. If you have very long strings or need maximum performance, it would
    /// make sense to copy the code from this function and use the data buffer generated by rapidjson directly.
    ///
    /// \returns This object serialized to a JSON string.
    std::string getJSONString() const;
    
    virtual ~TextSerializable() {}
};

}

#endif // IYF_TEXT_SERIALIZABLE_HPP
