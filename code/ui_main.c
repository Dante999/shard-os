#include "ui_main.h"

#include "config.h"
#include "ui_radio.h"
#include "libcutils/logger.h"

#include <time.h>

void ui_main_draw_header(struct Screen *screen)
{
	log_debug("mouse: x=%4d y=%4d\n", screen->mouse_x, screen->mouse_y);
	screen_draw_box(
		screen,
		SCREEN_BORDER_WIDTH,
		SCREEN_BORDER_WIDTH,
		SCREEN_LOGICAL_WIDTH-2*SCREEN_BORDER_WIDTH,
		100,
		false);

	screen_draw_text(screen, 20, 20, g_config.screen_font_size_l, "ShardOS");
	screen_draw_text(screen, 20, 20+5+g_config.screen_font_size_l, g_config.screen_font_size_s, "v0.1");

	time_t now = time(NULL);
	struct tm *tm = localtime(&now);           /* use gmtime(&now) for UTC */
	char buf[32];

	strftime(buf, sizeof buf, "%d.%m.%Y", tm);
	screen_draw_text(screen, SCREEN_LOGICAL_WIDTH-200, 20, g_config.screen_font_size_m, buf);

	strftime(buf, sizeof buf, " %H:%M:%S", tm); // extra space to be aligned
						    // with date
	screen_draw_text(screen, SCREEN_LOGICAL_WIDTH-200, 20+g_config.screen_font_size_m, g_config.screen_font_size_m, buf);

	ui_radio_render(screen);

}
