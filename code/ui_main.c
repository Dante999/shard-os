#include "ui_main.h"

#include "config.h"
#include "ui_radio.h"
#include "libcutils/logger.h"

#include <time.h>

#define UI_STATUS_BAR_HEIGHT 70
#define UI_STATUS_BAR_TEXT_Y_START     20
#define UI_STATUS_BAR_DATETIME_X_START 840
#define UI_WINDOW_BORDER               20

void ui_main_draw_header(struct Screen *screen)
{
	//log_debug("mouse: x=%4d y=%4d\n", screen->mouse_x, screen->mouse_y);
	screen_draw_box(
		screen,
		SCREEN_BORDER_WIDTH,
		SCREEN_BORDER_WIDTH,
		SCREEN_LOGICAL_WIDTH-2*SCREEN_BORDER_WIDTH,
		UI_STATUS_BAR_HEIGHT,
		true);

	screen_draw_text(screen, 20, UI_STATUS_BAR_TEXT_Y_START, g_config.screen_font_size_l, "ShardOS");
	screen_draw_text(screen, 20, UI_STATUS_BAR_TEXT_Y_START+g_config.screen_font_size_l, g_config.screen_font_size_s, "v0.1");

	time_t now = time(NULL);
	struct tm *tm = localtime(&now);           /* use gmtime(&now) for UTC */
	char buf[32];

	strftime(buf, sizeof buf, "%d.%m.%Y", tm);
	screen_draw_text(screen, UI_STATUS_BAR_DATETIME_X_START, UI_STATUS_BAR_TEXT_Y_START, g_config.screen_font_size_m, buf);

	strftime(buf, sizeof buf, " %H:%M:%S", tm); // extra space to be aligned
						    // with date
	screen_draw_text(screen, UI_STATUS_BAR_DATETIME_X_START, UI_STATUS_BAR_TEXT_Y_START+g_config.screen_font_size_m, g_config.screen_font_size_m, buf);

	const int window_y_pos = SCREEN_BORDER_WIDTH+UI_STATUS_BAR_HEIGHT+UI_WINDOW_BORDER;
	screen_draw_window(screen,UI_WINDOW_BORDER, window_y_pos, SCREEN_LOGICAL_WIDTH-2*UI_WINDOW_BORDER, SCREEN_LOGICAL_HEIGHT-window_y_pos-UI_WINDOW_BORDER, "Radiostation");
	ui_radio_render(screen);
}

