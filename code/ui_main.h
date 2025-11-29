#ifndef UI_MAIN_H
#define UI_MAIN_H

#include "screen.h"

enum App_Status {
	APP_STATUS_RUNNING,
	APP_STATUS_CLOSED
};

void ui_main_init(struct Screen *screen);
void ui_main_render(struct Screen *screen);

#endif // UI_MAIN_H
