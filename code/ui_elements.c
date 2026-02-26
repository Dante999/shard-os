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

bool ui_outline_selected(const struct Screen *screen, const struct Ui_Outline *outline)
{
	return (IN_RANGE((int)screen->mouse_x, outline->x, outline->x+outline->w) &&
		IN_RANGE((int)screen->mouse_y, outline->y, outline->y+outline->h));
}

void ui_button_init_icon(
	struct Screen *screen,
	struct Ui_Button *btn,
	const char *icon, int x, int y, int w)
{
	(void) screen;

	btn->font_size = UI_BUTTON_FONT_SIZE;
	strncpy(btn->text, icon, sizeof(btn->text));

	btn->type           = BUTTON_TYPE_ICON;
	btn->outline.x      = x;
	btn->outline.y      = y;
	btn->outline.w      = w;
	btn->outline.h      = UI_BUTTON_FONT_SIZE+2*UI_BUTTON_BORDER_WIDTH;
	btn->outline.border = UI_BORDER_NORMAL;
	btn->type     = BUTTON_TYPE_ICON;
	btn->is_selectable = true;
}

void ui_button_init(
	struct Screen *screen,
	struct Ui_Button *btn,
	const char *text, int x, int y)
{
	btn->font_size = UI_BUTTON_FONT_SIZE;
	strncpy(btn->text, text, sizeof(btn->text));

	struct Screen_Dimension text_dimensions = screen_get_text_dimension(screen, btn->font_size, btn->text);
	btn->outline.x      = x;
	btn->outline.y      = y;
	btn->outline.w      = text_dimensions.w+2*UI_BUTTON_BORDER_WIDTH;
	btn->outline.h      = text_dimensions.h+2*UI_BUTTON_BORDER_WIDTH;
	btn->outline.border = UI_BORDER_NORMAL;
	btn->type     = BUTTON_TYPE_TEXT;
	btn->is_selectable = true;
}

void ui_box_render(struct Screen *screen, struct Ui_Box *box)
{
	bool is_selected = false;

	if (box->is_selectable && ui_outline_selected(screen, &box->outline)) {
		is_selected = true;
	}

	screen_draw_box(screen, box->outline.x, box->outline.y, box->outline.w, box->outline.h, is_selected);

	if (is_selected && screen->mouse_clicked && box->on_click != NULL) {
		box->on_click(box);
	}
}

void ui_window_render(struct Screen *screen, struct Ui_Window *window)
{

	struct Ui_Button close_btn = {0};
	ui_button_init(screen, &close_btn, "X", window->x+window->w-50, window->y+10);
	close_btn.outline.w = 40;
	close_btn.outline.border = UI_BORDER_NONE;

	screen_draw_window(screen, window->x, window->y,
		window->w, window->h, window->name);

	if (ui_button_render(screen, &close_btn) == UI_EVENT_CLICKED) {
		window->should_close = true;
	}

}

enum Ui_Event ui_button_render(struct Screen *screen, struct Ui_Button *btn)
{
	bool is_selected = false;

	if (btn->is_selectable && ui_outline_selected(screen, &btn->outline)) {
		is_selected = true;
	}

	if (btn->outline.border == UI_BORDER_NORMAL || (btn->outline.border == UI_BORDER_NONE && is_selected)) {
		screen_draw_box(screen, btn->outline.x, btn->outline.y, btn->outline.w, btn->outline.h, is_selected);
	}

	if (btn->type == BUTTON_TYPE_TEXT) {
		screen_draw_text(screen, btn->outline.x+UI_BUTTON_BORDER_WIDTH, btn->outline.y+UI_BUTTON_BORDER_WIDTH, btn->font_size, btn->text);
	}
	else if (btn->type == BUTTON_TYPE_ICON) {
		char path[512];
		snprintf(path, sizeof(path), "%s/icons/%s", g_config.resources_dir, btn->text);
		screen_draw_icon(screen,
			btn->outline.x+UI_BUTTON_BORDER_WIDTH, btn->outline.y+UI_BUTTON_BORDER_WIDTH,
			btn->font_size, btn->font_size, path);
	}

