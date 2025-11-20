#include "ui_elements.h"

#include "config.h"
#include <SDL_render.h>

#include "libcutils/logger.h"
#include "libcutils/util_makros.h"

#define UI_BUTTON_BORDER_WIDTH 10

void ui_button_init(struct Screen *screen, struct Ui_Button *btn, int x, int y, const char *text)
{
	btn->font_size = g_config.screen_font_size_s;
	strncpy(btn->text, text, sizeof(btn->text));

	struct Screen_Dimension text_dimensions = screen_get_text_dimension(screen, btn->font_size, btn->text);
	btn->x      = x;
	btn->y      = y;
	btn->w      = text_dimensions.w+2*UI_BUTTON_BORDER_WIDTH;
	btn->h      = text_dimensions.h+2*UI_BUTTON_BORDER_WIDTH;
	btn->border = UI_BORDER_NORMAL;
	btn->is_selectable = true;
}

void ui_button_render(struct Screen *screen, struct Ui_Button *btn)
{
	bool is_selected = false;

	if (btn->is_selectable && IN_RANGE(screen->mouse_x, btn->x, btn->x+btn->w) &&
		IN_RANGE(screen->mouse_y, btn->y, btn->y+btn->h)) {
		is_selected = true;
	}

	screen_draw_box(screen, btn->x, btn->y, btn->w, btn->h, is_selected);
	screen_draw_text(screen, btn->x+UI_BUTTON_BORDER_WIDTH, btn->y+UI_BUTTON_BORDER_WIDTH, btn->font_size, btn->text);
}

void ui_clickable_list_init(struct Screen *screen, struct Ui_Clickable_List *list, int x, int y, int w, size_t items_per_page)
{
	list->x              = x;
	list->y              = y;
	list->w              = w;
	list->items_per_page = items_per_page;
	list->internal.count = 0;


	const int x_center     = list->x + (list->w/2);
	const int y_pagination = list->y + (int)(list->items_per_page)*50+20;

	const int page_clearance    = 30;
	const int page_button_width = 70;
	const int page_index_width  = 50;
	const int nav_page_width    = page_button_width*2 + page_index_width + 2*page_clearance;
	ui_button_init(screen, &list->internal.button_prev_page , x_center-nav_page_width/2, y_pagination, "prev");
	ui_button_init(screen, &list->internal.button_page_index, x_center-page_index_width/2, y_pagination, "0");
	ui_button_init(screen, &list->internal.button_next_page , x_center+nav_page_width/2-page_button_width, y_pagination, "next");

	list->internal.button_prev_page.w  = page_button_width;
	list->internal.button_page_index.w = page_index_width;
	list->internal.button_next_page.w  = page_button_width;
	list->internal.button_page_index.is_selectable = false;
}

void ui_clickable_list_clear(struct Ui_Clickable_List *list)
{
	list->internal.count=0;
}

void ui_clickable_list_append(struct Screen *screen, struct Ui_Clickable_List *list, const char *text)
{
	if (list->internal.count >= UI_LIST_MAX_ITEMS) return;

	struct Ui_Button *btn = &list->internal.items[list->internal.count++];

	ui_button_init(screen, btn, list->x, list->y + (int)((list->internal.count-1) * 50), text);
	btn->w = list->w;

	log_debug("list size: %zu\n", list->internal.count);
}

void ui_clickable_list_render(struct Screen *screen, struct Ui_Clickable_List *list)
{
	size_t start = list->page_index * list->items_per_page;
	size_t end   = MIN(start+list->items_per_page, list->internal.count);
	if (start >= end) start=0;

	// TODO: render relative positions depending on page_index instead of 
	// static ones assigned during ui_clickable_list_append
	for (size_t i=start; i < end; ++i) {
		ui_button_render(screen, &list->internal.items[i]);
	}

	struct Ui_Button *btn_prev  = &list->internal.button_prev_page;
	struct Ui_Button *btn_index = &list->internal.button_page_index;
	struct Ui_Button *btn_next  = &list->internal.button_next_page;

	snprintf(btn_index->text, sizeof(btn_index->text), "%zu", list->page_index+1);

	ui_button_render(screen, btn_prev);
	ui_button_render(screen, btn_index);
	ui_button_render(screen, btn_next);
}
