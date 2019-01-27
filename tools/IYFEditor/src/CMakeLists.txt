include_directories(${IYFEngine_SOURCE_DIR}/include)
include_directories(${IYFEngine_SOURCE_DIR}/tools/IYFEditor/include)

set(COMPRESSONATOR_LIB "" CACHE FILEPATH "A path to the AMD Compressonator SDK library")
set(COMPRESSONATOR_INCLUDE_PATH "" CACHE PATH "A path to the AMD Compressonator SDK includes (the path should contain compressonator.h)")

if (COMPRESSONATOR_LIB STREQUAL "") 
    message(FATAL_ERROR "COMPRESSONATOR_LIB wasn't specified. CMake cannot continue.")
endif()

if (COMPRESSONATOR_INCLUDE_PATH STREQUAL "") 
    message(FATAL_ERROR "COMPRESSONATOR_INCLUDE_PATH wasn't specified. CMake cannot continue.")
endif()

include_directories(../../../dependencies/tinyfiledialogs)
include_directories(${COMPRESSONATOR_INCLUDE_PATH})

add_library(IYFTools STATIC 
    assetImport/ConverterManager.cpp
    MeshImporterState.cpp
    EditorUI.cpp
    states/EditorState.cpp
    states/EditorWelcomeState.cpp
    assetImport/converterStates/FontConverterState.cpp
    assetImport/converterStates/LocalizationStringConverterState.cpp
    assetImport/converterStates/MaterialInstanceConverterState.cpp
    assetImport/converterStates/MaterialTemplateConverterState.cpp
    assetImport/converterStates/MeshConverterState.cpp
    assetImport/converterStates/ShaderConverterState.cpp
    assetImport/converterStates/TextureConverterState.cpp
    assetImport/ConverterState.cpp
    assetImport/Converter.cpp
    assetImport/converters/LocalizationStringConverter.cpp
    assetImport/converters/MaterialTemplateConverter.cpp
    assetImport/converters/MaterialInstanceConverter.cpp
    assetImport/converters/MeshConverter.cpp
    assetImport/converters/ShaderConverter.cpp
    assetImport/converters/TextureConverter.cpp
    assetImport/converters/FontConverter.cpp
    tools/AssetCreatorWindow.cpp
    tools/AssetUpdateManager.cpp
    tools/Compressonator_Test_Helpers.cpp
    tools/MaterialEditor.cpp
    tools/MaterialFamilyEditor.cpp
    tools/MaterialInstanceEditor.cpp
    ../../../testing/TestState.cpp
    ../../../dependencies/tinyfiledialogs/tinyfiledialogs.c)

target_link_libraries(IYFTools IYFEngine ${ASSIMP_LIBRARIES} ${COMPRESSONATOR_LIB})

add_executable(IYFEditor main.cpp)

add_executable(SystemAssetPacker 
    ../../SystemAssetPacker/main.cpp
    ../../SystemAssetPacker/SystemAssetPacker.cpp)

target_link_libraries(IYFEditor IYFTools)
target_link_libraries(SystemAssetPacker IYFTools)

# TESTS ----------------------------------------------------------------------------------------------------------
add_executable(IYFTest 
    ../../../testing/main.cpp
    ../../../testing/BehaviourTreeTests.cpp
    ../../../testing/FileMonitorTests.cpp
    ../../../testing/MemorySerializerTests.cpp
    ../../../testing/CSVParserTests.cpp
    ../../../testing/MetadataSerializationTests.cpp)

target_link_libraries(IYFTest IYFTools)

# if(WIN32)
# #TODO windows symlink?
# add_custom_command(TARGET IYFTest PRE_BUILD
#                    COMMAND ${CMAKE_COMMAND} -E copy_directory
#                    ${IYFEngine_SOURCE_DIR}/data $<TARGET_FILE_DIR:IYFTest>/data)
# else(WIN32)
# add_custom_command(TARGET IYFTest PRE_BUILD
#                    COMMAND ${CMAKE_COMMAND} -E create_symlink
#                    ${IYFEngine_SOURCE_DIR}/data $<TARGET_FILE_DIR:IYFTest>/data)
# endif(WIN32)
# #add_custom_command(TARGET IYFEngine PRE_BUILD
# #                   COMMAND ${CMAKE_COMMAND} -E copy_directory
# #                   ${IYFEngine_SOURCE_DIR}/data $<TARGET_FILE_DIR:IYFEngine>/data)

