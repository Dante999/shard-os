sdl_dependency     = dependency('sdl2'      , include_type: 'system')
sdl_ttf_dependency = dependency('SDL2_ttf'  , include_type: 'system')
sdl_gfx            = dependency('SDL2_gfx'  , include_type: 'system')
sdl_image          = dependency('SDL2_image', include_type: 'system')
sdl_mixer          = dependency('SDL2_mixer', include_type: 'system')
libcurl            = dependency('libcurl', include_type: 'system')

shard_os_sources = [
  'audio.c',
  'config.c',
  'main.c',
  'screen.c',
  'filebrowser.c',
  'audioplayer.c',
  'audiostreamer.c',
  'ui_elements.c',
  'ui_main.c',
  'app_jukebox.c',
  'app_radio.c',
  'app_dice.c'
]

executable('shard-os',
  shard_os_sources,
  include_directories: ['../thirdparty/libcutils/include'],
  dependencies:        [sdl_dependency, sdl_ttf_dependency, sdl_gfx, sdl_image, sdl_mixer, libcurl]
)
