#ifndef AUDIO_H
#define AUDIO_H

#include "libcutils/result.h"

//struct Audio_Callback {
//void (*)(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount);
//}
Result audio_open(void);
void audio_close(void);

#endif // AUDIO_H
