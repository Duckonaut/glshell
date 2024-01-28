project('glshell', 'c',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'b_lto=true',
  ]
)

cc = meson.get_compiler('c')

add_global_arguments('-DPROJECT_NAME="glshell"', language : 'c')
add_global_arguments('-DPROJECT_VERSION="0.1.0"', language : 'c')

src = [
  'src/args.c',
  'src/glshell.c',
  'src/main.c',
]

wayland_client = dependency('wayland-client')
wayland_egl = dependency('wayland-egl')
wayland_protocols = dependency('wayland-protocols')
subdir('protocols')

deps = [
  wayland_client,
  wayland_egl,
  wayland_protocols,
  client_protos,
  dependency('glew'),
  cc.find_library('m', required : false),
  cc.find_library('EGL', required : true),
  cc.find_library('GL', required : true),
]

inc = include_directories('include')
stb = include_directories('stb', is_system : true)

exe = executable('glshell', src,
  include_directories : [
    inc,
    stb
  ],
  dependencies : deps,
  install : true)
