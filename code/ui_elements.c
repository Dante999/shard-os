#include "ui_elements.h"

#include "config.h"
#include <SDL3/SDL_render.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "libcutils/logger.h"
#include "libcutils/util_makros.h"

#define UI_BUTTON_BORDER_WIDTH 7
#define UI_BUTTON_FONT_SIZE    g_config.screen_font_size_s
#define UI_BUTTON_HEIGHT       (UI_BUTTON_FONT_SIZE+2*UI_BUTTON_BORDER_WIDTH)
#define UI_CLICKABLE_LIST_ENTRY_FACTOR (UI_BUTTON_FONT_SIZE+2*UI_BUTTON_BORDER_WIDTH+10)
#define UI_MEDIA_PLAYER_CLEARANCE 10
#define UI_CLICKABLE_LIST_PAGINATION_CLEARANCE 10

struct Audio_Len {
	int h;
	int m;
	int s;
};

struct Audio_Len seconds_to_len(int seconds){
	struct Audio_Len len = {0};

	len.h = seconds / 3600;
	len.m = (seconds - (len.h*3600)) / 60;
	len.s = seconds % 60;

	return len;
}

static void on_mediaplayer_button_pressed(struct Ui_Button *btn)
{
	log_debug("on_mediaplayer_button_pressed: %s\n", btn->id);
	assert(btn->user_data != NULL);

	struct Ui_Media_Player *ptr = (struct Ui_Media_Player*) btn->user_data;

	if (ptr->on_button_clicked != NULL) {
		ptr->on_button_clicked(btn->id);
	}
	else {
		log_debug("no callback on mediaplayer set\n");
	}
}

static void on_clickable_list_button_pressed(struct Ui_Button *btn)
{
	log_debug("clickable list, button pressed: id=%s\n", btn->id);
	struct Ui_Clickable_List *list = (struct Ui_Clickable_List*) btn->user_data;

	if (list == NULL) {
		log_error("button has list instance not as user data set!\n");
		return;
	}

	log_debug("on_list_item_clicked (count=%zu)\n", list->internal.count);
	if (strcmp("btn_prev", btn->id) == 0 ) {
		if (list->internal.page_index > 0) {
			--list->internal.page_index;
		}
		return;
	}

	if (strcmp("btn_next", btn->id) == 0) {
		size_t max_page_index = list->internal.count/list->internal.items_per_page;
		log_debug("next page: currently %zu of %zu\n", list->internal.page_index, max_page_index);
		if (list->internal.page_index < max_page_index) {
			++list->internal.page_index;
		}
		return;
	}

	if (strcmp("btn_index", btn->id) == 0 && list->internal.page_index > 0) return;

	char *end;
	long val = strtol(btn->id, &end, 10); // base 10

	if (btn->id == end) {
		log_error("Failed to parse button id as integer!\n");
		return;
	}

	if (list->on_click != NULL) {
		log_info("invoking callback\n");
		list->on_click((int)val);
	}
	else {
		log_warning("no callback for button set\n");
	}
}

void ui_button_init_icon(
	struct Screen *screen,
	struct Ui_Button *btn,
	const char *id, int x, int y, int w,
	const char *icon,
	void (*on_click)(struct Ui_Button *btn))
{
	(void) screen;

	btn->font_size = UI_BUTTON_FONT_SIZE;
	strncpy(btn->text, icon, sizeof(btn->text));
	strncpy(btn->id, id, sizeof(btn->text));

	btn->type     = BUTTON_TYPE_ICON;
	btn->x        = x;
	btn->y        = y;
	btn->w        = w;
	btn->h        = UI_BUTTON_FONT_SIZE+2*UI_BUTTON_BORDER_WIDTH;
	btn->border   = UI_BORDER_NORMAL;
	btn->on_click = on_click;
	btn->is_selectable = true;
}

void ui_button_init(
	struct Screen *screen,
	struct Ui_Button *btn,
	const char *id, int x, int y,
	const char *text,
	void (*on_click)(struct Ui_Button *btn))
{
	btn->font_size = UI_BUTTON_FONT_SIZE;
	strncpy(btn->text, text, sizeof(btn->text));
	strncpy(btn->id, id, sizeof(btn->text));

	struct Screen_Dimension text_dimensions = screen_get_text_dimension(screen, btn->font_size, btn->text);
	btn->x        = x;
	btn->y        = y;
	btn->w        = text_dimensions.w+2*UI_BUTTON_BORDER_WIDTH;
	btn->h        = text_dimensions.h+2*UI_BUTTON_BORDER_WIDTH;
	btn->border   = UI_BORDER_NORMAL;
	btn->on_click = on_click;
	btn->is_selectable = true;
}

