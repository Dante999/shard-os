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
	enum Play_Status play_status;
	bool              is_format_set;
	size_t bytes_feed;
	struct Audio_Metadata metadata;
	struct Track_Info {
		long rate_hz;
		int channels;
		int encoding;
	} track_info;

	struct Static_Buffer download_buffer;
	struct Urlstream {
		pthread_t download_thread;
		bool thread_running;
		bool   eof;                       /* set when curl finishes (unlikely) */
		pthread_mutex_t lock;
		bool quit;

	} stream_by_url;
} g_audio;

static void set_audio_format_if_needed(void)
{
	if (g_audio.is_format_set) return;

	int error = mpg123_getformat(
		g_audio.decode_handle, &g_audio.track_info.rate_hz,
		&g_audio.track_info.channels, &g_audio.track_info.encoding);

	if ( error == MPG123_OK) {
		log_info("New format: %li Hz, %i channels, encoding value %i\n", 
			g_audio.track_info.rate_hz,
			g_audio.track_info.channels,
			g_audio.track_info.encoding);

		SDL_AudioSpec src_spec;
		SDL_GetAudioStreamFormat(g_audio.stream, &src_spec, NULL);
		log_info("SDL format: %li Hz, %i channels, encoding value %i\n",
			src_spec.freq, src_spec.channels, src_spec.format);

		src_spec.freq = (int)g_audio.track_info.rate_hz;
		SDL_SetAudioStreamFormat(g_audio.stream, &src_spec, NULL);

		if (g_audio.type == STREAM_TYPE_FILE) {
			off_t samples = mpg123_length(g_audio.decode_handle);
			g_audio.metadata.length_secs = (double) samples / (double)g_audio.track_info.rate_hz;
		}
		g_audio.is_format_set = true;
	}
}

static void copy_mpg123_string(char *dst, mpg123_string *src ,size_t dst_size)
{
    if(!dst || dst_size == 0) return;
    dst[0] = '\0';
    if(!src || !src->p || src->fill == 0) return;
    /* copy at most dst_size-1 bytes */
    size_t n = src->fill;
    if(n > dst_size - 1) n = dst_size - 1;
    memcpy(dst, src->p, n);
    dst[n] = '\0';
}


static Result audio_parse_id3_metadata(void)
{
	struct Audio_Metadata *metadata = &g_audio.metadata;

	mpg123_id3v1 *v1;
	mpg123_id3v2 *v2;

	int error = mpg123_id3(g_audio.decode_handle, &v1, &v2);
	if (error != MPG123_OK) {
		return result_make(
			false,
			"failed to get id3 tags: %s",
			mpg123_plain_strerror(error));
	}
	if (v1 != NULL) {
		strncpy(metadata->artist, v1->artist, sizeof(metadata->artist));
		strncpy(metadata->title, v1->title, sizeof(metadata->title));
		return result_make_success();
	}
	if (v2 != NULL) {
		copy_mpg123_string(metadata->artist, v2->artist, sizeof(metadata->artist));
		copy_mpg123_string(metadata->title, v2->title, sizeof(metadata->title));
		return result_make_success();
	}
	return result_make(false, "no id3 tags contained!");
}

static size_t fill_stream_from_url(uint8_t *dst, size_t bytes_wanted)
{
	static struct Static_Buffer tmp_buffer = {0};

	if (audio_get_buffered_bytes() < MAX_FEED_CAPACITY) {
		pthread_mutex_lock(&g_audio.stream_by_url.lock);
		static_buffer_dup(&tmp_buffer, &g_audio.download_buffer);
		g_audio.download_buffer.used = 0;
		pthread_mutex_unlock(&g_audio.stream_by_url.lock);

		int error = mpg123_feed(
			g_audio.decode_handle,
			tmp_buffer.data,
			tmp_buffer.used);
		g_audio.bytes_feed += tmp_buffer.used;

		if (error != MPG123_OK) {
			log_error("error while mpg123_feed via url: %s\n", mpg123_plain_strerror(error));
		}
	}

	//int retval = mpg123_meta_check(g_audio.decode_handle);
	//log_info("meta check return : 0x%x\n", retval);
	//if ((retval&(1<< MPG123_ID3)) != 0 || (retval&(1<<MPG123_NEW_ID3)) != 0) {
	//	Result ret = audio_parse_id3_metadata();
	//	if (ret.success) {
	//		log_info("successfully parsed id3 metadata:\n"
	//			"\tartist: %s\n"
	//			"\ttitle: %s\n",
	//			g_audio.metadata.artist,
	//			g_audio.metadata.title);
	//	}
	//	else {
	//		log_warning("failed to parse id3 metadata: %s\n", ret.msg);
	//	}
	//}
	//else if (retval != 0) {
	//	// nothing
	//}

	size_t bytes_done;
	int error = mpg123_read(g_audio.decode_handle, dst, bytes_wanted, &bytes_done);
	if (error != MPG123_OK) {
		log_error("error while mpg123_read() via url: %s\n", mpg123_plain_strerror(error));
	}

	if (bytes_done > g_audio.bytes_feed) {
		g_audio.bytes_feed = 0;
	}
	else {
		g_audio.bytes_feed -= bytes_done;
	}
	return bytes_done;
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

		size_t tmp = static_buffer_append(
			&g_audio.download_buffer,
			(uint8_t*)ptr+bytes_written,
			bytes_left);

		bytes_written += tmp;

		if (tmp < bytes_left) {
			log_debug("write: buffer full, waiting...\n");
			pthread_mutex_unlock(&buf->lock);
			sleep(1);
			continue;
		}

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

	log_debug("CURL END!\n");
	pthread_mutex_lock(&g_audio.stream_by_url.lock);
	g_audio.stream_by_url.eof = true;
	pthread_mutex_unlock(&g_audio.stream_by_url.lock);

	curl_easy_cleanup(curl);

	return NULL;
}



