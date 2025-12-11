#ifndef APP_JUKEBOX_H
#define APP_JUKEBOX_H

#include "screen.h"
#include "ui_main.h"

void app_jukebox_init(struct Screen *screen, const char *filepath);

void app_jukebox_open(struct Screen *screen);
enum App_Status app_jukebox_render(struct Screen *screen);
void app_jukebox_close(struct Screen *screen);


#endif // APP_JUKEBOX_H
