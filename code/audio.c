#include "audio.h"

#include <SDL3/SDL.h>
#include <curl/curl.h>
#include <mpg123.h>

#include <assert.h>
#include <pthread.h>
#include <unistd.h>

#include "libcutils/logger.h"
#include "libcutils/util_makros.h"


#include "config.h"

#define RINGBUFFER_SIZE (1024 * 1024)
#define MAX_FEED_CAPACITY (1024 *1024)
#define DOWNLOAD_BUFFER_SIZE RINGBUFFER_SIZE
#define USE_DOUBLE_BUFFER
enum Stream_Type {
	STREAM_TYPE_NONE,
	STREAM_TYPE_FILE,
	STREAM_TYPE_URL
};

struct Static_Buffer {
	uint8_t data[DOWNLOAD_BUFFER_SIZE];
	size_t used;
};


size_t static_buffer_bytes_free(const struct Static_Buffer *b) { return (sizeof(b->data) - b->used); }
void   static_buffer_dup(struct Static_Buffer *dst, const struct Static_Buffer *src) {
	memcpy(dst->data, src->data, src->used);
	dst->used = src->used;
}

void   static_buffer_reset(struct Static_Buffer *b) { b->used = 0; }
size_t static_buffer_append(struct Static_Buffer *b, uint8_t *data, size_t data_len) {
	const size_t bytes_to_write = MIN(static_buffer_bytes_free(b), data_len);
	memcpy(b->data+b->used, data, bytes_to_write);
	b->used += bytes_to_write;
	return bytes_to_write;
}


static struct Shard_Audio {
	SDL_AudioStream *stream;
	enum Stream_Type  type;
	mpg123_handle    *decode_handle;
	bool              is_playing;
	bool              is_format_set;
	size_t bytes_feed;
	struct Static_Buffer download_buffer;
	struct Urlstream {
		pthread_t download_thread;
		bool thread_running;
		bool   eof;                       /* set when curl finishes (unlikely) */
		pthread_mutex_t lock;
		//pthread_cond_t  can_read;
		//pthread_cond_t  can_write;
		bool quit;

	} stream_by_url;
} g_audio;

static void set_audio_format_if_needed(void)
{
	if (g_audio.is_format_set) return;

	long rate;
	int channels, enc;
	if (mpg123_getformat(g_audio.decode_handle, &rate, &channels, &enc) == MPG123_OK) {
		log_info("New format: %li Hz, %i channels, encoding value %i\n", rate, channels, enc);

		SDL_AudioSpec src_spec;
		SDL_GetAudioStreamFormat(g_audio.stream, &src_spec, NULL);
		log_info("SDL format: %li Hz, %i channels, encoding value %i\n", src_spec.freq, src_spec.channels, src_spec.format);

		src_spec.freq = (int)rate;
		SDL_SetAudioStreamFormat(g_audio.stream, &src_spec, NULL);

		g_audio.is_format_set = true;
	}
}

static size_t fill_stream_from_url(uint8_t *dst, size_t bytes_wanted)
{
#ifdef USE_DOUBLE_BUFFER
	static struct Static_Buffer tmp_buffer = {0};

	if (audio_get_buffered_bytes() < MAX_FEED_CAPACITY) {
		pthread_mutex_lock(&g_audio.stream_by_url.lock);
		static_buffer_dup(&tmp_buffer, &g_audio.download_buffer);
		g_audio.download_buffer.used = 0;
		pthread_mutex_unlock(&g_audio.stream_by_url.lock);

		mpg123_feed(
			g_audio.decode_handle,
			tmp_buffer.data,
			tmp_buffer.used);
		g_audio.bytes_feed += tmp_buffer.used;
	}
	size_t bytes_done;
	mpg123_read(g_audio.decode_handle, dst, bytes_wanted, &bytes_done);
	//log_info("read: wanted: %zu, done: %zu, feed: %zu\n", bytes_wanted, bytes_done, g_audio.bytes_feed);
	if (bytes_done > g_audio.bytes_feed) {
		g_audio.bytes_feed = 0;
	}
	else {
		g_audio.bytes_feed -= bytes_done;
	}
	return bytes_done;
#else
	size_t bytes_done;

	pthread_mutex_lock(&g_audio.stream_by_url.lock);
	mpg123_read(g_audio.decode_handle, dst, bytes_wanted, &bytes_done);

	//pthread_cond_signal(&g_audio.stream_by_url.can_write);
	pthread_mutex_unlock(&g_audio.stream_by_url.lock);

	if (bytes_done > g_audio.bytes_feed) {
		g_audio.bytes_feed = 0;
	}
	else {
		g_audio.bytes_feed -= bytes_done;
	}
	return bytes_done;
#endif
}

