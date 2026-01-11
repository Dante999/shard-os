#include "audioplayer.h"

#include <SDL.h>
#include <SDL_mixer.h>

#include "libcutils/logger.h"

static Mix_Music *g_current_file = NULL;
static bool g_is_playing = false;
// TODO: very very very likely memory leaks all over the place

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
