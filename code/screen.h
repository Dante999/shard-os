#ifndef SCREEN_H
#define SCREEN_H

#include "libcutils/result.h"

#include <SDL.h>
#include <SDL_ttf.h>

#define SCREEN_FPS            30

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
	uint32_t       ticks;
	bool           quit;
};

enum Screen_Color {
	SCREEN_COLOR_FONT,
	SCREEN_COLOR_HIGHLIGHT
};


Result screen_init(struct Screen *screen, int width, int height);
void screen_destroy(struct Screen *screen);
void screen_rendering_start(struct Screen *screen);
void screen_rendering_stop(struct Screen *screen);

void screen_set_color(struct Screen *screen, enum Screen_Color color);
void screen_draw_text(struct Screen *screen, int x, int y, int font_size, const char *fmt, ...);
void screen_draw_text_boxed(struct Screen *screen, int x, int y, int font_size, int min_width, bool is_selected, const char *fmt, ...);
void screen_draw_box(struct Screen *screen, int x, int y, int width, int height, bool is_selected);


#endif // SCREEN_H
