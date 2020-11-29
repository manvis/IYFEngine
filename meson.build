project('IYFEngine', ['cpp', 'c'], 
    license : ['BSD-3-Clause'],
    default_options : ['cpp_std=c++17', 'warning_level=3']) #, 'layout=flat'])

cpp_comp = meson.get_compiler('cpp')
cpp_comp_id = cpp_comp.get_id()

# TODO it seems that gcc 9 will support lld as well. Start using it once distros with gcc 9
# become more widely available
if cpp_comp_id == 'clang'# or cpp_comp_id == 'gcc'
    if get_option('use_fast_linker')
        add_global_link_arguments('-fuse-ld=lld', language : 'cpp')
    endif
    
    # Use https://github.com/aras-p/ClangBuildAnalyzer to examine the results 
    # accross multiple files because the output is per-file and may be difficult
    # to examine by hand
    if get_option('trace_compilation_times')
        add_global_arguments('-ftime-trace', language : 'cpp')
    endif
endif

add_global_link_arguments('-lstdc++fs', language : 'cpp')

add_global_arguments(
    # GLM definitions
    '-DGLM_ENABLE_EXPERIMENTAL',
    '-DGLM_FORCE_DEPTH_ZERO_TO_ONE',
    '-DDGLM_FORCE_LEFT_HANDED',
    '-DGLM_FORCE_RADIANS',
    # xxHash definitions
    '-DXXH_INLINE_ALL',
    # required by a couple components
    '-D_LARGEFILE64_SOURCE',
    # rapidjson
    '-DRAPIDJSON_HAS_STDSTRING',
    # vulkan
    '-DBUILD_WITH_VULKAN',
    language : ['cpp', 'c'])
    
chosen_sanitizer = get_option('b_sanitize')
chosen_sanitizer_name = '-DACTIVE_SANITIZER_NAME=UNKNOWN'
if chosen_sanitizer == 'none'
    chosen_sanitizer_name = '-DACTIVE_SANITIZER_NAME="None"'
elif chosen_sanitizer == 'address'
    chosen_sanitizer_name = '-DACTIVE_SANITIZER_NAME="Address"'
elif chosen_sanitizer == 'thread'
    chosen_sanitizer_name = '-DACTIVE_SANITIZER_NAME="Thread"'
elif chosen_sanitizer == 'undefined'
    chosen_sanitizer_name = '-DACTIVE_SANITIZER_NAME="UndefinedBehaviour"'
elif chosen_sanitizer == 'memory'
    chosen_sanitizer_name = '-DACTIVE_SANITIZER_NAME="Memory"'
elif chosen_sanitizer == 'address,undefined'
    chosen_sanitizer_name = '-DACTIVE_SANITIZER_NAME="AddressAndUndefinedBehviour"'
endif

add_global_arguments(chosen_sanitizer_name, language : ['cpp', 'c'])

if get_option('iyf_thread_profiler_enabled') or get_option('iyf_build_tools')
    add_global_arguments('-DIYFT_ENABLE_PROFILING', '-DIYFT_THREAD_POOL_PROFILE', language : 'cpp')
endif

if get_option('physics_engine') == 'bullet'
    physics_dep = dependency('bullet')
    add_global_arguments('-DIYF_PHYSICS_BULLET', language : ['cpp', 'c'])
endif

# Deeply integrated library dependencies
imgui_inc = include_directories('include/graphics/imgui')
stb_inc = include_directories('include/stb')
miniz_inc = include_directories('include/miniz')

# TODO create a system to download and extract the required files from git repos

# Header only library dependencies
glm_inc = include_directories('dependencies/glm')
#include_directories('dependencies/sol2/single')
rapidjson_inc = include_directories('dependencies/rapidjson/include')
recast_inc = include_directories('dependencies/recastnavigation/Recast/Include')
detour_inc = include_directories('dependencies/recastnavigation/Detour/Include')
detour_crowd_inc = include_directories('dependencies/recastnavigation/DetourCrowd/Include')
detour_tile_cache_inc = include_directories('dependencies/recastnavigation/DetourTileCache/Include')
detour_debug_inc = include_directories('dependencies/recastnavigation/DebugUtils/Include')
sqlite_inc = include_directories('dependencies/sqlite')

iyf_core_inc = include_directories('include')

common_project_inc = [iyf_core_inc,
                      imgui_inc,
                      stb_inc,
                      miniz_inc,
                      glm_inc,
                      rapidjson_inc,
                      recast_inc,
                      detour_inc,
                      detour_crowd_inc,
                      detour_tile_cache_inc,
                      detour_debug_inc,
                      sqlite_inc]

#cmake = import('cmake')
#fmt_sub_proj = cmake.subproject('fmt')
#fmt_dep = fmt_sub_proj.dependency('fmt')

thread_dep = dependency('threads')
sdl2_dep = dependency('sdl2', version : '>=2.0.9')
vulkan_dep = dependency('vulkan', method : 'system')
fmt_dep = dependency('fmt')
physfs_dep = dependency('physfs', version: '>=3.0.2')

shaderc_dep = dependency('shaderc', static : true, required : false)
if not shaderc_dep.found()
    # Create a dependency from user set options
    # TODO add more checks and explicit error messages
    shaderc_dir = get_option('shaderc_lib_dir')
    
    combined_lib = cpp_comp.find_library('shaderc_combined', dirs : [shaderc_dir])
    shaderc_inc_dir = include_directories(get_option('shaderc_include_dir'))
    shaderc_dep = declare_dependency(
        include_directories : shaderc_inc_dir,
        dependencies : combined_lib,
    )
endif

subdir('src')

if get_option('iyf_build_tools')
    assimp_dep = dependency('assimp')
    # Create a dependency from user set options
    # TODO add more checks and explicit error messages
    compr_dir = 'dependencies/compressonator'
    
    compr_lib = cpp_comp.find_library('compressonator',
        required: true,
        static: true,
        dirs : [meson.current_source_dir() + '/' + compr_dir])
    compr_inc_dir = include_directories(compr_dir)
    compressonator_dep = declare_dependency(
        include_directories : compr_inc_dir,
        dependencies : compr_lib,
    )
    
    tiny_file_dialogs_inc = include_directories('dependencies/tinyfiledialogs')
    subdir('tools')
    subdir('testing')
    
    custom_target('system_assets',
                  command: [asset_packer, '--output', IYFEditor],
                  depends: [IYFEditor],
                  output: ['system.iyfpak'])
endif
