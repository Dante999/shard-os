#include "audio.h"

#include <SDL3/SDL.h>
#include <curl/curl.h>
#include <mpg123.h>

#include <assert.h>
#include <pthread.h>

#include "libcutils/logger.h"
#include "libcutils/util_makros.h"

#define RINGBUFFER_IMPLEMENTATION
#include "libcutils/ringbuffer.h"

#include "config.h"

#define RINGBUFFER_SIZE (5 * 1024 * 1024)

enum Stream_Type {
	STREAM_TYPE_NONE,
	STREAM_TYPE_FILE,
	STREAM_TYPE_URL
};

static struct Shard_Audio {
	SDL_AudioStream *stream;
	enum Stream_Type  type;
	mpg123_handle    *decode_handle;
	bool              is_playing;
	struct Urlstream {
		uint8_t  rbuffer_data[RINGBUFFER_SIZE];
		struct Ringbuffer rbuffer;
		bool   eof;                       /* set when curl finishes (unlikely) */
		pthread_mutex_t lock;
		pthread_cond_t  can_read;
		pthread_cond_t  can_write;
		bool quit;
		
	} stream_by_url;
} g_audio;

static size_t curl_buffer_write_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
#if 1
	struct Urlstream *buf = (struct Urlstream*)userdata;
	size_t bytes_to_write = size * nmemb;
	size_t bytes_written  = 0;

	static uint8_t decode_buffer[2048];
	while (bytes_written < bytes_to_write) {
		pthread_mutex_lock(&buf->lock);

		if (buf->quit) {
			log_info("write: detected quit action,!\n");
			pthread_cond_signal(&buf->can_read);
			pthread_mutex_unlock(&buf->lock);
			return CURL_WRITEFUNC_ERROR;
		}

		while (ringbuffer_full(&buf->rbuffer)) {
			log_debug("write: buffer full, waiting...\n");
			pthread_cond_wait(&buf->can_write, &buf->lock);
		}

		const size_t bytes_free = ringbuffer_bytes_free(&buf->rbuffer);
		
		size_t chunk_size = MIN((bytes_to_write - bytes_written), bytes_free);
		chunk_size        = MIN(chunk_size, sizeof(decode_buffer));

		size_t bytes_decoded = 0;

		int decode_err = mpg123_decode( 
				g_audio.decode_handle,
				(unsigned char *) ptr, bytes_to_write, 
				decode_buffer, chunk_size, &bytes_decoded);

		if (decode_err != MPG123_OK) {
			log_error("error while decoding from url: %s\n", mpg123_plain_strerror(decode_err));
		}


		//size_t chunk_size       = bytes_to_write - bytes_written;
		//if (chunk_size > bytes_free) chunk_size = bytes_free;

		log_debug("write: writing %zu bytes (%zu bytes filled)\n", chunk_size, ringbuffer_bytes_used(&buf->rbuffer));
		ringbuffer_write(&buf->rbuffer, decode_buffer, bytes_decoded);
		bytes_written += bytes_decoded;

		pthread_cond_signal(&buf->can_read);
		pthread_mutex_unlock(&buf->lock);
	}
	return bytes_to_write;
#endif
}

static void *curl_thread(void *arg)
{
	const char *url = (const char*) arg;

	CURL *curl = curl_easy_init();
	if (!curl) {
		log_error("curl init failed\n");
		return NULL;
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_buffer_write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &g_audio.stream_by_url);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "DuckAI/1.0");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);          /* no timeout for live stream */

	log_info("start streaming from %s\n", url);
	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		log_error("curl error: %s\n", curl_easy_strerror(res));
	}

	log_debug("CURL END!\n");
	pthread_mutex_lock(&g_audio.stream_by_url.lock);
	g_audio.stream_by_url.eof = true;
	pthread_cond_signal(&g_audio.stream_by_url.can_read);
	pthread_mutex_unlock(&g_audio.stream_by_url.lock);

	curl_easy_cleanup(curl);

	return NULL;
}

static size_t fill_stream_from_url(uint8_t *dst, size_t bytes_wanted)
{
#if 1
	struct Urlstream *buf = &g_audio.stream_by_url;
	size_t want = bytes_wanted;
	size_t got  = 0;

	while (got < bytes_wanted) {
		pthread_mutex_lock(&buf->lock);

		while (ringbuffer_empty(&buf->rbuffer) && !buf->eof) {
			log_warning("read: buffer empty, waiting...\n");
			pthread_cond_wait(&buf->can_read, &buf->lock);
		}

		if (ringbuffer_empty(&buf->rbuffer) && buf->eof) {   /* nothing left */
			pthread_mutex_unlock(&buf->lock);
			break;
		}

		size_t avail = ringbuffer_bytes_used(&buf->rbuffer);
		size_t chunk = want - got;
		if (chunk > avail) chunk = avail;

		ringbuffer_read(&buf->rbuffer, dst, chunk);
		got += chunk;

		pthread_cond_signal(&buf->can_write);
		pthread_mutex_unlock(&buf->lock);
	}

	if (got > 0) {
		log_debug("read %zu byte(s), requested %zu\n", got, bytes_wanted);
	}
	return got;
#endif
}

