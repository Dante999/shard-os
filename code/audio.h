#ifndef AUDIO_H
#define AUDIO_H

#include "libcutils/result.h"

struct Audio_Metadata{
	char artist[255];
	char title[255];
	double length_secs;
};

enum Play_Status {
	PLAY_STATUS_STOPPED,
	PLAY_STATUS_PLAYING,
	PLAY_STATUS_PAUSED,
	PLAY_STATUS_FINISHED
};

void audio_init(void);
Result audio_play_url(const char *url);
Result audio_play_file(const char *filepath);
Result audio_get_metadata(struct Audio_Metadata *metadata);
int audio_get_buffered_bytes(void);
int audio_get_buffered_percent(void);
bool audio_is_playing(void);
enum Play_Status audio_get_play_status(void);
void audio_pause(void);
void audio_resume(void);
Result audio_open(void);
void audio_close(void);
int audio_get_current_pos_in_secs(void);
void audio_set_pos(int pos_secs);
int audio_get_volume(void);
void audio_set_volume(int volume);

#endif // AUDIO_H
