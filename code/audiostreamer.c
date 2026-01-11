#include "audiostreamer.h"

#include <SDL.h>
#include <SDL_mixer.h>
#include <curl/curl.h>

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "libcutils/logger.h"

#define RINGBUFFER_IMPLEMENTATION
#include "libcutils/ringbuffer.h"

#define RINGBUFFER_SIZE (5 * 1024 * 1024)



struct Audiostreamer{
	uint8_t  rbuffer_data[RINGBUFFER_SIZE];
	struct Ringbuffer rbuffer;
	bool   eof;                       /* set when curl finishes (unlikely) */
	pthread_mutex_t lock;
	pthread_cond_t  can_read;
	pthread_cond_t  can_write;
	bool stream_start;
	bool quit;
	Mix_Music *music;
};

static struct Audiostreamer g_streamer;



static Sint64 rwsize(SDL_RWops *context) {
	log_debug("rwsize: unknown\n");
	return -1; }   /* unknown size */

static size_t rwread(SDL_RWops *context, void *ptr, size_t size, size_t maxnum)
{
	struct Audiostreamer *buf = (struct Audiostreamer *)context->hidden.unknown.data1;
	size_t want = size * maxnum;
	size_t got  = 0;

	while (got < want) {
		pthread_mutex_lock(&buf->lock);
		log_debug("read: reading %zu bytes\n", maxnum);

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

		ringbuffer_read(&buf->rbuffer, (uint8_t *)ptr, chunk);
		got += chunk;

		pthread_cond_signal(&buf->can_write);
		pthread_mutex_unlock(&buf->lock);
	}

	const size_t num_read = got / size;
	if (maxnum > 0) {
		log_debug("rwread: request %zu byte(s), returned %zu\n", maxnum, num_read);
	}
	return num_read;
}

/* No seeking – streaming only */
static Sint64 rwseek(SDL_RWops *context, Sint64 offset, int whence)
{
	char whence_str[20];
	struct Audiostreamer *buf = (struct Audiostreamer *)context->hidden.unknown.data1;
	Sint64 result = 0;
	switch (whence) {
		case RW_SEEK_SET:
			strncpy(whence_str, "RW_SEEK_SET", sizeof(whence_str));
			result = offset;
			break;
		case RW_SEEK_CUR:
			strncpy(whence_str, "RW_SEEK_CUR", sizeof(whence_str));
			result = offset;
			break;
		case RW_SEEK_END:
			strncpy(whence_str, "RW_SEEK_END", sizeof(whence_str));
			result = (Sint64) ringbuffer_bytes_used(&buf->rbuffer) + offset;
			break;
		default: assert(false);
	}

	log_debug("rwseek: %s -> %d: %d\n", whence_str, offset, result);
	return result;
}

static int rwclose(SDL_RWops *context) { return 0; }

/* Helper to create the RWops */
static SDL_RWops *create_sdl_opts_from_audiostreamer(struct Audiostreamer *buf)
{
	SDL_RWops *rw = SDL_AllocRW();
	if (!rw) return NULL;
	rw->size   = rwsize;
	rw->seek   = rwseek;
	rw->read   = rwread;
	rw->write  = NULL;
	rw->close  = rwclose;
	rw->type   = SDL_RWOPS_UNKNOWN;
	rw->hidden.unknown.data1 = buf;
	return rw;
}

static size_t buffer_write(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	struct Audiostreamer *buf = (struct Audiostreamer*)userdata;
	size_t bytes_to_write = size * nmemb;
	size_t bytes_written  = 0;

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
		size_t chunk_size       = bytes_to_write - bytes_written;
		if (chunk_size > bytes_free) chunk_size = bytes_free;

		log_debug("write: writing write %zu bytes (%zu bytes free)\n", chunk_size, bytes_free);
		if (buf->stream_start && ringbuffer_empty(&buf->rbuffer)) {
			log_debug("write: header, writing first 12 bytes two times\n");
			ringbuffer_write(&buf->rbuffer, (uint8_t *) ptr + bytes_written, 12);
			buf->stream_start = false;
			log_debug("write: bytes used after first header: %zu\n", ringbuffer_bytes_used(&buf->rbuffer));
		}
		ringbuffer_write(&buf->rbuffer, (uint8_t *) ptr + bytes_written, chunk_size);
		bytes_written += chunk_size;

		pthread_cond_signal(&buf->can_read);
		pthread_mutex_unlock(&buf->lock);
	}
	return bytes_to_write;
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
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, buffer_write);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &g_streamer);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "DuckAI/1.0");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);          /* no timeout for live stream */

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		log_error("curl error: %s\n", curl_easy_strerror(res));
	}

	log_debug("CURL END!\n");
	pthread_mutex_lock(&g_streamer.lock);
	g_streamer.eof = true;
	pthread_cond_signal(&g_streamer.can_read);
	pthread_mutex_unlock(&g_streamer.lock);

	curl_easy_cleanup(curl);

	return NULL;
}


void audiostreamer_init(void)
{
	memset(&g_streamer, 0, sizeof(g_streamer));
	ringbuffer_init(&g_streamer.rbuffer, g_streamer.rbuffer_data, sizeof(g_streamer.rbuffer_data));
	pthread_mutex_init(&g_streamer.lock, NULL);
	pthread_cond_init(&g_streamer.can_read, NULL);
	pthread_cond_init(&g_streamer.can_write, NULL);
}

void audiostreamer_stop(void)
{
	g_streamer.quit = true;
	log_info("stopping stream\n");
	Mix_HaltMusic();

	if (g_streamer.music != NULL) {
		log_debug("freeing *music\n");
		Mix_FreeMusic(g_streamer.music);
	}
}

Result audiostreamer_start(const char *url)
{
	g_streamer.stream_start = true;
	g_streamer.quit         = false;
	g_streamer.eof          = false;
	ringbuffer_reset(&g_streamer.rbuffer);
	/* start curl thread */
	pthread_t th;
	if (pthread_create(&th, NULL, curl_thread, (void*)url) != 0) {
		return result_make(false, "Failed to start curl thread\n");
	}
	pthread_detach(th);   /* we don't need to join later */

	/* create SDL_RWops that reads from the circular buffer */
	SDL_RWops *rw = create_sdl_opts_from_audiostreamer(&g_streamer);
	if (!rw) {
		return result_make(false, "Failed to create SDL_RWops");
	}

	SDL_Delay(200);

	log_debug("Loading music...\n");
	g_streamer.music = Mix_LoadMUS_RW(rw, 1);   /* 1 = SDL will free rw */
	if (g_streamer.music == NULL) {
		result_make(false, "Mix_LoadMUS_RW error: %s\n", Mix_GetError());
	}

	if (Mix_PlayMusic(g_streamer.music, -1) < 0) {
		result_make(false, "Mix_LoadMUS_RW error: %s\n", Mix_GetError());
	}

	return result_make_success();
}
