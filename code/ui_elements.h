#ifndef UI_ELEMENTS_H
#define UI_ELEMENTS_H

#include "screen.h"

enum Ui_Border {
	UI_BORDER_NONE,
	UI_BORDER_NORMAL
};

enum Button_Type {
	BUTTON_TYPE_TEXT,
	BUTTON_TYPE_ICON
};

struct Ui_Button {
	char id[40];
	int x;
	int y;
	int w;
	int h;
	char text[40];
	enum Button_Type type;
	void (*on_click)(struct Ui_Button *btn);
	int font_size;
	enum Ui_Border border;
	bool is_selectable;
	void *user_data; // TODO: remove? unused?
};

struct Ui_Box {
	char id[40];
	int x;
	int y;
	int w;
	int h;
	void (*on_click)(struct Ui_Box *box);
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

struct Ui_Media_Player {
	int x;
	int y;
	int w;
	int h;
	char first_line[40];
	char second_line[40];
	int track_pos_sec;
	int track_len_sec;
	struct {
		struct {
			int x;
			int y;
			int w;
		} progress_bar;
		struct Ui_Button button_prev;
		struct Ui_Button button_rewind;
		struct Ui_Button button_play;
		struct Ui_Button button_forward;
		struct Ui_Button button_next;
	} internal;

};

void ui_button_init(
	struct Screen *screen,
	struct Ui_Button *btn,
	const char *id, int x, int y,
	const char *text,
	void (*on_click)(struct Ui_Button *btn));

void ui_button_init_icon(
	struct Screen *screen,
	struct Ui_Button *btn,
	const char *id, int x, int y, int w,
	const char *icon,
	void (*on_click)(struct Ui_Button *btn));

void ui_button_render(struct Screen *screen, struct Ui_Button *btn);
void ui_box_render(struct Screen *screen, struct Ui_Box *box);

void ui_media_player_init(
	struct Screen *screen,
	struct Ui_Media_Player *player,
	int x, int y, int w, int h);
void ui_media_player_render(struct Screen *screen, struct Ui_Media_Player *player);

void ui_clickable_list_init(struct Screen *screen, struct Ui_Clickable_List *list, int x, int y, int w, size_t items_per_page);
void ui_clickable_list_clear(struct Ui_Clickable_List *list);
void ui_clickable_list_append(struct Screen *screen, struct Ui_Clickable_List *list, const char *text);
void ui_clickable_list_render(struct Screen *screen, struct Ui_Clickable_List *list);

#endif // UI_ELEMENTS_H