void ui_box_render(struct Screen *screen, struct Ui_Box *box)
{
	bool is_selected = false;

	if (box->is_selectable && IN_RANGE(screen->mouse_x, box->x, box->x+box->w) &&
		IN_RANGE(screen->mouse_y, box->y, box->y+box->h)) {
		is_selected = true;
	}

	screen_draw_box(screen, box->x, box->y, box->w, box->h, is_selected);

	if (is_selected && screen->mouse_clicked && box->on_click != NULL) {
		box->on_click(box);
	}
}

static void on_ui_window_close(struct Ui_Button *btn)
{
	struct Ui_Window *window = (struct Ui_Window*) btn->user_data;
	window->should_close = true;
}

void ui_window_render(struct Screen *screen, struct Ui_Window *window)
{

	struct Ui_Button close_btn = {0};
	ui_button_init(screen, &close_btn, "close", window->x+window->w-50, window->y+10, "X", on_ui_window_close);
	close_btn.w         = 40;
	close_btn.on_click  = on_ui_window_close;
	close_btn.user_data = window;
	close_btn.border    = UI_BORDER_NONE;

	screen_draw_window(screen, window->x, window->y,
			window->w, window->h, window->name);

	ui_button_render(screen, &close_btn);

}

void ui_button_render(struct Screen *screen, struct Ui_Button *btn)
{
	bool is_selected = false;

	if (btn->is_selectable && IN_RANGE(screen->mouse_x, btn->x, btn->x+btn->w) &&
		IN_RANGE(screen->mouse_y, btn->y, btn->y+btn->h)) {
		is_selected = true;
	}

	if (btn->border == UI_BORDER_NORMAL || (btn->border == UI_BORDER_NONE && is_selected)) {
		screen_draw_box(screen, btn->x, btn->y, btn->w, btn->h, is_selected);
	}

	if (btn->type == BUTTON_TYPE_TEXT) {
		screen_draw_text(screen, btn->x+UI_BUTTON_BORDER_WIDTH, btn->y+UI_BUTTON_BORDER_WIDTH, btn->font_size, btn->text);
	}
	else if (btn->type == BUTTON_TYPE_ICON) {
		char path[512];
		snprintf(path, sizeof(path), "%s/icons/%s", g_config.resources_dir, btn->text);
		screen_draw_icon(screen,
			btn->x+UI_BUTTON_BORDER_WIDTH, btn->y+UI_BUTTON_BORDER_WIDTH,
			btn->font_size, btn->font_size, path);
	}

	if (is_selected && screen->mouse_clicked && btn->on_click != NULL) {
		btn->on_click(btn);
	}
}

void ui_clickable_list_init(struct Screen *screen,
	struct Ui_Clickable_List *list,
	int x, int y, int w, int h)
{
	list->attr.x = x;
	list->attr.y = y;
	list->attr.w = w;
	list->attr.h = h;
	list->attr.border = UI_BORDER_NONE;
	list->internal.count = 0;
	list->internal.index_selected_item = -1;

	const int x_center     = list->attr.x + (list->attr.w/2);
	const int y_pagination = (list->attr.y+list->attr.h)-(UI_BUTTON_HEIGHT+UI_CLICKABLE_LIST_PAGINATION_CLEARANCE);

	list->internal.items_per_page = (size_t)((y_pagination-list->attr.y) / UI_CLICKABLE_LIST_ENTRY_FACTOR);

	const int page_clearance    = UI_CLICKABLE_LIST_PAGINATION_CLEARANCE;
	const int page_button_width = 70;
	const int page_index_width  = 50;
	const int nav_page_width    = page_button_width*2 + page_index_width + 2*page_clearance;
#if 1
	ui_button_init(
		screen, &list->internal.button_prev_page , "btn_prev" ,
		x_center-nav_page_width/2, y_pagination, "prev",
		on_clickable_list_button_pressed);
	list->internal.button_prev_page.user_data = list;
#else
	ui_button_init_icon(
		screen, &list->internal.button_prev_page, "btn_prev",
		x_center-nav_page_width/2, y_pagination, page_button_width,
		"arrow-left-64x64.png",
		on_clickable_list_button_pressed);
#endif

	ui_button_init(
		screen, &list->internal.button_page_index, "btn_index",
		x_center-page_index_width/2, y_pagination, "0",
		on_clickable_list_button_pressed);

	ui_button_init(
		screen, &list->internal.button_next_page , "btn_next" ,
		x_center+nav_page_width/2-page_button_width, y_pagination, "next",
		on_clickable_list_button_pressed);
	list->internal.button_next_page.user_data = list;

	list->internal.button_prev_page.w  = page_button_width;
	list->internal.button_page_index.w = page_index_width;
	list->internal.button_next_page.w  = page_button_width;
	list->internal.button_page_index.is_selectable = false;

	log_info("clickable list created, items_per_page: %zu\n", list->internal.items_per_page);
}

