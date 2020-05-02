iyf_core_src = [
    'ai/BehaviourTree.cpp',
    'ai/Blackboard.cpp',
    #------- assets directory
    'assets/AssetConstants.cpp',
    'assets/AssetManager.cpp',
    #------- loader directory
    'assets/loaders/MeshLoader.cpp',
    'assets/loaders/TextureLoader.cpp',
    #------- metadata directory
    'assets/metadata/AnimationMetadata.cpp',
    'assets/metadata/AudioMetadata.cpp',
    'assets/metadata/FontMetadata.cpp',
    'assets/metadata/MaterialInstanceMetadata.cpp',
    'assets/metadata/MaterialTemplateMetadata.cpp',
    'assets/metadata/MeshMetadata.cpp',
    'assets/metadata/Metadata.cpp',
    'assets/metadata/MetadataBase.cpp',
    'assets/metadata/CustomMetadata.cpp',
    'assets/metadata/TextureMetadata.cpp',
    'assets/metadata/VideoMetadata.cpp',
    'assets/metadata/ScriptMetadata.cpp',
    'assets/metadata/ShaderMetadata.cpp',
    'assets/metadata/StringMetadata.cpp',
    #------- typeManager directory
    'assets/typeManagers/FontTypeManager.cpp',
    'assets/typeManagers/MeshTypeManager.cpp',
    'assets/typeManagers/ShaderTypeManager.cpp',
    'assets/typeManagers/TextureTypeManager.cpp',
    'assets/typeManagers/TypeManager.cpp',
    #------- core directory
    'core/configuration/Configuration.cpp',
    'core/interfaces/Configurable.cpp',
    'core/interfaces/ComponentContainer.cpp',
    'core/interfaces/TextSerializable.cpp',
    'core/serialization/MemorySerializer.cpp',
    'core/serialization/VirtualFileSystemSerializer.cpp',
    'core/ComponentType.cpp',
    'core/Constants.cpp',
    'core/DefaultWorld.cpp',
    'core/Engine.cpp',
    'core/EntitySystemManager.cpp',
    'core/GameState.cpp',
    'core/InputListener.cpp',
    'core/InputState.cpp',
    'core/Logger.cpp',
    'core/Platform.cpp',
    'core/ProductID.cpp',
    'core/Project.cpp',
    'core/TransformationComponent.cpp',
    'core/World.cpp',
    #------- filesystem directory
    'core/filesystem/physfs/physfsrwops.c',
    'core/filesystem/File.cpp',
    'core/filesystem/FileSystem.cpp',
    'core/filesystem/FileSystemWatcher.cpp',
    'core/filesystem/linux/InotifyFileSystemWatcher.cpp',
    #------- graphics directory
    'graphics/Camera.cpp',
    'graphics/CubemapSkybox.cpp',
    'graphics/DebugRenderer.cpp',
    'graphics/GraphicsAPI.cpp',
    'graphics/GraphicsAPIConstants.cpp',
    'graphics/GraphicsSystem.cpp',
    'graphics/LightComponent.cpp',
    'graphics/MeshComponent.cpp',
    'graphics/Renderer.cpp',
    'graphics/RendererProperties.cpp',
    'graphics/ShaderConstants.cpp',
    'graphics/ShaderMacros.cpp',
    'graphics/Skybox.cpp',
    'graphics/VertexDataLayouts.cpp',
    #--------------------- clustered rendering renderer
    'graphics/clusteredRenderers/ClusteredRenderer.cpp',
    'graphics/clusteredRenderers/ClusteredRendererProperties.cpp',
    #--------------------- culling
    'graphics/culling/Frustum.cpp',
    'graphics/culling/BoundingVolumes.cpp',
    #--------------------- OpenGL graphics backend
    #'graphics/gl/OpenGLAPI.cpp',
    #--------------------- imgui
    'graphics/imgui/ImGuiImplementation.cpp',
    'graphics/imgui/imgui.cpp',
    'graphics/imgui/imgui_demo.cpp',
    'graphics/imgui/imgui_draw.cpp',
    'graphics/imgui/imgui_widgets.cpp',
    'graphics/imgui/imguiCustom.cpp',
    #--------------------- materials
    'graphics/materials/MaterialDatabase.cpp',
    'graphics/materials/MaterialFamilyDefinition.cpp',
    'graphics/materials/MaterialInstanceDefinition.cpp',
    'graphics/materials/MaterialLogicGraph.cpp',
    #--------------------- shader generation
    'graphics/shaderGeneration/ShaderGenerator.cpp',
    'graphics/shaderGeneration/ShaderMacroCombiner.cpp',
    'graphics/shaderGeneration/VulkanGLSLShaderGenerator.cpp',
    #--------------------- Vulkan graphics backend
    'graphics/vulkan/VulkanAPI.cpp',
    'graphics/vulkan/VulkanDebug.cpp',
    'graphics/vulkan/VulkanDeviceMemoryManager.cpp',
    'graphics/vulkan/VulkanInitializationAndSwapchain.cpp',
    'graphics/vulkan/VulkanMemoryAllocatorImpl.cpp',
    #------- localization directory
    'localization/LocalizationCSVParser.cpp',
    'localization/TextLocalization.cpp',
    #--------------------- miniz library
    'miniz/miniz.c',
    #------- physics directory
    'physics/PhysicsSystem.cpp',
    'physics/RigidBody.cpp',
    #------- sound directory
    'sound/SoundAPI.cpp',
    #------- threading directory
    'threading/Implementation.cpp',
    #------- utilities folder
    'utilities/BufferRangeSet.cpp',
    'utilities/Compression.cpp',
    'utilities/ImGuiUtils.cpp',
    'utilities/stbImpl.cpp',
    'utilities/StringUtilities.cpp',
    'utilities/ReadWholeFile.cpp',
    'utilities/Regexes.cpp',
    #------- separately stored third party dependencies
    #--------------------- sqlite
    '../dependencies/sqlite/sqlite3.c',
    #--------------------- recast
    '../dependencies/recastnavigation/Recast/Source/Recast.cpp',
    '../dependencies/recastnavigation/Recast/Source/RecastAlloc.cpp',
    '../dependencies/recastnavigation/Recast/Source/RecastArea.cpp',
    '../dependencies/recastnavigation/Recast/Source/RecastContour.cpp',
    '../dependencies/recastnavigation/Recast/Source/RecastFilter.cpp',
    '../dependencies/recastnavigation/Recast/Source/RecastLayers.cpp',
    '../dependencies/recastnavigation/Recast/Source/RecastMesh.cpp',
    '../dependencies/recastnavigation/Recast/Source/RecastMeshDetail.cpp',
    '../dependencies/recastnavigation/Recast/Source/RecastRasterization.cpp',
    '../dependencies/recastnavigation/Recast/Source/RecastRegion.cpp',
    #--------------------- detour
    '../dependencies/recastnavigation/Detour/Source/DetourAlloc.cpp',
    '../dependencies/recastnavigation/Detour/Source/DetourCommon.cpp',
    '../dependencies/recastnavigation/Detour/Source/DetourNavMesh.cpp',
    '../dependencies/recastnavigation/Detour/Source/DetourNavMeshBuilder.cpp',
    '../dependencies/recastnavigation/Detour/Source/DetourNavMeshQuery.cpp',
    '../dependencies/recastnavigation/Detour/Source/DetourNode.cpp',
    #--------------------- detour crowds
    '../dependencies/recastnavigation/DetourCrowd/Source/DetourCrowd.cpp',
    '../dependencies/recastnavigation/DetourCrowd/Source/DetourLocalBoundary.cpp',
    '../dependencies/recastnavigation/DetourCrowd/Source/DetourObstacleAvoidance.cpp',
    '../dependencies/recastnavigation/DetourCrowd/Source/DetourPathCorridor.cpp',
    '../dependencies/recastnavigation/DetourCrowd/Source/DetourPathQueue.cpp',
    '../dependencies/recastnavigation/DetourCrowd/Source/DetourProximityGrid.cpp',
    #--------------------- detour tile caching
    '../dependencies/recastnavigation/DetourTileCache/Source/DetourTileCache.cpp',
    '../dependencies/recastnavigation/DetourTileCache/Source/DetourTileCacheBuilder.cpp',
    #--------------------- recast detour debug utils
    '../dependencies/recastnavigation/DebugUtils/Source/DebugDraw.cpp',
    '../dependencies/recastnavigation/DebugUtils/Source/DetourDebugDraw.cpp',
    '../dependencies/recastnavigation/DebugUtils/Source/RecastDebugDraw.cpp',
    '../dependencies/recastnavigation/DebugUtils/Source/RecastDump.cpp'
]

if get_option('physics_engine') == 'bullet'
    physics_src = [
        'physics/bullet/BulletPhysicsDebugRenderer.cpp',
        'physics/bullet/BulletPhysicsSystem.cpp'
    ]
endif

IYFEngine_lib = static_library('IYFEngine', [iyf_core_src, physics_src],
    include_directories : [common_project_inc],
    dependencies : [physics_dep,
                    sdl2_dep,
                    shaderc_dep,
                    thread_dep,
                    vulkan_dep,
                    fmt_dep,
                    physfs_dep]
)
