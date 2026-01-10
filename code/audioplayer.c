#include "audioplayer.h"

#include <SDL.h>
#include <SDL_mixer.h>

#include "config.h"
#include "libcutils/logger.h"

static Mix_Music *g_current_file = NULL;
static bool g_is_playing = false;
// TODO: very very very likely memory leaks all over the place

Result audioplayer_init(void)
{
	const int count = SDL_GetNumAudioDevices(0);
	for (int i = 0; i < count; ++i) {
		log_debug("Found Audio Device: [%d]: %s\n", i, SDL_GetAudioDeviceName(i, 0));
	}

	if (Mix_OpenAudioDevice(44100, MIX_DEFAULT_FORMAT, 2, 1024, g_config.audio_device_name, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE) != 0) {
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
void audioplayer_stop(void)
{
	Mix_HaltMusic();

	if (g_current_file != NULL) {
		Mix_FreeMusic(g_current_file);
		g_current_file = NULL;
	}
}

Result audioplayer_play_file(const char *filepath, struct Audio_File_Metadata *metadata)
{
	audioplayer_stop();
	g_current_file = Mix_LoadMUS(filepath);

	if (g_current_file == NULL) {
		return result_make( false, "Failed to open audiofile");
	}

	Mix_PlayMusic(g_current_file, 0);
	if (metadata != NULL) {
		metadata->length_secs = Mix_MusicDuration(g_current_file);

		const char *title = Mix_GetMusicTitleTag(g_current_file);
		const char *artist = Mix_GetMusicArtistTag(g_current_file);

		if (strlen(title) > 0 || strlen(artist) > 0) {
			strncpy(metadata->title, title, sizeof(metadata->title));
			strncpy(metadata->artist, artist, sizeof(metadata->artist));
		}
		else {
			const char *filename = strrchr(filepath, '/');
			if (filename != NULL) {
				log_debug("%s contains not tags, using filename as metadata: '%s'\n", filepath, filename);
				strncpy(metadata->title, filename+1, sizeof(metadata->title));
			}
			else {
				strncpy(metadata->title, filepath, sizeof(metadata->title));
			}
		}
	}

	g_is_playing = true;
	return result_make_success();
}

int audioplayer_get_current_pos_in_secs(void)
{
	if (g_current_file != NULL) {
		return (int)Mix_GetMusicPosition(g_current_file);
	}
	else {
		return -1;
	}
}

bool audioplayer_is_playing(void)
{
	return (g_is_playing && Mix_PlayingMusic());
}

void audioplayer_pause(void) {
	Mix_PauseMusic();
	g_is_playing = false;
}

void audioplayer_resume(void) {
	Mix_ResumeMusic();
	g_is_playing = true;
}

void audioplayer_set_pos(double pos_secs)
{
	Mix_SetMusicPosition(pos_secs);
}
