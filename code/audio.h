#ifndef AUDIO_H
#define AUDIO_H

#include "libcutils/result.h"

struct Audio_File_Metadata {
	char artist[255];
	char title[255];
	double length_secs;
};

Result audio_play_file(const char *filepath, struct Audio_File_Metadata *metadata);
Result audio_play_url(const char *url);
int audio_get_buffered_bytes(void);
int audio_get_buffered_percent(void);
bool audio_is_playing(void);
void audio_pause(void);
void audio_resume(void);
Result audio_open(void);
void audio_close(void);
int audio_get_current_pos_in_secs(void);
void audio_set_pos(double pos_secs);

#endif // AUDIO_H
