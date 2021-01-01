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

#include "core/Engine.hpp"
#include "logging/Logger.hpp"
#include "configuration/Configuration.hpp"
#include "assets/AssetManager.hpp"
#include "assets/typeManagers/TypeManager.hpp"

#include "fmt/ostream.h"

namespace iyf {
TypeManager::TypeManager(AssetManager* manager) : manager(manager) {
    if (manager == nullptr) {
        throw std::runtime_error("Manager can't be nullptr");
    }
    
    Configuration* config = manager->getEngine()->getConfiguration();
    loggingRemovals = config->getValue(HS("logAssetRemovals"), ConfigurationValueNamespace::Engine);
    loggingCreations = config->getValue(HS("logAssetCreations"), ConfigurationValueNamespace::Engine);
    
    longTermWorkerPool = manager->getEngine()->getLongTermWorkerPool();
}

void TypeManager::logLeakedAsset(std::size_t id, StringHash nameHash, std::uint32_t count) const {
    const Path path = *manager->getAssetPath(nameHash);
    LOG_W("Asset with id {} loaded from path {} still has {} live references.", id, path, count);
}

void TypeManager::logAssetCreation(std::size_t id, StringHash nameHash, bool isFetch, bool isAsync) const {
    if (isFetch) {
        LOG_V("Fetching an already loaded asset with id {}. Name Hash: {}", id, nameHash.value());
    } else {
        LOG_V("Loading an asset with id {}. Name Hash: {}. Async: {}", id, nameHash.value(), isAsync);
    }
}

void TypeManager::logAssetRemoval(std::size_t id, StringHash nameHash) const {
    LOG_V("Removing asset with id {}. Name Hash: {}", id, nameHash.value());
}

void TypeManager::notifyRemoval(StringHash nameHash) {
    manager->notifyRemoval(nameHash);
}

}
