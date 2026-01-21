#include "audioplayer.h"

#include <SDL3/SDL.h>

#include "libcutils/logger.h"



struct Audioplayer {
	SDL_AudioStream *stream;
};

static struct Audioplayer g_audioplayer = {0};
/**
 * maybe this? https://github.com/icculus/SDL_sound/blob/main/examples/playsound_simple.c
 * => not available on raspbian 
 * => use mpg123 for conversion of mp3
 */

//static Mix_Music *g_current_file = NULL;
static void *g_current_file = NULL;
static bool g_is_playing = false;
// TODO: very very very likely memory leaks all over the place

static void callback_fill_stream_with_data(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
	(void) userdata;
	(void) stream;
	(void) additional_amount;
	(void) total_amount;
}

void audioplayer_stop(void)
{
	//Mix_HaltMusic();

	if (g_current_file != NULL) {
		//Mix_FreeMusic(g_current_file);
		g_current_file = NULL;
	}
}

Result audioplayer_play_file(const char *filepath, struct Audio_File_Metadata *metadata)
{
#if 0
/* We're just playing a single thing here, so we'll use the simplified option.
       We are always going to feed audio in as mono, float32 data at 8000Hz.
       The stream will convert it to whatever the hardware wants on the other side. */
    spec.channels = 1;
    spec.format = SDL_AUDIO_F32;
    spec.freq = 8000;
    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!stream) {
        SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    /* SDL_OpenAudioDeviceStream starts the device paused. You have to tell it to start! */
    SDL_ResumeAudioStreamDevice(stream);
#endif 


	

	audioplayer_stop();
	//g_current_file = Mix_LoadMUS(filepath);

	if (g_current_file == NULL) {
		return result_make( false, "Failed to open audiofile");
	}

	//Mix_PlayMusic(g_current_file, 0);
	if (metadata != NULL) {
		//metadata->length_secs = Mix_MusicDuration(g_current_file);

		//const char *title = Mix_GetMusicTitleTag(g_current_file);
		//const char *artist = Mix_GetMusicArtistTag(g_current_file);
		const char *title = "no-title";
		const char *artist = "no-artist";
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
		//return (int)Mix_GetMusicPosition(g_current_file);
		return 0;
	}
	else {
		return -1;
	}
}

bool audioplayer_is_playing(void)
{
	return g_is_playing;
	//return (g_is_playing && Mix_PlayingMusic());
}

void audioplayer_pause(void) {
	//Mix_PauseMusic();
	g_is_playing = false;
}

void audioplayer_resume(void) {
	if (g_audioplayer.stream != NULL) {
		SDL_ResumeAudioStreamDevice(g_audioplayer.stream);
		g_is_playing = true;
	}
}

void audioplayer_set_pos(double pos_secs)
{
	(void) pos_secs;
	//Mix_SetMusicPosition(pos_secs);
}
