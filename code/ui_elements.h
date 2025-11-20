#ifndef UI_ELEMENTS_H
#define UI_ELEMENTS_H

#include "screen.h"

enum Ui_Border {
	UI_BORDER_NONE,
	UI_BORDER_NORMAL
};

struct Ui_Button {
	int x;
	int y;
	int w;
	int h;
	char text[40];
	void (*on_click)(void *data);
	int font_size;
	enum Ui_Border border;
	bool is_selectable;
};


# define UI_LIST_MAX_ITEMS  40

struct Ui_Clickable_List {
	int x;
	int y;
	int w;
	int h;
	size_t items_per_page;
	size_t page_index;
	void (*on_click)(int index);
	struct {
		struct Ui_Button items[UI_LIST_MAX_ITEMS];
		size_t count;
		struct Ui_Button button_prev_page;
		struct Ui_Button button_page_index;
		struct Ui_Button button_next_page;
	} internal;
};

void ui_button_init(struct Screen *screen, struct Ui_Button *btn, int x, int y, const char *text);
void ui_button_render(struct Screen *screen, struct Ui_Button *btn);
void ui_clickable_list_init(struct Screen *screen, struct Ui_Clickable_List *list, int x, int y, int w, size_t items_per_page);
void ui_clickable_list_clear(struct Ui_Clickable_List *list);
void ui_clickable_list_append(struct Screen *screen, struct Ui_Clickable_List *list, const char *text);
void ui_clickable_list_render(struct Screen *screen, struct Ui_Clickable_List *list);

#endif // UI_ELEMENTS_H
