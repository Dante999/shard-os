#include "audio.h"

#include <SDL3/SDL.h>

#include "libcutils/logger.h"

#include "config.h"

SDL_AudioDeviceID g_audio_device = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;

Result audio_open(void)
{
	g_audio_device = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;

	int num_devices = -1;
	SDL_AudioDeviceID *devices = SDL_GetAudioPlaybackDevices(&num_devices);
	if (devices) {
		for (int i = 0; i < num_devices; ++i) {
			SDL_AudioDeviceID instance_id = devices[i];
			const char        *name        = SDL_GetAudioDeviceName(instance_id);
			log_debug("Found Audio Device: [%d] %s\n", instance_id, name);

			if (strncmp(name, g_config.audio_device_name, sizeof(g_config.audio_device_name)) == 0) {
				g_audio_device = instance_id;
			}
		}
		SDL_free(devices);
	}
#if 0
	if (Mix_OpenAudioDevice(44100, MIX_DEFAULT_FORMAT, 2, 1024, g_config.audio_device_name, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE) == 0) {
		log_debug("successfully opened audio device from config: %s\n", g_config.audio_device_name);
		return result_make_success();
	}

	log_warning("unable to open audio device from config: '%s'\n", g_config.audio_device_name);
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		return result_make(
			false,
			"SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
	}
#endif
	return result_make_success();
}

void audio_close(void)
{
	SDL_CloseAudioDevice(g_audio_device);
}
