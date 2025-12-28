#include "audioplayer.h"

#include <SDL.h>
#include <SDL_mixer.h>

#include "libcutils/logger.h"

static Mix_Music *g_current_file = NULL;
static bool g_is_playing = false;
// TODO: very very very likely memory leaks all over the place

Result audioplayer_init(void)
{
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
		strncpy(metadata->title, title, sizeof(metadata->title));

		const char *artist = Mix_GetMusicArtistTag(g_current_file);
		strncpy(metadata->artist, artist, sizeof(metadata->artist));
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
