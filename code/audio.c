#include "audio.h"

#include <SDL.h>
#include <SDL_mixer.h>

#include "libcutils/logger.h"

#include "config.h"

Result audio_open(void)
{
	const int count = SDL_GetNumAudioDevices(0);
	for (int i = 0; i < count; ++i) {
		log_debug("Found Audio Device: [%d] %s\n", i, SDL_GetAudioDeviceName(i, 0));
	}

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

	return result_make_success();
}

void audio_close(void)
{
	Mix_CloseAudio();
}