void ui_clickable_list_clear(struct Ui_Clickable_List *list)
{
	list->internal.count=0;
}

void ui_clickable_list_append(struct Ui_Clickable_List *list, const char *text)
{
	if (list->internal.count >= UI_LIST_MAX_ITEMS) return;

	strncpy(
		list->internal.items[list->internal.count++],
		text,
		UI_LIST_MAX_ITEM_LEN);
}

bool ui_clickable_list_select(struct Ui_Clickable_List *list, int index)
{
	if (index < 0) {
		index = -1;
		return true;
	}

	if (index >= (int) list->internal.count) {
		return false;
	}

	list->internal.index_selected_item = index;
	list->internal.page_index = (size_t)index / list->internal.items_per_page;
	return true;
}

void ui_clickable_list_render(struct Screen *screen, struct Ui_Clickable_List *list)
{
	if (list->attr.border == UI_BORDER_NORMAL) {
		screen_draw_box(screen, list->attr.x, list->attr.y, list->attr.w, list->attr.h, false);
	}

	size_t start = list->internal.page_index * list->internal.items_per_page;
	size_t end   = MIN(start+list->internal.items_per_page, list->internal.count);
	if (start >= end) start=0;

	// TODO: render relative positions depending on page_index instead of
	// static ones assigned during ui_clickable_list_append
	struct Ui_Button btn;

	for (size_t i=start; i < end; ++i) {
		char btn_id[25];
		snprintf(btn_id, sizeof(btn_id), "%zu", i);
		ui_button_init(screen, &btn, btn_id,
			list->attr.x,
			list->attr.y + (int)(i-start) * UI_CLICKABLE_LIST_ENTRY_FACTOR,
			list->internal.items[i],
			on_clickable_list_button_pressed);

		if ((int)i == list->internal.index_selected_item) {
			screen_draw_box_filled(screen, btn.x, btn.y, list->attr.w, btn.h, SCREEN_COLOR_HIGHLIGHT, SCREEN_COLOR_HIGHLIGHT);
		}
		btn.user_data = list;
		btn.w = list->attr.w;
		ui_button_render(screen, &btn);
	}

	struct Ui_Button *btn_prev  = &list->internal.button_prev_page;
	struct Ui_Button *btn_index = &list->internal.button_page_index;
	struct Ui_Button *btn_next  = &list->internal.button_next_page;

	snprintf(btn_index->text, sizeof(btn_index->text), "%zu", list->internal.page_index+1);

	ui_button_render(screen, btn_prev);
	ui_button_render(screen, btn_index);
	ui_button_render(screen, btn_next);
}

void ui_media_player_init(
	struct Screen *screen,
	struct Ui_Media_Player *player,
	int x, int y, int w, int h,
	void (*on_button_clicked)(const char *id))
{

	player->is_playing = false;
	player->x = x;
	player->y = y;
	player->w = w;
	player->h = h;
	player->on_button_clicked = on_button_clicked;

	const int x_center     = x + (w/2);
	const int button_width = 70;
	const int y_button     = y+h-UI_BUTTON_BORDER_WIDTH*2-UI_BUTTON_FONT_SIZE-UI_MEDIA_PLAYER_CLEARANCE;
	const int y_progress   = y_button-40;
	const int button_clearance = 20;

	ui_button_init(screen, &player->internal.button_play, UI_MEDIA_PLAYER_PLAY_BUTTON_ID,
		x_center-button_width/2,
		y_button, "Play", on_mediaplayer_button_pressed);

	ui_button_init(screen, &player->internal.button_rewind, UI_MEDIA_PLAYER_REW_BUTTON_ID,
		x_center-button_width/2-button_clearance-button_width,
		y_button, "Rew", on_mediaplayer_button_pressed);

	ui_button_init(screen, &player->internal.button_prev, UI_MEDIA_PLAYER_PREV_BUTTON_ID,
		x_center-button_width/2-2*button_clearance-2*button_width,
		y_button, "Prev", on_mediaplayer_button_pressed);

	ui_button_init(screen, &player->internal.button_forward,UI_MEDIA_PLAYER_FWD_BUTTON_ID,
		x_center+button_width/2+button_clearance,
		y_button, "Fwd", on_mediaplayer_button_pressed);

	ui_button_init(screen, &player->internal.button_next, UI_MEDIA_PLAYER_NEXT_BUTTON_ID,
		x_center+button_width/2+2*button_clearance+button_width,
		y_button, "Next", on_mediaplayer_button_pressed);

	player->internal.button_play.w            = button_width;
	player->internal.button_play.user_data    = player;
	player->internal.button_prev.w            = button_width;
	player->internal.button_prev.user_data    = player;
	player->internal.button_next.w            = button_width;
	player->internal.button_next.user_data    = player;
	player->internal.button_rewind.w          = button_width;
	player->internal.button_rewind.user_data  = player;
	player->internal.button_forward.w         = button_width;
	player->internal.button_forward.user_data = player;

	player->internal.progress_bar.x = x+UI_MEDIA_PLAYER_CLEARANCE;
	player->internal.progress_bar.y = y_progress;
	player->internal.progress_bar.w = w-2*UI_MEDIA_PLAYER_CLEARANCE;

}


