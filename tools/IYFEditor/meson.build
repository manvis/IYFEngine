iyf_tool_inc = include_directories('include')
iyf_test_inc = include_directories('../../testing')

iyf_tool_src = [
    'src/assetImport/ConverterManager.cpp',
    'src/MeshImporterState.cpp',
    'src/EditorUI.cpp',
    'src/states/EditorState.cpp',
    'src/states/EditorWelcomeState.cpp',
    'src/assetImport/converterStates/FontConverterState.cpp',
    'src/assetImport/converterStates/LocalizationStringConverterState.cpp',
    'src/assetImport/converterStates/MaterialInstanceConverterState.cpp',
    'src/assetImport/converterStates/MaterialTemplateConverterState.cpp',
    'src/assetImport/converterStates/MeshConverterState.cpp',
    'src/assetImport/converterStates/ShaderConverterState.cpp',
    'src/assetImport/converterStates/TextureConverterState.cpp',
    'src/assetImport/ConverterState.cpp',
    'src/assetImport/Converter.cpp',
    'src/assetImport/converters/LocalizationStringConverter.cpp',
    'src/assetImport/converters/MaterialTemplateConverter.cpp',
    'src/assetImport/converters/MaterialInstanceConverter.cpp',
    'src/assetImport/converters/MeshConverter.cpp',
    'src/assetImport/converters/ShaderConverter.cpp',
    'src/assetImport/converters/TextureConverter.cpp',
    'src/assetImport/converters/FontConverter.cpp',
    'src/tools/AssetCreatorWindow.cpp',
    'src/tools/AssetUpdateManager.cpp',
    'src/tools/MaterialEditor.cpp',
    'src/tools/MaterialFamilyEditor.cpp',
    'src/tools/MaterialInstanceEditor.cpp',
    '../../testing/TestState.cpp',
    '../../dependencies/tinyfiledialogs/tinyfiledialogs.c'
]

IYFTools_lib = static_library('IYFTools', iyf_tool_src, 
    include_directories : [iyf_test_inc, common_project_inc, iyf_tool_inc, tiny_file_dialogs_inc],
    dependencies : [shaderc_dep, compressonator_dep, assimp_dep],
    link_with : [IYFEngine_lib]
)

IYFEditor = executable('IYFEditor', 'src/main.cpp',
    include_directories : [common_project_inc, iyf_tool_inc],
    link_with : [IYFTools_lib]
)
