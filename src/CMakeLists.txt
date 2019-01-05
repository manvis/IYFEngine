include_directories(${IYFEngine_SOURCE_DIR}/include)
include_directories(${IYFEngine_SOURCE_DIR}/testing)

#TODO implement this
option(USE_OWN_TYPE_IDENTIFIER "NOT YET IMPLEMENTED. Use a custom type identifier if checked. Else - C++11's typeid" OFF)
if (USE_OWN_TYPE_IDENTIFIER)
    MESSAGE(STATUS "Using own type identifier IS NOT YET IMPLEMENTED.")
    add_definitions(-DUSE_OWN_TYPE_ID)
endif()

option(ALLOW_NON_OPTIMAL_TEXTURE_FORMATS "Use stb_image library to enable texture creation from JPEG, PNG, and other formats. Otherwise only GPU optimal DDS and KTX formats are loaded via gli." ON)

if (ALLOW_NON_OPTIMAL_TEXTURE_FORMATS)
    MESSAGE(STATUS "Enabling stb_image use")
    add_definitions(-DALLOW_NON_OPTIMAL_TEXTURE_FORMATS)
endif()

# TODO use system asset packer
# if (GRAPHICS_BACKEND MATCHES "Vulkan")
#     # Shader compilation
#     set(sDir "${PROJECT_SOURCE_DIR}/data/shaders/")
#     set(shaders defaultVertex.vert forwardRendering.frag imgui.vert imgui.frag physicsDebug.vert physicsDebug.frag skyBox.vert skyBox.frag randomTests.frag)
#     set(compiledShaders)
#     
#     foreach(shader ${shaders})
#         #MESSAGE(STATUS "${sDir}${shader}.spv")
#         add_custom_command(
#             OUTPUT ${sDir}/spv/${shader}.spv
#             COMMAND glslangValidator -V ${sDir}${shader} -o ${sDir}/spv/${shader}.spv
#             DEPENDS ${sDir}${shader}
#         )
#         
#         set(compiledShaders ${compiledShaders} "${sDir}/spv/${shader}.spv")
#     endforeach()
#     
#     add_custom_target(compileShaders ALL DEPENDS ${compiledShaders})
# endif()