void ui_media_player_render(struct Screen *screen, struct Ui_Media_Player *player)
{
	screen_draw_box(screen, player->x, player->y, player->w, player->h, false);
	const int font_size_first_line  = g_config.screen_font_size_m;
	const int font_size_second_line = g_config.screen_font_size_s;
	const int font_size_last_line   = g_config.screen_font_size_xs;

	screen_draw_text(screen,
			player->x + 40,
			player->y + 30,
			font_size_first_line,
			player->first_line);

	screen_draw_text(screen,
			player->x + 40,
			player->y + 30 + font_size_first_line + 10,
			font_size_second_line,
			player->second_line);

	screen_draw_text(screen,
			player->x + 40,
			player->y + 30 + font_size_first_line + font_size_second_line + 100,
			font_size_last_line,
			player->last_line);

	if (player->is_playing) {
		strncpy(player->internal.button_play.text, "Stop", sizeof(player->internal.button_play.text));
	}
	else {
		strncpy(player->internal.button_play.text, "Play", sizeof(player->internal.button_play.text));
	}
	ui_button_render(screen, &player->internal.button_play);
	ui_button_render(screen, &player->internal.button_rewind);
	ui_button_render(screen, &player->internal.button_prev);
	ui_button_render(screen, &player->internal.button_next);
	ui_button_render(screen, &player->internal.button_forward);

	struct Audio_Len track_pos = seconds_to_len(player->track_pos_sec);
	struct Audio_Len track_len = seconds_to_len(player->track_len_sec);

	char buffer[20];

#if 0
	char fmt[50];

	if (track_len.h == 0) strcpy(fmt, "%02d:%02d");
	else                  strcpy(fmt, "%02d:%02d:%02d");


	if (track_pos.h == 0) {
		snprintf(buffer, sizeof(buffer), "%02d:%02d", track_pos.m, track_pos.s);
	}
	else {
		snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", track_pos.h, track_pos.m, track_pos.s);
	}
	screen_get_text_dimension(
#else
	snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", track_pos.h, track_pos.m, track_pos.s);
#endif

	screen_draw_text(
		screen,
		player->internal.progress_bar.x,
		player->internal.progress_bar.y,
		g_config.screen_font_size_xs,
		buffer);

	snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", track_len.h, track_len.m, track_len.s);
	screen_draw_text(
		screen,
		player->internal.progress_bar.x+player->internal.progress_bar.w-80,
		player->internal.progress_bar.y,
		g_config.screen_font_size_xs,
		buffer);

	{ // progress bar
		int x_start = player->internal.progress_bar.x;
		int x_end   = player->internal.progress_bar.x+player->internal.progress_bar.w;
		int y       = player->internal.progress_bar.y-20;

		screen_draw_line( screen, x_start, y, x_end, y);

		float progress = 0.0f;

		if (player->track_len_sec > 0) {
			progress = (float)(player->track_pos_sec*100)/(float)player->track_len_sec;

			// when optimized with line above, result is always 0.0000
			progress /= 100;
		}
		if (progress > 1.0f) progress = 0.9f;
		if (progress < 0.0f) progress = 0.1f;
		const int slider_w = 50;
		const int slider_h = 20;

		// TODO: handle boundaries like min/max pos
		screen_draw_box_filled(
			screen,
			x_start + (int)((float)player->internal.progress_bar.w*progress),
			y-slider_h/2,
			slider_w,
			slider_h,
			SCREEN_COLOR_PRIMARY,
			SCREEN_COLOR_PRIMARY);
	}

}
