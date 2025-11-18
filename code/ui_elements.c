#include "ui_elements.h"

#include "config.h"
#include <SDL_render.h>

#include "libcutils/util_makros.h"

void ui_button_render(struct Screen *screen, struct Ui_Button *btn)
{
	bool is_selected = false;

	if (IN_RANGE(screen->mouse_x, btn->x, btn->x+btn->w) &&
		IN_RANGE(screen->mouse_y, btn->y, btn->y+btn->h)) {
		is_selected = true;
	}
	screen_draw_text_boxed(screen, btn->x, btn->y, g_config.screen_font_size_m, btn->w, is_selected, btn->text);
}
