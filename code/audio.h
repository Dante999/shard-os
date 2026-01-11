#ifndef AUDIO_H
#define AUDIO_H

#include "libcutils/result.h"

Result audio_open(void);
void audio_close(void);

#endif // AUDIO_H