	if (is_selected && screen->mouse_clicked) {
		return UI_EVENT_CLICKED;
	}
	if (is_selected) {
		return UI_EVENT_SELECTED;
	}

	return UI_EVENT_NONE;
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

	ui_button_init(
		screen, &list->internal.button_prev_page , "prev" ,
		x_center-nav_page_width/2, y_pagination);

	ui_button_init(
		screen, &list->internal.button_page_index, "0",
		x_center-page_index_width/2, y_pagination);

	ui_button_init(
		screen, &list->internal.button_next_page , "next" ,
		x_center+nav_page_width/2-page_button_width, y_pagination);

	list->internal.button_prev_page.outline.w  = page_button_width;
	list->internal.button_page_index.outline.w = page_index_width;
	list->internal.button_next_page.outline.w  = page_button_width;
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
		ui_button_init(screen, &btn, list->internal.items[i],
			list->attr.x,
			list->attr.y + (int)(i-start) * UI_CLICKABLE_LIST_ENTRY_FACTOR);

		if ((int)i == list->internal.index_selected_item) {
			screen_draw_box_filled(screen, btn.outline.x, btn.outline.y, list->attr.w, btn.outline.h, SCREEN_COLOR_HIGHLIGHT, SCREEN_COLOR_HIGHLIGHT);
		}
		btn.outline.w = list->attr.w;
		if (ui_button_render(screen, &btn) == UI_EVENT_CLICKED) {
			list->on_click((int)i);
		}
	}

	struct Ui_Button *btn_prev  = &list->internal.button_prev_page;
	struct Ui_Button *btn_index = &list->internal.button_page_index;
	struct Ui_Button *btn_next  = &list->internal.button_next_page;

	snprintf(btn_index->text, sizeof(btn_index->text), "%zu", list->internal.page_index+1);

	if (ui_button_render(screen, btn_prev) == UI_EVENT_CLICKED) {
		log_debug("prev page: currently %zu\n", list->internal.page_index);
		if (list->internal.page_index > 0) {
			--list->internal.page_index;
		}

	}
	if (ui_button_render(screen, btn_next) == UI_EVENT_CLICKED) {
		size_t max_page_index = (list->internal.count-1)/list->internal.items_per_page;
		log_debug("next page: currently %zu of %zu\n", list->internal.page_index, max_page_index);
		if (list->internal.page_index < max_page_index) {
			++list->internal.page_index;
		}
	}
	ui_button_render(screen, btn_index);
}

void ui_media_player_init(
	struct Screen *screen,
	struct Ui_Media_Player *player,
	int x, int y, int w, int h,
	void (*on_button_clicked)(enum Ui_Media_Button btn))
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

	ui_button_init(screen, &player->internal.button_play, "Play",
		x_center-button_width/2,
		y_button);

	ui_button_init(screen, &player->internal.button_rewind, "Rew",
		x_center-button_width/2-button_clearance-button_width,
		y_button);

	ui_button_init(screen, &player->internal.button_prev, "Prev",
		x_center-button_width/2-2*button_clearance-2*button_width,
		y_button);

	ui_button_init(screen, &player->internal.button_forward, "Fwd",
		x_center+button_width/2+button_clearance,
		y_button);

	ui_button_init(screen, &player->internal.button_next, "Next",
		x_center+button_width/2+2*button_clearance+button_width,
		y_button);

	player->internal.button_play.outline.w    = button_width;
	player->internal.button_prev.outline.w    = button_width;
	player->internal.button_next.outline.w    = button_width;
	player->internal.button_rewind.outline.w  = button_width;
	player->internal.button_forward.outline.w = button_width;

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

	if (ui_button_render(screen, &player->internal.button_play) == UI_EVENT_CLICKED) {
		player->on_button_clicked(UI_MEDIA_BUTTON_PLAY);
	}
	if (ui_button_render(screen, &player->internal.button_rewind) == UI_EVENT_CLICKED) {
		player->on_button_clicked(UI_MEDIA_BUTTON_REW);
	}
	if (ui_button_render(screen, &player->internal.button_prev) == UI_EVENT_CLICKED) {
		player->on_button_clicked(UI_MEDIA_BUTTON_PREV);
	}
	if (ui_button_render(screen, &player->internal.button_next) == UI_EVENT_CLICKED) {
		player->on_button_clicked(UI_MEDIA_BUTTON_NEXT);
	}
	if (ui_button_render(screen, &player->internal.button_forward) == UI_EVENT_CLICKED) {
		player->on_button_clicked(UI_MEDIA_BUTTON_FWD);
	}

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

