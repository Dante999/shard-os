#ifndef AUDIOSTREAMER_H
#define AUDIOSTREAMER_H

#include "libcutils/result.h"

void audiostreamer_init(void);
void audiostreamer_stop(void);
Result audiostreamer_start(const char *url);

#endif // AUDIOSTREAMER_H
