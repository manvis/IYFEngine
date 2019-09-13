iyf_tests_src = [
    'main.cpp',
    'BehaviourTreeTests.cpp',
    'ChunkedVectorTests.cpp',
    'ConfigurationTests.cpp',
    'FileMonitorTests.cpp',
    'MemorySerializerTests.cpp',
    'CSVParserTests.cpp',
    'MetadataSerializationTests.cpp',
]
executable('IYFTest', iyf_tests_src,
    include_directories : [common_project_inc, iyf_tool_inc],
    link_with : [IYFTools_lib]
)
