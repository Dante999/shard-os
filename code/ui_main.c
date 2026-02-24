#include "ui_main.h"

#include "config.h"
#include "app_radio.h"
#include "app_jukebox.h"
#include "app_dice.h"
#include "ui_elements.h"
#include "audio.h"

#include "libcutils/logger.h"
#include "libcutils/util_makros.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define UI_STATUS_BAR_HEIGHT 70
#define UI_STATUS_BAR_TEXT_Y_START     20
#define UI_STATUS_BAR_DATETIME_X_START 840
#define UI_WINDOW_BORDER               20
#define UI_STATUS_BAR_X_START_RIGHT    300

struct App {
	const char *name;
	void (*app_open)(struct Screen *screen);
	void (*app_render)(struct Screen *screen);
	void (*app_close)(struct Screen *screen);
};

int g_active_app_idx = -1;
struct App g_apps[] = {
	{"Radio"  , app_radio_open  , app_radio_render  , app_radio_close},
	{"Jukebox", app_jukebox_open, app_jukebox_render, app_jukebox_close},
	{"Dice"   , app_dice_open   , app_dice_render   , app_dice_close},
};


struct {
	struct Ui_Dialog instance;
	bool first_time_open;
} g_dialog;

static bool g_is_sleeping = false;

static void render_audio_settings(struct Screen *screen)
{
	static struct Ui_Chooser_Integer volume_chooser = {
		.name = "Volume",
		.outline = {
			.x = UI_DIALOG_X_START + SCREEN_BORDER_WIDTH,
			.y = UI_DIALOG_Y_START + SCREEN_BORDER_WIDTH,
			.w = (UI_DIALOG_X_END - UI_DIALOG_Y_START - SCREEN_BORDER_WIDTH)/3,
			.h = 0
		},
		.x_value_offset = 100,
		.min_value = 0,
		.max_value = 100,
		.steps = 10,
		.cur_value = 0
	};

	volume_chooser.cur_value = g_config.volume;
	ui_chooser_integer_render(screen, &volume_chooser);

	if (volume_chooser.cur_value != g_config.volume) {
		g_config.volume = volume_chooser.cur_value;
		audio_set_volume(g_config.volume);
	}

}

static void render_sleep_dialog(struct Screen *screen)
{
	UNUSED(screen);
	g_is_sleeping = true;
	g_dialog.first_time_open = false;
	g_dialog.instance.is_open = false;
}


struct Icon {
	const char *name;
	void (*render_dialog)(struct Screen *screen);
};

struct Icon g_icons[] = {
	{"[SLEEP]", render_sleep_dialog},
	{"[AUDIO]", render_audio_settings},
};

void ui_main_init(struct Screen *screen)
{
	app_radio_init(screen  , "data/radiostations.conf");
	app_jukebox_init(screen, "data/jukebox");
	app_dice_init(screen);


}

static void ui_main_draw_header_icons(struct Screen *screen, int x_left, int y_top, int height)
{
	UNUSED(height);

	struct Ui_Button button = {0};
	for (size_t i=0; i < ARRAY_SIZE(g_icons); ++i) {
		struct Icon *icon = &g_icons[i];
		ui_button_init(
			screen,
			&button,
			icon->name,
			x_left,
			y_top);

		button.outline.border = UI_BORDER_NONE;

		if (ui_button_render(screen, &button) == UI_EVENT_CLICKED) {
			g_dialog.first_time_open = true;
			g_dialog.instance.is_open = true;
			g_dialog.instance.name = icon->name;
			g_dialog.instance.render_content = icon->render_dialog;
			icon->render_dialog(screen);
		}
		x_left += button.outline.w + 10;
	}
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

	ui_main_draw_header_icons(screen, UI_STATUS_BAR_X_START_RIGHT, UI_STATUS_BAR_TEXT_Y_START+10, UI_STATUS_BAR_HEIGHT);

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
	struct App *active_app = &g_apps[g_active_app_idx];
	active_app->app_open(box->userdata);
}

