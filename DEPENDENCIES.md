# Dependencies
**TODO** dependencies should probably be downloaded automatically via CMake scripts.

**WARNING**: You should always prefer to use stable versions of the libraries mentioned in this document.

Like most software projects, the IYFEngine depends on third-party libraries. Many good game development libraries have been created with easy compilation, integration and distribution in mind. Such libraries are often header only or contain very few `.c/.cpp` files that can be built without complicated build scripts.

The IYFEngine has three types of dependencies:

1. **External:** these dependencies are big external libraries that are found and linked with the help of CMake build scripts (SDL2, Boost, etc.).
2. **Regular:** these are (usually, but not always) header only libraries that tightly integrate into the IYFEngine's build system. These libraries tend to contain quite a few files and they may often be updated. To avoid polluting the source tree with tons of external code changes, they are stored in a *dependencies* folder that is not tracked by git.
3. **Internal:** Tightly integrated libraries that are stored directly in the source tree. These libraries tend to consist of very few files or they require engine-specific customizations and need to be tracked in git because of that.

##External dependencies
If you're on Linux, most of these libraries should be available in the OS repositories. However, even if they are, you may wish to use newer versions that you've built yourself. Vulkan SDK is a special case - **always** try to download and install the latest version. Having the most up-to-date tools and validation layers is very important.

0. [SDL2](https://www.libsdl.org/)
1. [SDL2 Mixer](https://www.libsdl.org/projects/SDL_mixer/)
2. [Open Asset Import Library (a.k.a. assimp)](http://assimp.sourceforge.net/)
3. [Lua](https://www.lua.org/) or, preferably, [LuaJIT](http://luajit.org/)
4. [Boost](http://www.boost.org/): at the moment, the only Boost component we're using is boost::filesystem. We've already bumped up the C++ standard to C++17 and switched to std::variant, std::optional and std::any. Unfortunately, as of this writing, filesystem support in GCC is still experimental. We may remove the dependency on Boost once std::filesystem is supported everywhere.
5. [Bullet Physics](http://www.bulletphysics.org/)
6. [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
7. [physfs](https://icculus.org/physfs/). Version 3.0.2 or later.

## Regular dependencies
Before you can build the IYFEngine, you must create a *dependencies* folder in the root directory of this project (in the same directory as this file). Don't worry about polluting the source tree - the *dependencies* folder is included in *.gitignore*. 

Create sub-folders named after these libraries in the "depedencies" folder. You can use the following command on Linux:

    mkdir glm sol2 recastnavigation physfs sqlite rapidjson fmt

Next, download and put the appropriate libraries into those folders. Each folder should contain the root of the appropriate library. Moreover, as mentioned before, try to use the stable versions.

0. [glm](https://github.com/g-truc/glm)
1. [sol2](https://github.com/ThePhD/sol2/)
2. [recastnavigation](https://github.com/recastnavigation/recastnavigation/)
3. [rapidjson](https://github.com/Tencent/rapidjson.git)
4. [sqlite](https://www.sqlite.org/download.html): using the amalgamation.
5. [fmt](https://github.com/fmtlib/fmt): make sure the version is 5.3.0 or newer
6. [Compressonator SDK](https://github.com/GPUOpen-Tools/Compressonator): this may be a bit troublesome. Follow the [docs](https://compressonator.readthedocs.io/en/latest/build_from_source/build_instructions.html#build-instructions-for-compressonator-sdk-libs). When I last tried it, *CMakeLists.txt* files were all in lower case. I used this to perform the rename:
```c++
// TO BUILD AND RUN
// g++ -std=c++17 CompressonatorRenamer.cpp
// ./a.out

#include <iostream>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

int main() {
    std::vector<Path> toChange;
    
    for (const auto& di : fs::recursive_directory_iterator("compressonator")) {
        if (di.path().filename() == "cmakelists.txt") {
            toChange.emplace_back(di.path());
        }
    }
    
    for (const Path& path : toChange) {
        Path newPath = path;
        newPath.remove_filename();
        
        newPath += "CMakeLists.txt";
        
        std::cout << path << "->" << newPath << "\n";
        fs::rename(path, newPath);
    }
    
    return 0;
}
```
After the build, go to the *lib* folder and copy the *libCMP_Compressonator.a* to *dependencies/compressonator/compressonator.a*. Likewise, copy *cmp_compressonatorlib/compressonator.h* to *dependencies/compressonator/Compressonator.h*

If you receive compilation errors that talk about missing headers or source files, look into the *CMakeLists.txt* files for hints. If you cloned the repositories directly into the created folders, it's possible that you ended up with something like *dependencies/glm/glm/glm/glm.hpp* (note the extra subdirectory) when you actually needed *dependencies/glm/glm/glm.hpp*.
    
## Internal dependencies
Unless you're one of IYFEngine's developers or **really** need a newer version, you don't have to do anything because these have already been included and will be built automatically.

0. [Dear Imgui](https://github.com/ocornut/imgui): stored in *include/graphics/imgui* and *src/graphics/imgui* This library is stored in the source tree because of the customized *imconfig.h* file and an engine specific backend (check *ImGuiImplementation.cpp*). Make sure to **NEVER OVERWRITE** *imconfig.h* when updating this library or **Bad Things™** will happen.
1. [stb](https://github.com/nothings/stb): stored in *include/stb*. Required headers are:
 * *stb\_image.h*
 * *stb\_image\_resize.h*
 * *stb\_image\_write.h*
 * *stb\_rect\_pack.h*
 * *stb\_text\_edit.h*
 * *stb\_true\_type.h*
            
    stb libraries are stored in the source tree because of Dear Imgui.
2. [miniz](https://github.com/richgel999/miniz/releases): stored in *include/miniz* and *src/miniz*.
3. [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator): stored in *include/graphics/vulkan*. I noticed that there were some breaking changes between versions 1 and 2. Since the API doesn't seem to be completely stable, having a specific version of the library in the source tree seems to be a better choice.
4. [IYFThreading](https://github.com/manvis/IYFThreading): stored in *include/threading* and *src/threading*. My own thread pool and a simple profiler. Like Dear Imgui, it uses a customizable project specific header file.
    
##Future dependencies
0. stb\_vorbis for audio effect decompression and music decompression. Only an option. I may need to look into something that supports more formats or use opus instead. Need to research things first.
1. Steam audio or OpenAL soft for 3D audio.
2. Valve's GameNetworkingSockets OR netcode.io OR doing things from scratch with ASIO (which, considering my lack of network programming experience, may end up being a recipe for disaster) for networking.
    
## No longer used
These libraries have been replaced on removed.

0. [gli](https://github.com/g-truc/gli): was replaced because I started using a custom texture format.
1. [selene](https://github.com/jeremyong/Selene/): was replaced because sol2 is faster and better maintained.

