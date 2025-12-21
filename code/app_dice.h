#ifndef APP_DICE_H
#define APP_DICE_H

#include "screen.h"
#include "ui_main.h"

void app_dice_init(struct Screen *screen);
void app_dice_open(struct Screen *screen);
void app_dice_render(struct Screen *screen);
void app_dice_close(struct Screen *screen);

#endif // APP_DICE_H