#define APP_GRID_X_START  50
#define APP_GRID_Y_START  150
#define APP_BOX_WIDTH     100
#define APP_BOX_HEIGHT    120
#define APP_TEXT_Y_OFFSET 40
#define APPS_PER_LINE      5
#define APP_BOX_CLEARANCE 30
#define APP_BOX_FONT_SIZE g_config.screen_font_size_s
static void ui_draw_app_icons(struct Screen *screen)
{
	for (size_t i=0; i < ARRAY_SIZE(g_apps); ++i) {
		const int x = APP_GRID_X_START+(APP_BOX_WIDTH+APP_BOX_CLEARANCE)*((int)i%APPS_PER_LINE);
		const int y = APP_GRID_Y_START+(APP_BOX_HEIGHT+APP_BOX_CLEARANCE+APP_BOX_FONT_SIZE)*((int)i/APPS_PER_LINE);


		struct Ui_Box box = {
			.outline.x  = x,
			.outline.y  = y,
			.outline.w  = APP_BOX_WIDTH,
			.outline.h  = APP_BOX_HEIGHT,
			.on_click = on_app_clicked,
			.is_selectable = true,
			.userdata = screen
		};
		snprintf(box.id, sizeof(box.id), "%zu", i);

		ui_box_render(screen, &box);


#if 0
		struct Screen_Dimension text_size = screen_get_text_dimension(screen, APP_BOX_FONT_SIZE, g_apps[i].name);
		int clearance = (APP_BOX_WIDTH-text_size.w)/2;

		if (clearance < 0) clearance = 0;
		screen_draw_text(screen, x+clearance, y+APP_TEXT_Y_OFFSET, APP_BOX_FONT_SIZE, g_apps[i].name);
#else

		// TODO: Reuse and reduce complexity
		const char *appname = g_apps[i].name;
		const char *newline = strchr(appname, '\n');

		if (newline == NULL) {
			struct Screen_Dimension text_size = screen_get_text_dimension(screen, APP_BOX_FONT_SIZE, appname);
			int clearance = (APP_BOX_WIDTH-text_size.w)/2;
			if (clearance < 0) clearance = 0;
			screen_draw_text(screen, x+clearance, y+APP_TEXT_Y_OFFSET, APP_BOX_FONT_SIZE, appname);
		}
		else {
			struct Screen_Dimension text_size = {0};
			int clearance = 0;
			char buffer[40];

			strncpy(buffer, appname, (size_t)(newline-appname)+1);
			text_size = screen_get_text_dimension(screen, APP_BOX_FONT_SIZE, buffer);
			clearance = (APP_BOX_WIDTH-text_size.w)/2;
			if (clearance < 0) clearance = 0;
			screen_draw_text(screen, x+clearance, y+APP_TEXT_Y_OFFSET, APP_BOX_FONT_SIZE, buffer);

			strncpy(buffer, newline, sizeof(buffer));
			text_size = screen_get_text_dimension(screen, APP_BOX_FONT_SIZE, buffer);
			clearance = (APP_BOX_WIDTH-text_size.w)/2;
			if (clearance < 0) clearance = 0;
			screen_draw_text(screen, x+clearance, y+APP_TEXT_Y_OFFSET+APP_BOX_FONT_SIZE+3, APP_BOX_FONT_SIZE, buffer);

		}
#endif
	}
}

void ui_main_render(struct Screen *screen)
{
	if (g_is_sleeping) {
		if(screen->mouse_clicked) g_is_sleeping = false;
		return;
	}

	ui_main_draw_header(screen);
	if (g_dialog.instance.is_open) {
		if (g_dialog.first_time_open) {
			screen->mouse_clicked    = false;
			g_dialog.first_time_open = false;
		}
		ui_dialog_render(screen, &g_dialog.instance);
		return;
	}

	if (g_active_app_idx == -1) {
		ui_draw_app_icons(screen);
	}
	else {
		struct App *active_app = &g_apps[g_active_app_idx];

		const int window_y_pos = SCREEN_BORDER_WIDTH+UI_STATUS_BAR_HEIGHT+UI_WINDOW_BORDER;
		struct Ui_Window window = {
			.name = active_app->name,
			.x    = UI_WINDOW_BORDER,
			.y    = window_y_pos,
			.w    = SCREEN_LOGICAL_WIDTH-2*UI_WINDOW_BORDER,
			.h    = SCREEN_LOGICAL_HEIGHT-window_y_pos-UI_WINDOW_BORDER
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
