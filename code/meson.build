sdl_dependency     = dependency('sdl2'      , include_type: 'system')
sdl_ttf_dependency = dependency('SDL2_ttf'  , include_type: 'system')
sdl_gfx            = dependency('SDL2_gfx'  , include_type: 'system')
sdl_image          = dependency('SDL2_image', include_type: 'system')
shard_os_sources = [
  'main.c',
  'config.c',
  'screen.c',
  'ui_main.c',
  'ui_elements.c',
  'app_radio.c'
]

executable('shard-os',
  shard_os_sources,
  include_directories: ['../thirdparty/libcutils/include'],
  dependencies:        [sdl_dependency, sdl_ttf_dependency, sdl_gfx, sdl_image]
)
