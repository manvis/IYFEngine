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

#include "FileSystem.hpp"

#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

inline iyf::UserInfo BuildUserInfo() {
    using namespace iyf;
    UserInfo info{};

    const uid_t euid = geteuid();

    constexpr size_t bufferSize = 8192;

    std::vector<char> buffer;
    buffer.resize(bufferSize);

    passwd pwd{};
    passwd* pwdPtr = nullptr;
    const int result = getpwuid_r(euid, &pwd, buffer.data(), buffer.size(), &pwdPtr);
    if (result != 0 || pwdPtr == nullptr)
    {
        return info;
    }

    info.user.id = pwd.pw_uid;
    info.user.name = pwd.pw_name;

    int numGroups = 512;
    std::vector<gid_t> groups;
    groups.resize(numGroups);

    const int groupListResult = getgrouplist(pwd.pw_name, pwd.pw_gid, groups.data(), &numGroups);
    if (groupListResult != 0 || pwdPtr == nullptr)
    {
        info.user.id = -1;
        info.user.name.clear();

        return info;
    }

    std::vector<char> groupBuffer;
    groupBuffer.resize(512);

    info.groups.reserve(numGroups);
    for (int i = 0; i < numGroups; ++i) {
        NameAndID nid;

        nid.id = groups[i];

        group grp{};
        group* grpPtr = nullptr;
        const int groupResult = getgrgid_r(groups[i], &grp, groupBuffer.data(),
            groupBuffer.size(), &grpPtr);

        if (groupResult != 0 || grpPtr == nullptr) {
            info.user.id = -1;
            info.user.name.clear();
            info.groups.clear();

            return info;
        }

        nid.name = grp.gr_name;

        info.groups.emplace_back(std::move(nid));
    }

    return info;
}
#endif

namespace iyf {

FileSystem::~FileSystem() {}

const UserInfo& FileSystem::getUserInfo() const {
    static const UserInfo info = BuildUserInfo();
    return info;
}
}
