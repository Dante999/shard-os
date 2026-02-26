#include "ui_main.h"

#include "config.h"
#include "app_radio.h"
#include "app_jukebox.h"
#include "app_dice.h"
#include "ui_elements.h"
#include "ui_audio_settings.h"

#include "libcutils/logger.h"
#include "libcutils/util_makros.h"

#include <assert.h>
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


//struct {
//	struct Ui_Dialog instance;
//	bool is_open;
//	bool first_time_open;
//} g_dialog;




static bool g_is_sleeping = false;

//static void render_audio_settings(struct Screen *screen)
//{
//	static struct Ui_Chooser volume_chooser = {
//		.name = "Volume",
//		.outline = {
//			.x = UI_DIALOG_X_START + SCREEN_BORDER_WIDTH,
//			.y = UI_DIALOG_Y_START + SCREEN_BORDER_WIDTH,
//			.w = (UI_DIALOG_X_END - UI_DIALOG_Y_START - SCREEN_BORDER_WIDTH)/2,
//			.h = 0
//		},
//		.name_width = 100,
//	};
//	ui_chooser_int_init(&volume_chooser,g_config.volume, 0, 100, 10);
//	ui_chooser_render(screen, &volume_chooser);
//
//	if (volume_chooser.data.int_chooser.cur_value != g_config.volume) {
//		g_config.volume = volume_chooser.data.int_chooser.cur_value;
//		audio_set_volume(g_config.volume);
//	}
//
//	static struct Ui_Chooser device_chooser = {
//		.name = "Device",
//		.outline = {
//			.x = UI_DIALOG_X_START + SCREEN_BORDER_WIDTH,
//			.y = UI_DIALOG_Y_START + SCREEN_BORDER_WIDTH + 40,
//			.w = (UI_DIALOG_X_END - UI_DIALOG_Y_START - SCREEN_BORDER_WIDTH)/2,
//			.h = 0
//		},
//		.name_width = 100
//	};
//	ui_chooser_str_init(&device_chooser);
//	ui_chooser_str_append(&device_chooser, "Headset");
//	ui_chooser_str_append(&device_chooser, "HDMI Screen");
//	ui_chooser_str_append(&device_chooser, "FM Sender");
//	ui_chooser_render(screen, &device_chooser);
//}


struct {
	struct Ui_Dialog instance;
	bool is_open;
	bool first_time_open;
} g_dialog;

struct Icon {
	const char *name;
	void (*on_enter)(void);
	void (*on_render)(struct Screen *screen);
	void (*on_exit)(void);
};

static void on_header_icon_close(void);


static void on_enter_sleep(void)
{
	log_info("entering sleep mode\n"),
	g_is_sleeping = true;
	on_header_icon_close();
}

static int g_icons_index = -1;
struct Icon g_icons[] = {
	{.name="[SLEEP]", .on_enter=on_enter_sleep         , .on_render=NULL                    , .on_exit=NULL},
	{.name="[AUDIO]", .on_enter=on_enter_audio_settings, .on_render=on_render_audio_settings, .on_exit=NULL},
};

static void on_header_icon_open(int index)
{
	PRECONDITION(index < (int)ARRAY_SIZE(g_icons));

	g_icons_index = index;
	g_dialog.first_time_open = true;
	g_dialog.is_open = true;
	g_dialog.instance.name = g_icons[index].name;
	g_dialog.instance.render_content = g_icons[index].on_render;

	if (g_icons[index].on_enter != NULL) {
		g_icons[index].on_enter();
	}
	//icon->render_dialog(screen);
}

static void on_header_icon_close(void)
{
	log_debug("closing dialog\n");
	g_dialog.is_open = false;
	
	if (g_icons_index != -1) {
		if (g_icons[g_icons_index].on_exit != NULL) {
			g_icons[g_icons_index].on_exit();
		}
		g_icons_index = -1;
	}
}

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
			on_header_icon_open((int)i);
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
	if (g_dialog.is_open) {
		if (g_dialog.first_time_open) {
			screen->mouse_clicked    = false;
			g_dialog.first_time_open = false;
		}
		if (ui_dialog_render(screen, &g_dialog.instance) == UI_EVENT_EXIT) {
			on_header_icon_close();
		}
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
