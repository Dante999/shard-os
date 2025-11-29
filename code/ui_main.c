#include "ui_main.h"

#include "config.h"
#include "app_radio.h"
#include "ui_elements.h"

#include "libcutils/logger.h"
#include "libcutils/util_makros.h"

#include <time.h>

#define UI_STATUS_BAR_HEIGHT 70
#define UI_STATUS_BAR_TEXT_Y_START     20
#define UI_STATUS_BAR_DATETIME_X_START 840
#define UI_WINDOW_BORDER               20

struct App {
	const char *name;
	void (*app_open)(struct Screen *screen);
	enum App_Status (*app_render)(struct Screen *screen);
	void (*app_close)(struct Screen *screen);
};

int g_active_app_idx = -1;
struct App g_apps[] = {
	{"Radio", app_radio_open, app_radio_render, app_radio_close},
	{"Radio", app_radio_open, app_radio_render, app_radio_close},
	{"Radio", app_radio_open, app_radio_render, app_radio_close},
	{"Radio", app_radio_open, app_radio_render, app_radio_close},
	{"Radio", app_radio_open, app_radio_render, app_radio_close},
	{"Radio", app_radio_open, app_radio_render, app_radio_close},
	{"Radio", app_radio_open, app_radio_render, app_radio_close},
	{"Radio", app_radio_open, app_radio_render, app_radio_close},
	{"Radio", app_radio_open, app_radio_render, app_radio_close},
};

void ui_main_init(struct Screen *screen)
{
	app_radio_init(screen, "radiostations.txt");
}


static void ui_main_draw_header(struct Screen *screen)
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

}

static void on_app_clicked(struct Ui_Box *box)
{
	int app_index = atoi(box->id);
	log_info("On app with id %d clicked!\n", app_index);

	g_active_app_idx = app_index;
}

#define APP_GRID_X_START  50
#define APP_GRID_Y_START  150
#define APP_BOX_WIDTH     70
#define APP_BOX_HEIGHT    80
#define APP_TEXT_Y_OFFSET 25
#define APPS_PER_LINE      5
#define APP_BOX_CLEARANCE 30
#define APP_BOX_FONT_SIZE g_config.screen_font_size_s
static void ui_draw_app_icons(struct Screen *screen)
{
	for (size_t i=0; i < ARRAY_SIZE(g_apps); ++i) {
		const int x = APP_GRID_X_START+(APP_BOX_WIDTH+APP_BOX_CLEARANCE)*(i%APPS_PER_LINE);
		const int y = APP_GRID_Y_START+(APP_BOX_HEIGHT+APP_BOX_CLEARANCE+APP_BOX_FONT_SIZE)*(i/APPS_PER_LINE);

		struct Screen_Dimension text_size = screen_get_text_dimension(screen, APP_BOX_FONT_SIZE, g_apps[i].name);

		struct Ui_Box box = {
			.x  = x,
			.y  = y,
			.w  = APP_BOX_WIDTH,
			.h  = APP_BOX_HEIGHT,
			.on_click = on_app_clicked,
			.is_selectable = true
		};
		snprintf(box.id, sizeof(box.id), "%zu", i);

		ui_box_render(screen, &box);

		int clearance = (APP_BOX_WIDTH-text_size.w)/2;
		if (clearance < 0) clearance = 0;

		screen_draw_text(screen, x+clearance, y+APP_TEXT_Y_OFFSET, APP_BOX_FONT_SIZE, g_apps[i].name);

	}
}

void ui_main_render(struct Screen *screen)
{
	ui_main_draw_header(screen);

	if (g_active_app_idx == -1) {
		ui_draw_app_icons(screen);
	}
	else {
		struct App *active_app = &g_apps[g_active_app_idx];

		const int window_y_pos = SCREEN_BORDER_WIDTH+UI_STATUS_BAR_HEIGHT+UI_WINDOW_BORDER;
		struct Ui_Window window = {
			.x = UI_WINDOW_BORDER,
			.y = window_y_pos,
			.w = SCREEN_LOGICAL_WIDTH-2*UI_WINDOW_BORDER,
			.h = SCREEN_LOGICAL_HEIGHT-window_y_pos-UI_WINDOW_BORDER
		};

		ui_window_render(screen, &window);

		if (!window.should_close) {
			active_app->app_render(screen);
		}
		else {
			log_debug("Closing app %s\n", active_app->name);
			active_app->app_close(screen);
			g_active_app_idx = -1;
		}
	}
}
