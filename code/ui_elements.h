#ifndef UI_ELEMENTS_H
#define UI_ELEMENTS_H

#include "screen.h"

enum Ui_Event {
	UI_EVENT_NONE,
	UI_EVENT_SELECTED,
	UI_EVENT_CLICKED,
};


////////////////////////////////////////////////////////////////////////////////
 enum Ui_Border {
	UI_BORDER_NONE,
	UI_BORDER_NORMAL
};

struct Ui_Outline {
	int x;
	int y;
	int w;
	int h;
	enum Ui_Border border;
};
bool ui_outline_selected(const struct Screen *screen, const struct Ui_Outline *outline);


////////////////////////////////////////////////////////////////////////////////
enum Button_Type {
	BUTTON_TYPE_TEXT,
	BUTTON_TYPE_ICON
};

struct Ui_Button {
	char text[40];
	struct Ui_Outline outline;
	enum Button_Type type;
	int font_size;
	bool is_selectable;
};

void ui_button_init(
	struct Screen *screen,
	struct Ui_Button *btn,
	const char *text,
	int x, int y);

void ui_button_init_icon(
	struct Screen *screen,
	struct Ui_Button *btn,
	const char *icon,
	int x, int y, int w);

enum Ui_Event ui_button_render(struct Screen *screen, struct Ui_Button *btn);


////////////////////////////////////////////////////////////////////////////////
struct Ui_Box {
	char id[40];
	struct Ui_Outline outline;
	void (*on_click)(struct Ui_Box *box);
	bool is_selectable;
	void *userdata;
};
void ui_box_render(struct Screen *screen, struct Ui_Box *box);


////////////////////////////////////////////////////////////////////////////////
struct Ui_Window {
	const char *name;
	int x;
	int y;
	int w;
	int h;
	bool should_close;
};
void ui_window_render(struct Screen *screen, struct Ui_Window *window);


////////////////////////////////////////////////////////////////////////////////
#define UI_DIALOG_X_START 100
#define UI_DIALOG_Y_START 100
#define UI_DIALOG_X_END   (SCREEN_LOGICAL_WIDTH-UI_DIALOG_X_START)
#define UI_DIALOG_Y_END   (SCREEN_LOGICAL_HEIGHT-UI_DIALOG_Y_START)
struct Ui_Dialog{
	const char *name;
	struct Ui_Outline outline;
	void (*render_content)(struct Screen *screen);
	bool is_open;
};
void ui_dialog_render(struct Screen *screen, struct Ui_Dialog *dialog);


////////////////////////////////////////////////////////////////////////////////
struct Ui_Chooser_Integer {
	const char *name;
	struct Ui_Outline outline;
	int x_value_offset;
	int min_value;
	int max_value;
	int cur_value;
	int steps;
};
void ui_chooser_integer_render(struct Screen *screen, struct Ui_Chooser_Integer *chooser);


////////////////////////////////////////////////////////////////////////////////
#define UI_LIST_MAX_ITEMS    40
#define UI_LIST_MAX_ITEM_LEN 40
struct Ui_Clickable_List {
	struct Ui_Outline attr;
	void (*on_click)(int index);
	struct {
		char items[UI_LIST_MAX_ITEMS][UI_LIST_MAX_ITEM_LEN];
		int index_selected_item;
		size_t count;
		size_t page_index;
		size_t items_per_page;
		struct Ui_Button button_prev_page;
		struct Ui_Button button_page_index;
		struct Ui_Button button_next_page;
	} internal;
};
void ui_clickable_list_init(
	struct Screen *screen,
	struct Ui_Clickable_List *list,
	int x, int y, int w, int h);

void ui_clickable_list_clear(struct Ui_Clickable_List *list);
void ui_clickable_list_append(struct Ui_Clickable_List *list, const char *text);
bool ui_clickable_list_select(struct Ui_Clickable_List *list, int index);
void ui_clickable_list_render(struct Screen *screen, struct Ui_Clickable_List *list);


////////////////////////////////////////////////////////////////////////////////
enum Ui_Media_Button {
	UI_MEDIA_BUTTON_PLAY,
	UI_MEDIA_BUTTON_REW,
	UI_MEDIA_BUTTON_PREV,
	UI_MEDIA_BUTTON_FWD,
	UI_MEDIA_BUTTON_NEXT
};
struct Ui_Media_Player {
	int x;
	int y;
	int w;
	int h;
	char first_line[40];
	char second_line[40];
	char last_line[40];
	int track_pos_sec;
	int track_len_sec;
	bool is_playing;
	void (*on_button_clicked)(enum Ui_Media_Button btn);
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
void ui_media_player_init(
	struct Screen *screen,
	struct Ui_Media_Player *player,
	int x, int y, int w, int h,
	void (*on_button_clicked)(enum Ui_Media_Button btn));

void ui_media_player_render(struct Screen *screen, struct Ui_Media_Player *player);





#endif // UI_ELEMENTS_H
