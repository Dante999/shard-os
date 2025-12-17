#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include "libcutils/result.h"

struct Audio_File_Metadata {
	char artist[255];
	char title[255];
	double length_secs;
};

Result audioplayer_init(void);
Result audioplayer_play_file(const char *filepath, struct Audio_File_Metadata *metadata);
void audioplayer_pause(void);
int audioplayer_get_current_pos_in_secs(void);

#endif // AUDIOPLAYER_H
