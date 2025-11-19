#include "ui_elements.h"

#include "config.h"
#include <SDL_render.h>

#include "libcutils/logger.h"
#include "libcutils/util_makros.h"

void ui_button_render(struct Screen *screen, struct Ui_Button *btn)
{
	bool is_selected = false;

	if (IN_RANGE(screen->mouse_x, btn->x, btn->x+btn->w) &&
		IN_RANGE(screen->mouse_y, btn->y, btn->y+btn->h)) {
		is_selected = true;
	}
	screen_draw_text_boxed(screen, btn->x, btn->y, g_config.screen_font_size_s, btn->w, is_selected, btn->text);
}


void ui_clickable_list_init(struct Ui_Clickable_List *list)
{
	list->internal.count = 0;
	strcpy(list->internal.button_prev_page.text, "<-");
	strcpy(list->internal.button_next_page.text, "->");

	int x_center = list->x + (list->w/2);
	log_debug("x_center=%d\n", x_center);
	list->internal.button_prev_page.x = x_center-100;
	list->internal.button_prev_page.y = list->y;
	list->internal.button_prev_page.h = list->h;
	list->internal.button_prev_page.w = 50;
	list->internal.button_next_page.x = x_center+100;
	list->internal.button_next_page.y = list->y;
	list->internal.button_next_page.h = list->h;
	list->internal.button_next_page.w = 50;
}

void ui_clickable_list_clear(struct Ui_Clickable_List *list)
{
	list->internal.count=0;
}

void ui_clickable_list_append(struct Ui_Clickable_List *list, const char *text)
{
	if (list->internal.count >= UI_LIST_MAX_ITEMS) return;

	struct Ui_Button *btn = &list->internal.items[list->internal.count++];

	strncpy(btn->text, text, sizeof(list->internal.items[0].text));
	btn->x = list->x;
	btn->y = list->y + (int)((list->internal.count-1) * 50);
	btn->w = list->w;
	btn->h = list->h;

	list->internal.button_prev_page.y = list->y + (int)(list->internal.count) * 50 + 20;
	list->internal.button_next_page.y = list->y + (int)(list->internal.count) * 50 + 20;
	log_debug("list size: %zu\n", list->internal.count);
}

void ui_clickable_list_render(struct Screen *screen, struct Ui_Clickable_List *list)
{
	//size_t start = list->page_index*list->items_per_page;
	//size_t end   = MIN(start+list->items_per_page, list->internal.count);
	size_t start = 0;
	size_t end   = list->internal.count;
	if (start >= end) start=0;

	for (size_t i=start; i < end; ++i) {
		ui_button_render(screen, &list->internal.items[i]);
	}
	
	struct Ui_Button *btn_prev = &list->internal.button_prev_page;
	struct Ui_Button *btn_next = &list->internal.button_next_page;

	struct Ui_Button page_index = {
		.x = btn_prev->x+(btn_next->x-btn_prev->x)/2, 
		.y = btn_prev->y, 
		.w = btn_prev->w,
		.h = 0
	};
	snprintf(page_index.text, sizeof(page_index.text), "%zu", list->page_index+1);
	
	ui_button_render(screen, btn_prev);
	ui_button_render(screen, &page_index); 
	ui_button_render(screen, btn_next); 


}
