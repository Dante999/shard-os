#ifndef APP_RADIO_H
#define APP_RADIO_H

#include "screen.h"
#include "ui_main.h"

void app_radio_init(struct Screen *screen, const char *filepath);
void app_radio_open(struct Screen *screen);
void app_radio_render(struct Screen *screen);
void app_radio_close(struct Screen *screen);

#endif // APP_RADIO_H
