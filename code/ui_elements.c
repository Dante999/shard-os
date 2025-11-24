#include "ui_elements.h"

#include "config.h"
#include <SDL_render.h>

#include "libcutils/logger.h"
#include "libcutils/util_makros.h"

#define UI_BUTTON_BORDER_WIDTH 10

static void on_clickable_list_button_pressed(struct Ui_Button *btn)
{
	log_debug("clickable list, button pressed: id=%s\n", btn->id);
	struct Ui_Clickable_List *list = (struct Ui_Clickable_List*) btn->user_data;

	if (list == NULL) {
		log_error("button has list instance not as user data set!\n");
		return;
	}

	log_debug("on_list_item_clicked (count=%zu)\n", list->internal.count);
	if (strcmp("btn_prev", btn->id) == 0 && list->page_index > 0) {
		--list->page_index;
		return;
	}

	if (strcmp("btn_next", btn->id) == 0) {
		log_debug("TODO: btn_next pressed\n");
		return;
	}

	if (strcmp("btn_index", btn->id) == 0 && list->page_index > 0) return;

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

void ui_button_init(
	struct Screen *screen,
	struct Ui_Button *btn,
	const char *id, int x, int y,
	const char *text,
	void (*on_click)(struct Ui_Button *btn))
{
	btn->font_size = g_config.screen_font_size_s;
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

void ui_button_render(struct Screen *screen, struct Ui_Button *btn)
{
	bool is_selected = false;

	if (btn->is_selectable && IN_RANGE(screen->mouse_x, btn->x, btn->x+btn->w) &&
		IN_RANGE(screen->mouse_y, btn->y, btn->y+btn->h)) {
		is_selected = true;
	}

	screen_draw_box(screen, btn->x, btn->y, btn->w, btn->h, is_selected);
	screen_draw_text(screen, btn->x+UI_BUTTON_BORDER_WIDTH, btn->y+UI_BUTTON_BORDER_WIDTH, btn->font_size, btn->text);

	if (is_selected && screen->mouse_clicked && btn->on_click != NULL) {
		btn->on_click(btn);
	}
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
	ui_button_init(screen, &list->internal.button_prev_page , "btn_prev" , x_center-nav_page_width/2, y_pagination, "prev", on_clickable_list_button_pressed);
	ui_button_init(screen, &list->internal.button_page_index, "btn_index", x_center-page_index_width/2, y_pagination, "0", on_clickable_list_button_pressed);
	ui_button_init(screen, &list->internal.button_next_page , "btn_next" , x_center+nav_page_width/2-page_button_width, y_pagination, "next", on_clickable_list_button_pressed);

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

	struct Ui_Button *btn = &list->internal.items[list->internal.count];

	char buffer[40];
	snprintf(buffer, sizeof(buffer), "%zu", list->internal.count);
	ui_button_init(screen, btn, buffer, list->x, list->y + (int)((list->internal.count-1) * 50), text, on_clickable_list_button_pressed);
	btn->user_data = list;
	btn->w = list->w;

	++list->internal.count;
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