static size_t fill_stream_from_file(uint8_t *dst, size_t bytes_wanted)
{
	size_t bytes_decoded = 0;
	int merror = mpg123_read(g_audio.decode_handle, dst, bytes_wanted, &bytes_decoded);

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
			bytes_decoded = fill_stream_from_url(buffer, bytes_wanted);
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
	SDL_AudioDeviceID result = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;

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


Result audio_play_url(const char *url)
{

	PRECONDITION(g_audio.stream != NULL);
	PRECONDITION(g_audio.decode_handle != NULL);

	g_audio.stream_by_url.quit         = false;
	g_audio.stream_by_url.eof          = false;
	ringbuffer_reset(&g_audio.stream_by_url.rbuffer);
	/* start curl thread */
	pthread_t th;
	if (pthread_create(&th, NULL, curl_thread, (void*)url) != 0) {
		return result_make(false, "Failed to start curl thread\n");
	}
	pthread_detach(th);   /* we don't need to join later */

	SDL_Delay(2000);

	if (mpg123_open_feed(g_audio.decode_handle) != MPG123_OK) {
		return result_make(false, "failed to open feed: %s", 
			mpg123_strerror(g_audio.decode_handle));
	}

	g_audio.type = STREAM_TYPE_URL;
	SDL_ResumeAudioStreamDevice(g_audio.stream);

	return result_make_success();
}

Result audio_play_file(const char *filepath, struct Audio_File_Metadata *metadata)
{
	UNUSED(metadata);

	PRECONDITION(g_audio.stream != NULL);
	PRECONDITION(g_audio.decode_handle != NULL);

	if (mpg123_open(g_audio.decode_handle, filepath) != MPG123_OK) {
		return result_make(false, "failed to open file %s: %s", 
			filepath, mpg123_strerror(g_audio.decode_handle));
	}

	g_audio.type = STREAM_TYPE_FILE;
	SDL_ResumeAudioStreamDevice(g_audio.stream);

	return result_make_success();
}

Result audio_open(void)
{
	log_info("opening audio device...\n");
	SDL_AudioDeviceID audio_device = get_audio_device_or_default(g_config.audio_device_name);

	g_audio.stream = SDL_OpenAudioDeviceStream(audio_device, NULL, fill_stream_callback, NULL);
	if (g_audio.stream == NULL) {
		return result_make(false, "unable to create audio stream: %s", SDL_GetError());
	}

	int decoder_error = -1;
	g_audio.decode_handle = mpg123_new(NULL, &decoder_error);
	if (g_audio.decode_handle == NULL) {
		SDL_DestroyAudioStream(g_audio.stream);
		g_audio.stream = NULL;
		return result_make(false, "unable to create mpg123 handle: %s", mpg123_plain_strerror(decoder_error));
	}

	memset(&g_audio.stream_by_url, 0, sizeof(g_audio.stream_by_url));
	ringbuffer_init(&g_audio.stream_by_url.rbuffer, g_audio.stream_by_url.rbuffer_data, sizeof(g_audio.stream_by_url.rbuffer_data));
	pthread_mutex_init(&g_audio.stream_by_url.lock, NULL);
	pthread_cond_init(&g_audio.stream_by_url.can_read, NULL);
	pthread_cond_init(&g_audio.stream_by_url.can_write, NULL);

	return result_make_success();
}

void audio_close(void)
{
	log_info("closing audio device...\n");

	if (g_audio.stream != NULL) {
		SDL_DestroyAudioStream(g_audio.stream);
		g_audio.stream = NULL;
	}

	if (g_audio.decode_handle != NULL) {
		mpg123_close(g_audio.decode_handle);
		mpg123_free(g_audio.decode_handle);
		g_audio.decode_handle = NULL;
	}

	if (g_audio.type == STREAM_TYPE_URL) {
		g_audio.stream_by_url.quit = true;
	}

	g_audio.is_playing = false;
	g_audio.type = STREAM_TYPE_NONE;

	pthread_mutex_destroy(&g_audio.stream_by_url.lock);
	pthread_cond_destroy(&g_audio.stream_by_url.can_read);
	pthread_cond_destroy(&g_audio.stream_by_url.can_write);
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

