#ifndef SCREEN_H
#define SCREEN_H

#include "libcutils/result.h"

#include <SDL.h>
#include <SDL_ttf.h>

#define SCREEN_FPS            60

#define SCREEN_BORDER_WIDTH   10
#define SCREEN_LOGICAL_WIDTH  1024
#define SCREEN_LOGICAL_HEIGHT 600

#define MAX_OPTION_VALUE_COUNT  5
#define MAX_OPTION_VALUE_LEN    255


struct Screen {
	SDL_Window     *window;
	SDL_Renderer   *renderer;
	TTF_Font       *font;
	int            mouse_x;
	int            mouse_y;
	bool           mouse_clicked;
	uint32_t       ticks;
	bool           quit;
};

struct Screen_Dimension {
	int w;
	int h;
};

enum Screen_Color {
	SCREEN_COLOR_PRIMARY,
	SCREEN_COLOR_HIGHLIGHT,
	SCREEN_COLOR_BACKGROUND
};


Result screen_init(struct Screen *screen, int width, int height);
void screen_destroy(struct Screen *screen);
void screen_rendering_start(struct Screen *screen);
void screen_rendering_stop(struct Screen *screen);

struct Screen_Dimension screen_get_text_dimension(struct Screen *screen, int font_size, const char *fmt, ...);

void screen_draw_line(struct Screen *screen, int x0, int y0, int x1, int y1);

void screen_set_color(struct Screen *screen, enum Screen_Color color);
void screen_draw_icon(struct Screen *screen, int x, int y, int width, int height, const char *name);
void screen_draw_window(struct Screen *screen, int x, int y, int width, int height, const char *name);
void screen_draw_text(struct Screen *screen, int x, int y, int font_size, const char *fmt, ...);
void screen_draw_text_boxed(struct Screen *screen, int x, int y, int font_size, int min_width, bool is_selected, const char *fmt, ...);
void screen_draw_box(struct Screen *screen, int x, int y, int width, int height, bool is_selected);
void screen_draw_box_filled(struct Screen *screen, int x, int y, int width, int height, enum Screen_Color color);


#endif // SCREEN_H