set (BUILD_FILES
ai/BehaviourTree.cpp
ai/Blackboard.cpp
#------- assets directory
assets/AssetConstants.cpp
assets/AssetManager.cpp
#------- loader directory
assets/loaders/MeshLoader.cpp
assets/loaders/TextureLoader.cpp
#------- metadata directory
assets/metadata/AnimationMetadata.cpp
assets/metadata/AudioMetadata.cpp
assets/metadata/FontMetadata.cpp
assets/metadata/MaterialTemplateMetadata.cpp
assets/metadata/MeshMetadata.cpp
assets/metadata/MetadataBase.cpp
assets/metadata/CustomMetadata.cpp
assets/metadata/TextureMetadata.cpp
assets/metadata/VideoMetadata.cpp
assets/metadata/ScriptMetadata.cpp
assets/metadata/ShaderMetadata.cpp
assets/metadata/StringMetadata.cpp
#------- typeManager directory
assets/typeManagers/FontTypeManager.cpp
assets/typeManagers/MeshTypeManager.cpp
assets/typeManagers/ShaderTypeManager.cpp
assets/typeManagers/TextureTypeManager.cpp
assets/typeManagers/TypeManager.cpp
#------- core directory
core/configuration/Configuration.cpp
core/interfaces/Configurable.cpp
core/interfaces/ComponentContainer.cpp
core/interfaces/TextSerializable.cpp
core/serialization/MemorySerializer.cpp
core/serialization/VirtualFileSystemSerializer.cpp
core/ComponentType.cpp
core/Constants.cpp
core/DefaultWorld.cpp
core/Engine.cpp
core/EntitySystemManager.cpp
core/GameState.cpp
core/InputListener.cpp
core/InputState.cpp
core/Logger.cpp
core/Platform.cpp
core/ProductID.cpp
core/Project.cpp
core/TransformationComponent.cpp
core/World.cpp
#------- filesystem directory
core/filesystem/physfs/physfsrwops.c
core/filesystem/File.cpp
core/filesystem/FileSystem.cpp
core/filesystem/FileSystemWatcher.cpp
core/filesystem/linux/InotifyFileSystemWatcher.cpp
#------- format directory
format/format.cc
#------- graphics directory
graphics/Camera.cpp
graphics/CubemapSkybox.cpp
graphics/DebugRenderer.cpp
graphics/GraphicsAPI.cpp
graphics/GraphicsAPIConstants.cpp
graphics/GraphicsSystem.cpp
graphics/Renderer.cpp
graphics/RendererProperties.cpp
graphics/ShaderConstants.cpp
graphics/ShaderMacros.cpp
graphics/Skybox.cpp
graphics/VertexDataLayouts.cpp
#--------------------- clustered rendering renderer
graphics/clusteredRenderers/ClusteredRenderer.cpp
graphics/clusteredRenderers/ClusteredRendererProperties.cpp
#--------------------- culling
graphics/culling/Frustum.cpp
graphics/culling/BoundingVolumes.cpp
#--------------------- OpenGL graphics backend
graphics/gl/OpenGLAPI.cpp
#--------------------- imgui
graphics/imgui/ImGuiImplementation.cpp
graphics/imgui/imgui.cpp
graphics/imgui/imgui_demo.cpp
graphics/imgui/imgui_draw.cpp
graphics/imgui/imguiCustom.cpp
#--------------------- materials
graphics/materials/MaterialDatabase.cpp
graphics/materials/MaterialFamilyDefinition.cpp
graphics/materials/MaterialInstanceDefinition.cpp
graphics/materials/MaterialLogicGraph.cpp
#--------------------- shader generation
graphics/shaderGeneration/ShaderGenerator.cpp
graphics/shaderGeneration/VulkanGLSLShaderGenerator.cpp
#--------------------- Vulkan graphics backend
graphics/vulkan/VulkanAPI.cpp
graphics/vulkan/VulkanDebug.cpp
graphics/vulkan/VulkanDeviceMemoryManager.cpp
graphics/vulkan/VulkanInitializationAndSwapchain.cpp
graphics/vulkan/VulkanMemoryAllocatorImpl.cpp
#------- localization directory
localization/LocalizationCSVParser.cpp
localization/TextLocalization.cpp
#--------------------- miniz library
miniz/miniz.c
#------- physics directory
physics/BulletPhysicsDebugRenderer.cpp
physics/Collider.cpp
physics/PhysicsSystem.cpp
#------- sound directory
sound/SoundAPI.cpp
#------- threading directory
threading/Implementation.cpp
#------- utilities folder
utilities/BufferRangeSet.cpp
utilities/Compression.cpp
utilities/ImGuiUtils.cpp
utilities/stbImpl.cpp
utilities/ReadWholeFile.cpp
utilities/Regexes.cpp
#------- separately stored third party dependencies
#--------------------- sqlite
../dependencies/sqlite/sqlite3.c
#--------------------- recast
../dependencies/recastnavigation/Recast/Source/Recast.cpp
../dependencies/recastnavigation/Recast/Source/RecastAlloc.cpp
../dependencies/recastnavigation/Recast/Source/RecastArea.cpp
../dependencies/recastnavigation/Recast/Source/RecastContour.cpp
../dependencies/recastnavigation/Recast/Source/RecastFilter.cpp
../dependencies/recastnavigation/Recast/Source/RecastLayers.cpp
../dependencies/recastnavigation/Recast/Source/RecastMesh.cpp
../dependencies/recastnavigation/Recast/Source/RecastMeshDetail.cpp
../dependencies/recastnavigation/Recast/Source/RecastRasterization.cpp
../dependencies/recastnavigation/Recast/Source/RecastRegion.cpp
#--------------------- detour
../dependencies/recastnavigation/Detour/Source/DetourAlloc.cpp
../dependencies/recastnavigation/Detour/Source/DetourCommon.cpp
../dependencies/recastnavigation/Detour/Source/DetourNavMesh.cpp
../dependencies/recastnavigation/Detour/Source/DetourNavMeshBuilder.cpp
../dependencies/recastnavigation/Detour/Source/DetourNavMeshQuery.cpp
../dependencies/recastnavigation/Detour/Source/DetourNode.cpp
#--------------------- detour crowds
../dependencies/recastnavigation/DetourCrowd/Source/DetourCrowd.cpp
../dependencies/recastnavigation/DetourCrowd/Source/DetourLocalBoundary.cpp
../dependencies/recastnavigation/DetourCrowd/Source/DetourObstacleAvoidance.cpp
../dependencies/recastnavigation/DetourCrowd/Source/DetourPathCorridor.cpp
../dependencies/recastnavigation/DetourCrowd/Source/DetourPathQueue.cpp
../dependencies/recastnavigation/DetourCrowd/Source/DetourProximityGrid.cpp
#--------------------- detour tile caching
../dependencies/recastnavigation/DetourTileCache/Source/DetourTileCache.cpp
../dependencies/recastnavigation/DetourTileCache/Source/DetourTileCacheBuilder.cpp
#--------------------- recast detour debug utils
../dependencies/recastnavigation/DebugUtils/Source/DebugDraw.cpp
../dependencies/recastnavigation/DebugUtils/Source/DetourDebugDraw.cpp
../dependencies/recastnavigation/DebugUtils/Source/RecastDebugDraw.cpp
../dependencies/recastnavigation/DebugUtils/Source/RecastDump.cpp)

