#include "audio.h"

#include <SDL3/SDL.h>
#include <mpg123.h>

#include <assert.h>

#include "libcutils/logger.h"
#include "libcutils/util_makros.h"

#include "config.h"

enum Stream_Type {
	STREAM_TYPE_NONE,
	STREAM_TYPE_FILE,
	STREAM_TYPE_URL
};

static struct Shard_Audio {
	SDL_AudioStream *stream;
	enum Stream_Type type;
	bool is_playing;
	struct Filestream {
		mpg123_handle *handle;
	} filestream;
} g_audio;

static size_t fill_stream_from_file(uint8_t *dst, size_t bytes_wanted)
{
	size_t bytes_decoded = 0;
	int merror = mpg123_read(g_audio.filestream.handle, dst, bytes_wanted, &bytes_decoded);

	if (merror == MPG123_DONE) {
		// TODO: play next track
		audio_pause();
	}
	else if (merror != MPG123_OK) {
		log_error("failed to decode audio file: %s\n", mpg123_plain_strerror(merror));
	}

	return bytes_decoded;
}

static void fill_stream_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
	uint8_t buffer[5000];
	size_t bytes_decoded = 0;

	const size_t bytes_wanted  = MIN(sizeof(buffer),(size_t)additional_amount);
	switch (g_audio.type) {
		case STREAM_TYPE_NONE:
			log_error("forbidden state! Stream callback but not stream type is set!\n");
			assert(false);
			break;

		case STREAM_TYPE_FILE:
			bytes_decoded = fill_stream_from_file(buffer, bytes_wanted);
			break;

		case STREAM_TYPE_URL:
			log_error("stream type URL not implemented!\n");
	}

	 if (!SDL_PutAudioStreamData(g_audio.stream, buffer, (int)bytes_decoded)) {
		log_error("failed to put audio stream data: %s\n", SDL_GetError());
		return;
	}
	(void) userdata;
	(void) stream;
	(void) additional_amount;
	(void) total_amount;
}

static SDL_AudioDeviceID get_audio_device_or_default(const char *device_name)
{
	log_debug("searching for audio device from config file: %s\n", device_name);
	SDL_AudioDeviceID result =SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;

	int num_devices = -1;
	SDL_AudioDeviceID *devices = SDL_GetAudioPlaybackDevices(&num_devices);
	if (devices) {
		for (int i = 0; i < num_devices; ++i) {
			SDL_AudioDeviceID instance_id = devices[i];
			const char        *name        = SDL_GetAudioDeviceName(instance_id);

			if (strcmp(name, device_name) == 0) {
				log_debug("audio device matches: [%d] %s\n", instance_id, name);
				result = instance_id;
				// do not exit loop on purpose, log all devices
			}
			else {
				log_debug("audio device doesn't match: [%d] %s\n", instance_id, name);
			}
		}
		SDL_free(devices);
	}
	return result;
}

static Result audio_open(void)
{
	SDL_AudioDeviceID audio_device = get_audio_device_or_default(g_config.audio_device_name);

	g_audio.stream = SDL_OpenAudioDeviceStream(audio_device, NULL, fill_stream_callback, NULL);
	if (g_audio.stream == NULL) {
		return result_make(false, "unable to create audio stream: %s", SDL_GetError());
	}

	return result_make_success();
}

Result audio_play_file(const char *filepath, struct Audio_File_Metadata *metadata)
{
	Result result;

	(void) metadata;

	int decoder_error = -1;
	g_audio.filestream.handle = mpg123_new(NULL, &decoder_error);
	if (g_audio.filestream.handle == NULL) {
		result = result_make(false, "unable to create mpg123 handle: %s", mpg123_plain_strerror(decoder_error));
		goto err_out_mpg123_handle;
	}

	if (mpg123_open(g_audio.filestream.handle, filepath) != MPG123_OK) {
		result = result_make(false, "failed to open file %s: %s", filepath, mpg123_strerror(g_audio.filestream.handle));
		goto err_out_mpg123_open;
	}

	if (g_audio.stream == NULL) {
		result = audio_open();
		if (!result.success) {
			goto err_out_sdl_stream;
		}
	}

	g_audio.type = STREAM_TYPE_FILE;
	SDL_ResumeAudioStreamDevice(g_audio.stream);
	return result_make_success();


err_out_sdl_stream:
	mpg123_close(g_audio.filestream.handle);
err_out_mpg123_open:
	mpg123_free(g_audio.filestream.handle);
	g_audio.filestream.handle = NULL;
err_out_mpg123_handle:
	g_audio.type = STREAM_TYPE_NONE;
	return result;
}

void audio_close(void)
{
	SDL_DestroyAudioStream(g_audio.stream);
	g_audio.stream = NULL;

	if (g_audio.type == STREAM_TYPE_FILE) {
		mpg123_close(g_audio.filestream.handle);
		mpg123_free(g_audio.filestream.handle);
		g_audio.filestream.handle = NULL;
	}

	g_audio.type = STREAM_TYPE_NONE;
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

