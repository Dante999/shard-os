#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include "libcutils/result.h"

struct Audio_File_Metadata {
	char artist[255];
	char title[255];
	double length_secs;
};

void audioplayer_stop(void);
Result audioplayer_play_file(const char *filepath, struct Audio_File_Metadata *metadata);
bool audioplayer_is_playing(void);
void audioplayer_pause(void);
void audioplayer_resume(void);
int audioplayer_get_current_pos_in_secs(void);
void audioplayer_set_pos(double pos_secs);

#endif // AUDIOPLAYER_H