if (GRAPHICS_BACKEND MATCHES "OpenGL")
    if (GLEW_DYNAMIC)
        set(GLEW_LIB_LOC "${GLEW_LIBRARY}")
    else (GLEW_DYNAMIC)
        set(GLEW_LIB_LOC "")
        set(BUILD_FILES "${BUILD_FILES};../dependencies/glew/src/glew.c")
    endif (GLEW_DYNAMIC)
endif()

add_library(IYFEngine STATIC ${BUILD_FILES})

if (USE_BOOST)
    set(FILE_SYSTEM_LIBS ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY})
else (USE_BOOST)
    if (WIN32)
        set(FILE_SYSTEM_LIBS)
    else (WIN32)
        set(FILE_SYSTEM_LIBS stdc++fs)
    endif (WIN32)
endif (USE_BOOST)

if(WIN32)
    add_definitions(-DIMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCS)
    
    #TODO GLEW_BUILD + GLEW_STATIC definitions, Microsoft kompiliatoriams
    
    if (GRAPHICS_BACKEND MATCHES "Vulkan")
        target_link_libraries(IYFEngine vulkan ${SDL2_LIBRARY} ${SDL2_MIX_LIBRARY} ${LUA_LIBRARIES} ${BULLET_LIBRARIES} ${PHYSFS_LIBRARY} ${FILE_SYSTEM_LIBS} ${CMAKE_DL_LIBS})
    elseif (GRAPHICS_BACKEND MATCHES "OpenGL")
        target_link_libraries(IYFEngine ${OPENGL_LIBRARIES} ${SDL2_LIBRARY} ${GLEW_LIB_LOC} ${SDL2_MIX_LIBRARY} ${LUA_LIBRARIES} ${BULLET_LIBRARIES} ${PHYSFS_LIBRARY} ${FILE_SYSTEM_LIBS} ${CMAKE_DL_LIBS})
    endif ()
else(WIN32)
    if (GRAPHICS_BACKEND MATCHES "Vulkan")
        target_link_libraries(IYFEngine ${X11_LIBRARIES} vulkan libshaderc_combined.a ${SDL2_LIBRARY} ${SDL2_MIX_LIBRARY} ${LUA_LIBRARIES} ${BULLET_LIBRARIES} ${PHYSFS_LIBRARY} ${FILE_SYSTEM_LIBS} ${CMAKE_DL_LIBS})
    else (GRAPHICS_BACKEND MATCHES "OpenGL")
        target_link_libraries(IYFEngine ${OPENGL_LIBRARIES} ${SDL2_LIBRARY} ${GLEW_LIB_LOC} ${SDL2_MIX_LIBRARY} ${LUA_LIBRARIES} ${BULLET_LIBRARIES} ${PHYSFS_LIBRARY} ${FILE_SYSTEM_LIBS} ${CMAKE_DL_LIBS})
    endif ()
endif(WIN32)