static size_t fill_stream_from_file(uint8_t *dst, size_t bytes_wanted)
{
	size_t bytes_decoded = 0;
	int merror = mpg123_read(g_audio.decode_handle, dst, bytes_wanted, &bytes_decoded);

	if (merror == MPG123_DONE) {
		audio_pause();
		g_audio.play_status = PLAY_STATUS_FINISHED;
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

	memset(&g_audio.metadata, 0, sizeof(g_audio.metadata));
	g_audio.bytes_feed = 0;
	g_audio.stream_by_url.quit         = false;
	g_audio.stream_by_url.eof          = false;
	g_audio.stream_by_url.thread_running = false;
	g_audio.is_format_set = false;
}

Result audio_play_url(const char *url)
{
	init_play_audio();

	if (pthread_create(&g_audio.stream_by_url.download_thread, NULL, curl_thread, (void*)url) != 0) {
		return result_make(false, "Failed to start curl thread\n");
	}

	SDL_Delay(1000);

	if (mpg123_open_feed(g_audio.decode_handle) != MPG123_OK) {
		return result_make(false, "failed to open feed: %s",
			mpg123_strerror(g_audio.decode_handle));
	}

	g_audio.type = STREAM_TYPE_URL;
	audio_resume();

	return result_make_success();
}

Result audio_play_file(const char *filepath)
{
	init_play_audio();

	if (mpg123_open(g_audio.decode_handle, filepath) != MPG123_OK) {
		return result_make(false, "failed to open file %s: %s",
			filepath, mpg123_strerror(g_audio.decode_handle));
	}
	mpg123_scan(g_audio.decode_handle);
	Result r = audio_parse_id3_metadata();
	if (!r.success) {
		log_warning("unable to parse id3 metadata: %s\n", r.msg);
	}

	g_audio.type = STREAM_TYPE_FILE;
	set_audio_format_if_needed();
	audio_resume();

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

	audio_set_volume(g_config.volume);
	int decoder_error = -1;
	g_audio.decode_handle = mpg123_new(NULL, &decoder_error);
	if (g_audio.decode_handle == NULL) {
		SDL_DestroyAudioStream(g_audio.stream);
		g_audio.stream = NULL;
		return result_make(false, "unable to create mpg123 handle: %s", mpg123_plain_strerror(decoder_error));
	}

	memset(&g_audio.stream_by_url, 0, sizeof(g_audio.stream_by_url));
	pthread_mutex_init(&g_audio.stream_by_url.lock, NULL);

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

	g_audio.play_status = PLAY_STATUS_STOPPED;
	g_audio.type        = STREAM_TYPE_NONE;

	pthread_mutex_destroy(&g_audio.stream_by_url.lock);
}

enum Play_Status audio_get_play_status(void)
{
	return g_audio.play_status;
}

bool audio_is_playing(void)
{
	//return !SDL_AudioStreamDevicePaused(g_audio.stream);
	return g_audio.play_status == PLAY_STATUS_PLAYING;
}
void audio_pause(void)
{
	if (SDL_PauseAudioStreamDevice(g_audio.stream)) {
		g_audio.play_status = PLAY_STATUS_PAUSED;
	}
}

void audio_resume(void)
{
	if (SDL_ResumeAudioStreamDevice(g_audio.stream)) {
		g_audio.play_status = PLAY_STATUS_PLAYING;
	}
}

int audio_get_current_pos_in_secs(void)
{
	if (g_audio.is_format_set) {
		return (int) (mpg123_tell(g_audio.decode_handle) / g_audio.track_info.rate_hz);
	}

	return 0;
}

void audio_set_pos(int pos_secs)
{
	if (pos_secs < 0) pos_secs = 0;

	off_t new_offset = pos_secs * g_audio.track_info.rate_hz;
	mpg123_seek(g_audio.decode_handle, new_offset, SEEK_SET);
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

int audio_get_volume(void)
{
	return (int)(SDL_GetAudioStreamGain(g_audio.stream) * 100.0f);
}

void audio_set_volume(int volume)
{
	SDL_SetAudioStreamGain(g_audio.stream, (float)volume / 100.f);
}


Result audio_get_metadata(struct Audio_Metadata *metadata)
{
	strncpy(metadata->title, g_audio.metadata.title, sizeof(metadata->title));
	strncpy(metadata->artist, g_audio.metadata.artist, sizeof(metadata->artist));
	metadata->length_secs = g_audio.metadata.length_secs;

	return result_make_success();
}
