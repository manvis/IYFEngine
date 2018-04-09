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

#ifndef PRODUCTID_HPP
#define PRODUCTID_HPP

#include <string>

namespace iyf {
/// A data structure that uniquely identifies a version of a game.
///
/// \warning The data in this class is not supposed to be used for display due to restrictive formatting requirements. Please use
/// the Localization database to store any strings that will be shown to end user.
class ProductID {
public:
    ProductID() = default;
    
    /// Constructs a new ProductID.
    ///
    /// \warning The specified strings will be used to create (or locate) the save game and configuration directories. They MUST be UTF-8 encoded,
    /// preferably short (exception thrown if size() > 64) and SHOULD only contain LETTERS, NUMBERS and SPACES. Avoid punctuation because some 
    /// operating systems may think it has special meaning.
    ///
    /// \param companyName Name of the company that created this game. Read the warning for formatting requirements.
    /// \param gameName Name of the game. Read the warning for formatting requirements.
    /// \param version Version of the game. Pack the version however you like, just make sure it increases with each release and (for Android 
    /// compatibility) that the value is not greater than 2100000000.
    ProductID(const std::string& companyName, const std::string& gameName, const std::uint32_t version);

    std::string getCompanyName() const {
        return companyName;
    }
    
    std::string getGameName() const {
        return gameName;
    }
    
    std::uint32_t getVersion() const {
        return version;
    }
    
    // We can't use the Serializable interface here because it relies on PhysFS being initialized and
    // PhysFS initialization relies on ProductID data.
    
    bool serialize(const std::string& path) const;
    bool deserialize(const std::string& path);
private:
    
    /// Name of the company that created this game
    std::string companyName;
    
    /// Name of the game itself
    std::string gameName;
    
    /// Version identifier
    std::uint32_t version;
};
}
#endif /* PRODUCTID_HPP */

