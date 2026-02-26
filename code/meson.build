sdl_dependency     = dependency('sdl3'      , include_type: 'system')
sdl_ttf_dependency = dependency('sdl3-ttf'  , include_type: 'system')
sdl_img_dependency = dependency('sdl3-image', include_type: 'system')
libcurl            = dependency('libcurl'   , include_type: 'system')
libmpg123          = dependency('libmpg123' , include_type: 'system')

shard_os_sources = [
  'audio.c',
  'config.c',
  'main.c',
  'screen.c',
  'filebrowser.c',
  'ui_audio_settings.c',
  'ui_elements.c',
  'ui_main.c',
  'app_jukebox.c',
  'app_radio.c',
  'app_dice.c'
]

executable('shard-os',
  shard_os_sources,
  include_directories: ['../thirdparty/libcutils/include'],
  dependencies:        [sdl_dependency, sdl_ttf_dependency, sdl_img_dependency, libcurl, libmpg123]
)
