cmake_minimum_required(VERSION 3.9)
project(IYFEngine)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# TODO in stable versions BUILD_TOOLS_AND_TESTS and PROFILER_ENABLED should be false by default
set(BUILD_TOOLS_AND_TESTS True CACHE BOOL "Build the editor, other tools and tests")
set(PROFILER_ENABLED True CACHE BOOL "Build the editor, other tools and tests")

set(EXECUTABLE_OUTPUT_DIR ${CMAKE_BINARY_DIR} CACHE PATH "Executable output path")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_DIR})

# We'll need c++17
set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -fdiagnostics-color=always")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g")

# GLM
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)
add_definitions(-DGLM_FORCE_LEFT_HANDED)
add_definitions(-DGLM_FORCE_RADIANS)

# IYFThreading
# Tool builds require the profiler to be enabled
if (BUILD_TOOLS_AND_TESTS OR PROFILER_ENABLED)
    add_definitions(-DIYF_ENABLE_PROFILING -DIYF_THREAD_POOL_PROFILE)
endif()

# XXHash
add_definitions(-DXXH_INLINE_ALL)

# Required by miniz to support large files on platforms that use glibc
# TODO do I need platform checks or can I keep this define as-is?
add_definitions(-D_LARGEFILE64_SOURCE)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(SANITIZER "none" CACHE STRING "Optional sanitizers that can aid in bug checking")
    set_property(CACHE SANITIZER PROPERTY STRINGS address leak none thread undefined)
    
    if (${SANITIZER} STREQUAL "address")
        add_definitions(-DACTIVE_SANITIZER_NAME=\"address\")
        
        MESSAGE(STATUS "Address sanitizer enabled")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    elseif (${SANITIZER} STREQUAL "leak")
        add_definitions(-DACTIVE_SANITIZER_NAME=\"leak\")
        
        MESSAGE(STATUS "Leak sanitizer enabled")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
    elseif (${SANITIZER} STREQUAL "none")
        add_definitions(-DACTIVE_SANITIZER_NAME=\"none\")
        
        MESSAGE(STATUS "All sanitizers disabled")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-sanitize=all")
    elseif (${SANITIZER} STREQUAL "thread")
        add_definitions(-DACTIVE_SANITIZER_NAME=\"thread\")
        
        MESSAGE(STATUS "Thread sanitizer enabled")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
    elseif (${SANITIZER} STREQUAL "undefined")
        add_definitions(-DACTIVE_SANITIZER_NAME=\"undefinedBehaviour\")
        
        MESSAGE(STATUS "Undefined behaviour sanitizer enabled")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fno-omit-frame-pointer")
    endif()
endif()

option(DEBUG_MODE "Compile with additional debug and validation features enabled. Some other similar features (e.g., Vulkan debug and validation layers) are independent of this setting and can be enabled or disabled using configuration files." OFF)
if (DEBUG_MODE)
    MESSAGE(STATUS "Building with additional debug and validation")
    add_definitions(-DENG_DEBUG_MODE)
endif()

option(STATIC_PHYSFS "If on, the build system compiles and statically links the PhysFS source from the dependencies folder. If off, it looks for a shared library installed in the system." ON)

# Local includes go before everything else
include_directories(${IYFEngine_SOURCE_DIR}/include)

# Set up external dependencies 
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

find_package(SDL2Mix REQUIRED)
include_directories(${SDL2_MIX_INCLUDE_DIR})

find_package(Lua REQUIRED)
include_directories(${LUA_INCLUDE_DIR})

# WARNING, Don't blindly overwrite physfsrwops.h and physfsrwops.c when updating the internal copy of physfs.
# some modifications have been made to those files.
if (STATIC_PHYSFS)
    include_directories(dependencies/physfs/src)
    set(PHYSFS_SOURCES
            dependencies/physfs/src/physfs.c
            dependencies/physfs/src/physfs_archiver_7z.c
            dependencies/physfs/src/physfs_archiver_dir.c
            dependencies/physfs/src/physfs_archiver_grp.c
            dependencies/physfs/src/physfs_archiver_hog.c
            dependencies/physfs/src/physfs_archiver_iso9660.c
            dependencies/physfs/src/physfs_archiver_mvl.c
            dependencies/physfs/src/physfs_archiver_qpak.c
            dependencies/physfs/src/physfs_archiver_slb.c
            dependencies/physfs/src/physfs_archiver_unpacked.c
            dependencies/physfs/src/physfs_archiver_vdf.c
            dependencies/physfs/src/physfs_archiver_wad.c
            dependencies/physfs/src/physfs_archiver_zip.c
            dependencies/physfs/src/physfs_byteorder.c
            #dependencies/physfs/src/physfs_platform_apple.m  # TODO add this when apple platforms are finally supported
            dependencies/physfs/src/physfs_platform_haiku.cpp 
            dependencies/physfs/src/physfs_platform_os2.c
            dependencies/physfs/src/physfs_platform_posix.c
            dependencies/physfs/src/physfs_platform_qnx.c
            dependencies/physfs/src/physfs_platform_unix.c
            dependencies/physfs/src/physfs_platform_windows.c
            dependencies/physfs/src/physfs_platform_winrt.cpp
            dependencies/physfs/src/physfs_unicode.c)
    add_library(physfs STATIC ${PHYSFS_SOURCES})
    #target_link_libraries(physfs)
    set(PHYSFS_LIBRARY physfs)
else (STATIC_PHYSFS)
    find_package(PhysFS REQUIRED)
    include_directories(${PHYSFS_INCLUDE_DIR})
endif (STATIC_PHYSFS)

find_package(Bullet REQUIRED)
# Marking bullet as SYSTEM because it has an annoying warning
include_directories(SYSTEM ${BULLET_INCLUDE_DIRS})

find_package(Boost COMPONENTS system filesystem REQUIRED)
include_directories(${Boost_FILESYSTEM_DIR})
# TODO replace when c++17 support is decent enough. std libs should replace boost's variant, optional, filesystem, etc.
include_directories(${Boost_INCLUDE_DIRS})
add_definitions(-DUSE_BOOST)

# Deeply integrated dependencies that are stored inside the source tree
# imgui
include_directories(include/graphics/imgui)
# stb
include_directories(include/stb)
# fmt
include_directories(include/format)
# miniz
include_directories(include/miniz)

# TODO create a system to download and extract the required files from git repos, maybe by using: ExternalProject_Add
# Set up header and integrated library dependencies
# glm
include_directories(dependencies/glm)
# sol2
include_directories(dependencies/sol2/single)
# rapidjson
include_directories(dependencies/rapidjson/include/)
add_definitions(-DRAPIDJSON_HAS_STDSTRING)
# recast (recast navigation)
include_directories(dependencies/recastnavigation/Recast/Include)
# detour (recast navigation)
include_directories(dependencies/recastnavigation/Detour/Include)
# detour crowds (recast navigation)
include_directories(dependencies/recastnavigation/DetourCrowd/Include)
# detour tile caching (recast navigation)
include_directories(dependencies/recastnavigation/DetourTileCache/Include)
# recast navigation debug utilities (recast navigation)
include_directories(dependencies/recastnavigation/DebugUtils/Include)
# sqlite
include_directories(dependencies/sqlite)

option(GLEW_DYNAMIC "Look for and link a dynamic version of the glew library, only valid when using OpenGL" OFF)

set(GRAPHICS_BACKEND "Vulkan" CACHE STRING "Graphics Backend that the engine will use.")
set_property(CACHE GRAPHICS_BACKEND PROPERTY STRINGS Vulkan OpenGL)

if (GRAPHICS_BACKEND MATCHES "Vulkan")
    MESSAGE(STATUS "Building with Vulkan. Looking for SDK in:")
    MESSAGE(STATUS $ENV{VULKAN_SDK}/include)

    add_definitions(-DBUILD_WITH_VULKAN)
    
    include_directories($ENV{VULKAN_SDK}/include)
    
    if (CMAKE_BUILD_TYPE MATCHES Debug)
        if (WIN32)
            LINK_DIRECTORIES($ENV{VULKAN_SDK}/source/lib)
        elseif (UNIX)
            LINK_DIRECTORIES($ENV{VULKAN_SDK}/../source/lib)
        endif()
        MESSAGE(STATUS "Will link Vulkan debug libs because of debug build mode")
    else ()
        if (WIN32)
            LINK_DIRECTORIES($ENV{VULKAN_SDK}/Bin)
        elseif (UNIX)
            LINK_DIRECTORIES($ENV{VULKAN_SDK}/lib)
        endif()
        #set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} $ENV{VULKAN_SDK}/lib)        
    endif ()
  
    if (WIN32)
        set(LIBRARIES "vulkan-1")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
        
        # TODO libshaderc linking on Windows
    elseif (UNIX)
        set(LIBRARIES "vulkan")
        find_package(X11 REQUIRED)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_XLIB_KHR")
        #find_package(XCB REQUIRED)
        #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_XCB_KHR")
        
        LINK_DIRECTORIES($ENV{VULKAN_SDK}/lib/libshaderc)
    else ()
    endif ()
elseif (GRAPHICS_BACKEND MATCHES "OpenGL")
    add_definitions(-DBUILD_WITH_OPENGL)

    if (GLEW_DYNAMIC)
        find_package(GLEW REQUIRED)
        include_directories(${GLEW_INCLUDE_DIR})
    else (GLEW_DYNAMIC)
        include_directories(/dependencies/glew/include/GL)

        if (WIN32)
            add_definitions(-DGLEW_STATIC)
        endif(WIN32)
    endif (GLEW_DYNAMIC)

    find_package(OpenGL REQUIRED)
    include_directories(${OPENGL_INCLUDE_DIRS})
endif (GRAPHICS_BACKEND MATCHES "Vulkan")

# Target name IYFEngine
add_subdirectory(src)

if (BUILD_TOOLS_AND_TESTS)
    find_package(Assimp REQUIRED)
    include_directories(${ASSIMP_INCLUDE_DIR})
    include_directories(testing)
    add_subdirectory(tools/IYFEditor/src)
endif()