enum Ui_Event ui_dialog_render(struct Screen *screen, struct Ui_Dialog *dialog)
{
	PRECONDITION(screen != NULL);
	PRECONDITION(dialog != NULL);

	static struct Ui_Box box = {
		.id="dialog",
		.outline = {
			.x = UI_DIALOG_X_START,
			.y = UI_DIALOG_Y_START,
			.w = UI_DIALOG_X_END-UI_DIALOG_X_START,
			.h = UI_DIALOG_Y_END-UI_DIALOG_Y_START
		},
		.on_click = NULL,
		.is_selectable = false
	};
	static struct Ui_Button close_btn = {0};
	ui_button_init(screen, &close_btn, "CLOSE", UI_DIALOG_X_END-50, UI_DIALOG_Y_END-50);
	close_btn.outline.x -= close_btn.outline.w-UI_BUTTON_HEIGHT;

	if (ui_button_render(screen, &close_btn) == UI_EVENT_CLICKED) {
		return UI_EVENT_EXIT;
	}


	ui_box_render(screen, &box);
	if (dialog->render_content != NULL) {
		dialog->render_content(screen);
	}
	return UI_EVENT_NONE;
}

#define UI_CHOOSER_BUTTON_WIDTH 40
void ui_chooser_int_init(struct Ui_Chooser *chooser, int cur_value, int min_value, int max_value, int steps)
{
	chooser->type = UI_CHOOSER_TYPE_INT;
	chooser->data.int_chooser.cur_value = cur_value;
	chooser->data.int_chooser.min_value = min_value;
	chooser->data.int_chooser.max_value = max_value;
	chooser->data.int_chooser.steps     = steps;
}
void ui_chooser_str_init(struct Ui_Chooser *chooser)
{
	chooser->type = UI_CHOOSER_TYPE_STR;
	chooser->data.str_chooser.selected = 0;
	chooser->data.str_chooser.count    = 0;
}

enum Ui_Event ui_chooser_render(struct Screen *screen, struct Ui_Chooser *chooser)
{
	struct Ui_Button btn_decrease;
	ui_button_init(
		screen,
		&btn_decrease,
		"<-",
		chooser->outline.x + chooser->name_width,
		chooser->outline.y);

	struct Ui_Button btn_increase;
	ui_button_init(
		screen,
		&btn_increase,
		"->",
		chooser->outline.x+chooser->outline.w-UI_CHOOSER_BUTTON_WIDTH,
		chooser->outline.y);

	btn_decrease.outline.w = UI_CHOOSER_BUTTON_WIDTH;
	btn_increase.outline.w = UI_CHOOSER_BUTTON_WIDTH;

	screen_draw_text(screen, chooser->outline.x,
		chooser->outline.y+UI_BUTTON_BORDER_WIDTH,
		UI_BUTTON_FONT_SIZE, chooser->name);

	char val_buffer[255];

	enum Ui_Event ret = UI_EVENT_NONE;

	// NOTE: always render all buttons, even if one is already clicked otherwise
	// the oposite button will appear flickering

