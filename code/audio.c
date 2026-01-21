#include "audio.h"

#include <SDL3/SDL.h>
#include <mpg123.h>

#include <assert.h>

#include "libcutils/logger.h"
#include "libcutils/util_makros.h"

#include "config.h"


static struct Shard_Audio {
	SDL_AudioDeviceID audio_device;
	SDL_AudioStream *stream;
	bool is_playing;
	struct Filestream {
		mpg123_handle *handle;
	} filestream;
} g_audio;

static void callback_fill_stream_with_data(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
	uint8_t buffer[5000];

	size_t bytes_decoded = 0;
	int merror = mpg123_read(g_audio.filestream.handle, buffer, MIN(sizeof(buffer),(size_t)additional_amount), &bytes_decoded);

	if (merror != MPG123_OK) {
		log_error("failed to decode audio file: %s\n", mpg123_strerror(g_audio.filestream.handle));
		return;
	}

	if (!SDL_PutAudioStreamData(g_audio.stream, buffer, (int)bytes_decoded)) {
		log_error("failed to put audio stream data: %s\n", SDL_GetError());
		return;
	}
	//log_debug("filling stream called: additional_amount=%d total_amount=%d\n", additional_amount, total_amount);
	(void) userdata;
	(void) stream;
	(void) additional_amount;
	(void) total_amount;
}

static Result audio_open(void)
{
	g_audio.audio_device = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;

	int num_devices = -1;
	SDL_AudioDeviceID *devices = SDL_GetAudioPlaybackDevices(&num_devices);
	if (devices) {
		for (int i = 0; i < num_devices; ++i) {
			SDL_AudioDeviceID instance_id = devices[i];
			const char        *name        = SDL_GetAudioDeviceName(instance_id);
			log_debug("Found Audio Device: [%d] %s\n", instance_id, name);

			if (strncmp(name, g_config.audio_device_name, sizeof(g_config.audio_device_name)) == 0) {
				log_debug("Audio device matches config file: %s\n", name);
				g_audio.audio_device = instance_id;
			}
		}
		SDL_free(devices);
	}

	g_audio.stream = SDL_OpenAudioDeviceStream(g_audio.audio_device, NULL, callback_fill_stream_with_data, NULL);

	if (g_audio.stream == NULL) {
		return result_make(false, "unable to create audio stream: %s", SDL_GetError());
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

Result audio_play_file(const char *filepath, struct Audio_File_Metadata *metadata)
{
	(void) metadata;

	int decoder_error = -1;
	g_audio.filestream.handle = mpg123_new(NULL, &decoder_error);
	if (g_audio.filestream.handle == NULL) {
		return result_make(false, "unable to create mpg123 handle: %s", mpg123_plain_strerror(decoder_error));
	}

	if (mpg123_open(g_audio.filestream.handle, filepath) != MPG123_OK) {
		return result_make(false, "failed to open file %s: %s", filepath, mpg123_strerror(g_audio.filestream.handle));
	}


	if (g_audio.stream == NULL) {
		Result r = audio_open();
		if (!r.success) return r;
	}
	SDL_ResumeAudioStreamDevice(g_audio.stream);
	return result_make_success();
}

void audio_close(void)
{
	SDL_DestroyAudioStream(g_audio.stream);
	g_audio.stream = NULL;

	mpg123_close(g_audio.filestream.handle);
	mpg123_free(g_audio.filestream.handle);
	g_audio.filestream.handle = NULL;
}

bool audio_is_playing(void)
{
	return !SDL_AudioStreamDevicePaused(g_audio.stream);
}
void audio_pause(void)
{
	SDL_PauseAudioStreamDevice(g_audio.stream);
	g_audio.is_playing = false;
}

void audio_resume(void)
{
	SDL_ResumeAudioStreamDevice(g_audio.stream);
	g_audio.is_playing = true;
}

int audio_get_current_pos_in_secs(void)
{
	return 10;
}

void audio_set_pos(double pos_secs)
{
	// TODO: implementation
	(void) pos_secs;
}