static size_t curl_buffer_write_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	UNUSED(userdata);
	struct Urlstream *buf = &g_audio.stream_by_url;

	pthread_mutex_lock(&buf->lock);
	buf->thread_running = true;
	pthread_mutex_unlock(&buf->lock);

	const size_t bytes_total   = size * nmemb;
	size_t       bytes_written = 0;

	while (bytes_written < bytes_total) {
		pthread_mutex_lock(&buf->lock);

		if (buf->quit) {
			log_info("write: detected quit action,!\n");
			buf->thread_running = false;
			pthread_mutex_unlock(&buf->lock);
			return CURL_WRITEFUNC_ERROR;
		}

		size_t bytes_left = bytes_total-bytes_written;
#ifdef USE_DOUBLE_BUFFER

		size_t tmp = static_buffer_append(
			&g_audio.download_buffer,
			(uint8_t*)ptr+bytes_written,
			bytes_left);

		bytes_written += tmp;
		log_debug("write: buffered %zu bytes (%zu total)\n", tmp, g_audio.download_buffer.used);
		if (tmp < bytes_left) {
			log_debug("write: buffer full, waiting...\n");
			pthread_mutex_unlock(&buf->lock);
			sleep(1);
			continue;
		}

		pthread_mutex_unlock(&buf->lock);

#else
		if (g_audio.bytes_feed+bytes_left >= RINGBUFFER_SIZE) {
			log_debug("write: buffer full, waiting...\n");
			pthread_mutex_unlock(&buf->lock);
			sleep(1);
			continue;
			//pthread_cond_wait(&buf->can_write, &buf->lock);
		}

		/*
		 * safe in ringbuffer and decode when needed to download as fast
		 * as possible?
		 */
		mpg123_feed(
			g_audio.decode_handle,
			(uint8_t*)ptr+bytes_written,
			bytes_left);
		g_audio.bytes_feed += bytes_left;
		//log_debug("write: bytes feed total: %zu\n", g_audio.bytes_feed);
		bytes_written += bytes_left;
		//pthread_cond_signal(&buf->can_read);
#endif
		pthread_mutex_unlock(&buf->lock);
	}
	return bytes_written;
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

	//SDL_Delay(200);
	log_debug("CURL END!\n");
	pthread_mutex_lock(&g_audio.stream_by_url.lock);
	g_audio.stream_by_url.eof = true;
	//pthread_cond_signal(&g_audio.stream_by_url.can_read);
	pthread_mutex_unlock(&g_audio.stream_by_url.lock);

	curl_easy_cleanup(curl);

	return NULL;
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

static void fill_sdl_stream_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
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

	set_audio_format_if_needed();
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

static void clear_download_and_cache(void)
{
	pthread_mutex_lock(&g_audio.stream_by_url.lock);
	if (g_audio.stream_by_url.thread_running) {
		g_audio.stream_by_url.quit = true;
		pthread_mutex_unlock(&g_audio.stream_by_url.lock);
		assert(pthread_join(g_audio.stream_by_url.download_thread, NULL) == 0);
		log_info("stopped old download thread\n");
	}
	else {
		pthread_mutex_unlock(&g_audio.stream_by_url.lock);
	}
	static_buffer_reset(&g_audio.download_buffer);
}

static void init_play_audio(void)
{
	PRECONDITION(g_audio.stream != NULL);
	PRECONDITION(g_audio.decode_handle != NULL);

	clear_download_and_cache();

	mpg123_close(g_audio.decode_handle);
	g_audio.bytes_feed = 0;
	g_audio.stream_by_url.quit         = false;
	g_audio.stream_by_url.eof          = false;
	g_audio.stream_by_url.thread_running = false;
	g_audio.is_format_set = false;
}

Result audio_play_url(const char *url)
{
	init_play_audio();
	/* start curl thread */
	//pthread_t th;
	if (pthread_create(&g_audio.stream_by_url.download_thread, NULL, curl_thread, (void*)url) != 0) {
		return result_make(false, "Failed to start curl thread\n");
	}
	//pthread_detach(th);   /* we don't need to join later */

	SDL_Delay(1000);

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
	init_play_audio();

	UNUSED(metadata);

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

	g_audio.stream = SDL_OpenAudioDeviceStream(audio_device, NULL, fill_sdl_stream_callback, NULL);
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
	pthread_mutex_init(&g_audio.stream_by_url.lock, NULL);
	//pthread_cond_init(&g_audio.stream_by_url.can_read, NULL);
	//pthread_cond_init(&g_audio.stream_by_url.can_write, NULL);

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
		//pthread_cond_signal(&g_audio.stream_by_url.can_write);
	}

	g_audio.is_playing = false;
	g_audio.type = STREAM_TYPE_NONE;

	pthread_mutex_destroy(&g_audio.stream_by_url.lock);
	//pthread_cond_destroy(&g_audio.stream_by_url.can_read);
	//pthread_cond_destroy(&g_audio.stream_by_url.can_write);
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

int audio_get_buffered_bytes(void) {
	long buffered_bytes;
	mpg123_getstate(g_audio.decode_handle, MPG123_BUFFERFILL, &buffered_bytes, NULL);
	return (int) buffered_bytes;
}
int audio_get_buffered_percent(void)
{
	return audio_get_buffered_bytes()*100/MAX_FEED_CAPACITY;
}