	if (chooser->type == UI_CHOOSER_TYPE_INT) {
		struct Ui_Chooser_Int *int_chooser = &chooser->data.int_chooser;
		snprintf(val_buffer, sizeof(val_buffer), "%d", int_chooser->cur_value);
		screen_draw_text(
			screen,btn_decrease.outline.x+20+
			(btn_increase.outline.x-
			(btn_decrease.outline.x+btn_decrease.outline.w))/2,
			chooser->outline.y+UI_BUTTON_BORDER_WIDTH,
			UI_BUTTON_FONT_SIZE,
			val_buffer);


		if (ui_button_render(screen, &btn_increase) == UI_EVENT_CLICKED) {
			if (int_chooser->cur_value+int_chooser->steps <= int_chooser->max_value) {
				int_chooser->cur_value += int_chooser->steps;
				ret = UI_EVENT_MODIFIED;
			}
		}
		if (ui_button_render(screen, &btn_decrease) == UI_EVENT_CLICKED) {
			if (int_chooser->cur_value-int_chooser->steps >= int_chooser->min_value) {
				int_chooser->cur_value -= int_chooser->steps;
				ret = UI_EVENT_MODIFIED;
			}
		}
	}
	else if (chooser->type == UI_CHOOSER_TYPE_STR) {
		struct Ui_Chooser_Str *str_chooser = &chooser->data.str_chooser;

		strncpy(val_buffer, str_chooser->items[str_chooser->selected], 20);
		screen_draw_text(
				screen,btn_decrease.outline.x+20+
				(btn_increase.outline.x-
				 (btn_decrease.outline.x+btn_decrease.outline.w))/2,
				chooser->outline.y+UI_BUTTON_BORDER_WIDTH,
				UI_BUTTON_FONT_SIZE,
				val_buffer);


		if (ui_button_render(screen, &btn_increase) == UI_EVENT_CLICKED) {
			if (str_chooser->selected < str_chooser->count-1) {
				++str_chooser->selected;
				ret = UI_EVENT_MODIFIED;
			}
		}
		if (ui_button_render(screen, &btn_decrease) == UI_EVENT_CLICKED) {
			if (str_chooser->selected > 0) {
				--str_chooser->selected;
				ret = UI_EVENT_MODIFIED;
			}
		}
	}
	else {
		assert(false && "unhandled chooser type!");
	}

	return ret;
}


//void ui_chooser_string_render(struct Screen *screen, struct Ui_Chooser_String *chooser)
//{
//	struct Ui_Button btn_decrease;
//	ui_button_init(
//		screen,
//		&btn_decrease,
//		"<-",
//		chooser->outline.x + chooser->x_value_offset,
//		chooser->outline.y);
//
//	struct Ui_Button btn_increase;
//	ui_button_init(
//		screen,
//		&btn_increase,
//		"->",
//		chooser->outline.x+chooser->outline.w-UI_CHOOSER_BUTTON_WIDTH,
//		chooser->outline.y);
//
//	btn_decrease.outline.w = UI_CHOOSER_BUTTON_WIDTH;
//	btn_increase.outline.w = UI_CHOOSER_BUTTON_WIDTH;
//
//	screen_draw_text(screen, chooser->outline.x,
//		chooser->outline.y+UI_BUTTON_BORDER_WIDTH,
//		UI_BUTTON_FONT_SIZE, chooser->name);
//
//	char val_buffer[255];
//	strncpy(val_buffer, chooser->items[chooser->selected], 20);
//	screen_draw_text(
//		screen,btn_decrease.outline.x+20+
//		(btn_increase.outline.x-
//		(btn_decrease.outline.x+btn_decrease.outline.w))/2,
//		chooser->outline.y+UI_BUTTON_BORDER_WIDTH,
//		UI_BUTTON_FONT_SIZE,
//		val_buffer);
//
//
//	if (ui_button_render(screen, &btn_increase) == UI_EVENT_CLICKED) {
//		if (chooser->selected < chooser->count-1) {
//			++chooser->selected;
//		}
//	}
//	if (ui_button_render(screen, &btn_decrease) == UI_EVENT_CLICKED) {
//		if (chooser-> selected > 0) {
//			--chooser->selected;
//		}
//	}
//}


Result ui_chooser_str_append(struct Ui_Chooser *chooser, const char *s)
{
	struct Ui_Chooser_Str *str_chooser = &chooser->data.str_chooser;
	if (str_chooser->count < UI_CHOOSER_STRING_MAX_ITEMS) {
		strncpy(str_chooser->items[str_chooser->count++], s, UI_CHOOSER_STRING_MAX_ITEM_LEN);
		return result_make_success();
	}

	return result_make(false, "string chooser max items reachted!");
}

void ui_chooser_str_clear(struct Ui_Chooser *chooser)
{
	chooser->data.str_chooser.selected = 0;
	chooser->data.str_chooser.count    = 0;
}
