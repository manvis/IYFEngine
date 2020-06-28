gnome = import('gnome')

iyf_launcher_res = gnome.compile_resources(
    'iyflauncher-resources', 'data/iyflauncher.gresource.xml',
    source_dir: 'data'
)

launcher_inc = include_directories('include')

launcher_thread_dep = dependency('threads')
gtkmm_dep = dependency('gtkmm-3.0', version : '>=3.24.0')

launcher_sources = [
    'main.cpp',
    'src/LauncherApp.cpp',
    'src/LauncherAppWindow.cpp',
    'src/ProgressDialog.cpp',
    iyf_launcher_res
]

executable('IYFLauncher', launcher_sources,
    include_directories : [launcher_inc, rapidjson_inc],
    dependencies: [gtkmm_dep, launcher_thread_dep],
    #cpp_args: '-DGTKMM_DISABLE_DEPRECATED'
)
