iyf_common_src = [
    'src/check.cpp',
    'src/configuration/Configuration.cpp',
    'src/configuration/interfaces/Configurable.cpp',
    'src/io/DefaultFileSystem.cpp',
    'src/io/DefaultFileSystemFile.cpp',
    'src/io/File.cpp',
    'src/io/FileSystem.cpp',
    'src/io/Path.cpp',
    'src/io/serialization/FileSerializer.cpp',
    'src/io/serialization/MemorySerializer.cpp',
    'src/logging/Logger.cpp',
    'src/utilities/ReadWholeFile.cpp',
    'src/utilities/StringUtilities.cpp',
]

IYFCommon_lib_inc = include_directories('src')
IYFCommon_lib = static_library('IYFCommon', [iyf_common_src],
    include_directories : [IYFCommon_lib_inc]
)
