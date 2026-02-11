#include "screen.h"

#include "config.h"

#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "libcutils/logger.h"
#include "libcutils/util_makros.h"

#define STATUS_BOX_WIDTH    (SCREEN_LOGICAL_WIDTH-(2*SCREEN_BORDER_WIDTH))
#define HEADER_BOX_WIDTH    (SCREEN_LOGICAL_WIDTH-(2*SCREEN_BORDER_WIDTH))
#define HEADER_BOX_HEIGHT_1   30
#define HEADER_BOX_HEIGHT_2   50
#define HEADER_BOX_HEIGHT_3   100
#define STATUS_BOX_HEIGHT   50

#define Y_OFFSET_STATUS_MSG (SCREEN_LOGICAL_HEIGHT-SCREEN_BORDER_WIDTH-STATUS_BOX_HEIGHT)

#define TEXT_BORDER 10
#define CORNER_CUT  20


static SDL_FColor color_as_fcolor(SDL_Color color)
{
	SDL_FColor fcolor = {
		.r = (float)color.r/255,
		.g = (float)color.g/255,
		.b = (float)color.b/255,
		.a = (float)color.a/255,
	};

	return fcolor;
}
static SDL_Color screen_get_color(enum Screen_Color color)
{
	struct Color *ptr = NULL;

	switch(color) {
	case SCREEN_COLOR_PRIMARY:
		ptr = &g_config.screen_color_primary;
		break;
	case SCREEN_COLOR_HIGHLIGHT:
		ptr = &g_config.screen_color_highlight;
		break;
	case SCREEN_COLOR_BACKGROUND:
		ptr = &g_config.screen_color_background;
		break;
	case SCREEN_COLOR_NONE:
		ptr = NULL;
		break;
	}

	SDL_Color sdl_color = {0};

	if (ptr != NULL) {
		sdl_color.r = (uint8_t) ptr->r;
		sdl_color.g = (uint8_t) ptr->g;
		sdl_color.b = (uint8_t) ptr->b;
		sdl_color.a = (uint8_t) ptr->a;
	}
	return sdl_color;
}

void screen_set_color(struct Screen *screen, enum Screen_Color color)
{
	SDL_Color sdl_color = screen_get_color(color);
	SDL_SetRenderDrawColor(screen->renderer, sdl_color.r, sdl_color.g, sdl_color.b, sdl_color.a);
}

static void check_sdl(bool success)
{
	if (!success) {
		fprintf(stderr, "SDL_ERROR: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}
}

static Result screen_load_ttf_font(struct Screen *screen, const char *filepath)
{
	TTF_Init();
	screen->font = TTF_OpenFont(filepath, 24);

	if (screen->font == NULL) {
		return result_make(
			false,
			"could not initialize font %s: %s\n",
			filepath, SDL_GetError());
	}


	return result_make_success();
}

struct Screen_Dimension screen_get_text_dimension(struct Screen *screen, int font_size, const char *fmt, ...)
{
	if ( fmt == NULL || strlen(fmt) == 0) {
		struct Screen_Dimension retval = {0};
		return retval;
	}

	char buffer[1024];

	va_list arg_list;
	va_start(arg_list, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg_list);
	va_end(arg_list);

	// quickfix, draw anything to avoid tmp_surface is null
	if (strlen(buffer) == 0) {
		buffer[0] = ' ';
		buffer[1] = '\0';
	}

	TTF_SetFontSize(screen->font, (float)font_size);

	SDL_Color font_color = {0};

	SDL_Surface *tmp_surface = TTF_RenderText_Blended(
		screen->font,
		buffer,
		0,
		font_color);

	struct Screen_Dimension retval = {.w = tmp_surface->w, .h = tmp_surface->h};
	SDL_DestroySurface(tmp_surface);

	return retval;
}

void screen_draw_icon(struct Screen *screen, int x, int y, int width, int height, const char *name)
{
// TODO: implement draw icon/svg
	(void) screen;
	(void) x;
	(void) y;
	(void) width;
	(void) height;
	(void) name;
#if 1
	//char full_path[2048];
	//snprintf(full_path, sizeof(full_path), "%s/%s", g_config.resources_dir, name);
	SDL_Surface *svg_surface = IMG_Load(name);

	if (!svg_surface) {
		fprintf(stderr, "IMG_LoadSVG error: %s\n", SDL_GetError());
		return;
	}

	SDL_Texture *svg_tex = SDL_CreateTextureFromSurface(screen->renderer, svg_surface);
	SDL_DestroySurface(svg_surface);
	if (!svg_tex) {
		SDL_DestroySurface(svg_surface);
		fprintf(stderr, "SDL_CreateTextureFromSurface error: %s\n", SDL_GetError());
	}
	screen_set_color(screen, SCREEN_COLOR_PRIMARY);
	SDL_SetTextureBlendMode(svg_tex, SDL_BLENDMODE_BLEND);
	SDL_SetTextureColorMod(svg_tex, 0xFF, 0xD4, 0x00);
	//SDL_SetTextureColorMod(svg_tex, 0xFF, 0x00, 0x00);
	SDL_FRect text_rect = {
		.x = (float)x,
		.y = (float)y,
		.w = (float)width,
		.h = (float)height
	};
	SDL_RenderTexture(screen->renderer, svg_tex, NULL, &text_rect);
	SDL_DestroyTexture(svg_tex);

#else
	SDL_Surface *tmp_surface = IMG_Load(name);
	if (tmp_surface == NULL) {
		printf("ERROR: unable to load %s!\n", name);
		return; // TODO: Error
	}


	SDL_Texture *texture = SDL_CreateTextureFromSurface(
			screen->renderer,
			tmp_surface);

	SDL_Color primary_color = screen_get_color(SCREEN_COLOR_PRIMARY);
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	SDL_SetTextureColorMod(texture, primary_color.r, primary_color.g, primary_color.b);   // tint multiplier for RGB
	SDL_SetTextureAlphaMod(texture, primary_color.a);         // overall alpha
	SDL_Rect text_rect = {
		.x = x,
		.y = y,
		.w = width,
		.h = height
	};

	SDL_DestroySurface(tmp_surface);
	SDL_RenderTexture(screen->renderer, texture, NULL, &text_rect);
	SDL_DestroyTexture(texture);
#endif
}

void screen_draw_text(struct Screen *screen, int x, int y, int font_size, const char *fmt, ...)
{
	if ( fmt == NULL || strlen(fmt) == 0) {
		return;
	}

	char buffer[1024];

	va_list arg_list;
	va_start(arg_list, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg_list);
	va_end(arg_list);

	// quickfix, draw anything to avoid tmp_surface is null
	if (strlen(buffer) == 0) {
		buffer[0] = ' ';
		buffer[1] = '\0';
	}

	TTF_SetFontSize(screen->font, (float)font_size);

	SDL_Color primary_color = screen_get_color(SCREEN_COLOR_PRIMARY);
	SDL_Surface *tmp_surface = TTF_RenderText_Blended(
		screen->font,
		buffer,
		0,
		primary_color);

	SDL_Texture *texture = SDL_CreateTextureFromSurface(
			screen->renderer,
			tmp_surface);

	SDL_FRect text_rect = {
		.x = (float)x,
		.y = (float)y,
		.w = (float)tmp_surface->w,
		.h = (float)tmp_surface->h
	};

	SDL_DestroySurface(tmp_surface);
	SDL_RenderTexture(screen->renderer, texture, NULL, &text_rect);
	SDL_DestroyTexture(texture);
}

void screen_draw_line(struct Screen *screen, int x0, int y0, int x1, int y1)
{
	screen_set_color(screen, SCREEN_COLOR_PRIMARY);
	SDL_RenderLine(screen->renderer, (float)x0, (float)y0, (float)x1, (float)y1);
}

void screen_draw_box(struct Screen *screen, const int x, const int y, int width, int height, bool is_selected)
{
	if (is_selected) {
		screen_draw_box_filled(screen, x, y, width, height, SCREEN_COLOR_PRIMARY, SCREEN_COLOR_HIGHLIGHT);
	}
	else {
		screen_draw_box_filled(screen, x, y, width, height, SCREEN_COLOR_PRIMARY, SCREEN_COLOR_NONE);
	}
}

void screen_draw_box_filled(struct Screen *screen, int x, int y, int width, int height, enum Screen_Color fg_color, enum Screen_Color bg_color)
{
	/* 0 --------------- 1
	 * |                 |
	 * |                 2
	 * 4 ------------- 3
	 */
	SDL_FPoint points[] = {
		{.x = (float)x                        , .y = (float)y},
		{.x = (float)x+(float)width           , .y = (float)y},
		{.x = (float)x+(float)width           , .y = (float)y+(float)height-CORNER_CUT},
		{.x = (float)x+(float)width-CORNER_CUT, .y = (float)y+(float)height},
		{.x = (float)x                        , .y = (float)y+(float)height},
		{.x = (float)x                        , .y = (float)y},
	};

	const int vertices_indexes[] = {
		0, 1, 2,
		0, 2, 3,
		0, 3, 4
	};

	if (bg_color != SCREEN_COLOR_NONE) {
		SDL_Vertex vertexes[ARRAY_SIZE(points)];

		SDL_FColor fcolor = color_as_fcolor(screen_get_color(bg_color));
		for (size_t i=0; i < ARRAY_SIZE(vertexes); ++i) {
			vertexes[i].position.x = points[i].x;
			vertexes[i].position.y = points[i].y;
			vertexes[i].color.r = fcolor.r;
			vertexes[i].color.g = fcolor.g;
			vertexes[i].color.b = fcolor.b;
			vertexes[i].color.a = fcolor.a;
		}

		SDL_RenderGeometry(
			screen->renderer,
			NULL,
			vertexes, ARRAY_SIZE(vertexes),
			vertices_indexes, ARRAY_SIZE(vertices_indexes));
	}

	if (fg_color != SCREEN_COLOR_NONE) {
		screen_set_color(screen, fg_color);
		SDL_RenderLines(screen->renderer, points, ARRAY_SIZE(points));
	}
}

void screen_draw_text_boxed(struct Screen *screen, int x, int y, int font_size, int min_width, bool is_selected, const char *fmt, ...)
{
	if ( fmt == NULL || strlen(fmt) == 0) {
		return;
	}

	char buffer[1024];

	va_list arg_list;
	va_start(arg_list, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg_list);
	va_end(arg_list);

	// quickfix, draw anything to avoid tmp_surface is null
	if (strlen(buffer) == 0) {
		buffer[0] = ' ';
		buffer[1] = '\0';
	}

	TTF_SetFontSize(screen->font, (float)font_size);

	SDL_Color primary_color = screen_get_color(SCREEN_COLOR_PRIMARY);
	SDL_Surface *tmp_surface = TTF_RenderText_Blended(
		screen->font,
		buffer,
		0,
		primary_color);

	SDL_Texture *texture = SDL_CreateTextureFromSurface(
			screen->renderer,
			tmp_surface);

	SDL_FRect text_rect = {
		.x = (float) x+TEXT_BORDER,
		.y = (float) y+TEXT_BORDER,
		.w = (float) tmp_surface->w,
		.h = (float) tmp_surface->h
	};

	const int text_width = (int)text_rect.w+2*TEXT_BORDER;

	screen_draw_box(
		screen,
		x,
		y,
		MAX(min_width, text_width), // pick greater one
		(int) text_rect.h+2*TEXT_BORDER,
		is_selected
	);

	SDL_DestroySurface(tmp_surface);
	SDL_RenderTexture(screen->renderer, texture, NULL, &text_rect);
	SDL_DestroyTexture(texture);
}

//#define WINDOW_CLOSE_BUTTON_WIDTH 20
void screen_draw_window(struct Screen *screen, int x, int y, int width, int height, const char *name)
{
	static const int close_button_width = 50;
	/*
	 *                           2---3
	 *   0----------------------1    |
	 * 8                             |
	 * |                             |
	 * |                             |
	 * |                             |
	 * |                             |
	 * |                             |
	 * |                             |
	 * 7                             4
	 *   6 ----------------------- 5
	 */

	const int x_left   = x;
	const int x_right  = x+width;
	const int y_top    = y;
	const int y_bottom = y+height;

	SDL_FPoint points[] = {
		{.x = (float)x_left+CORNER_CUT                         , .y = (float)y_top+CORNER_CUT},
		{.x = (float)x_right-(2*CORNER_CUT)-close_button_width , .y = (float)y_top+CORNER_CUT},
		{.x = (float)x_right-(1*CORNER_CUT)-close_button_width , .y = (float)y_top},
		//{.x = x_right-(1*CORNER_CUT)                    , .y = y_top},
		{.x = (float)x_right                                   , .y = (float)y_top},
		{.x = (float)x_right                                   , .y = (float)y_bottom-CORNER_CUT},
		{.x = (float)x_right-CORNER_CUT                        , .y = (float)y_bottom},
		{.x = (float)x_left+CORNER_CUT                         , .y = (float)y_bottom},
		{.x = (float)x_left                                    , .y = (float)y_bottom-CORNER_CUT},
		{.x = (float)x_left                                    , .y = (float)y_top+(2*CORNER_CUT)},
		{.x = (float)x_left+CORNER_CUT                         , .y = (float)y_top+CORNER_CUT},
	};

	const int vertices_indexes[] = {
		0, 1, 5,
		5, 1, 2,
		5, 2, 4,
		4, 2, 3,
		5, 6, 7,
		5, 7, 8,
		5, 8, 0
	};

	SDL_Vertex vertexes[ARRAY_SIZE(points)];

	SDL_FColor fcolor = color_as_fcolor(screen_get_color(SCREEN_COLOR_HIGHLIGHT));
	for (size_t i=0; i < ARRAY_SIZE(vertexes); ++i) {
		vertexes[i].position.x = points[i].x;
		vertexes[i].position.y = points[i].y;
		vertexes[i].color.r = fcolor.r;
		vertexes[i].color.g = fcolor.g;
		vertexes[i].color.b = fcolor.b;
		vertexes[i].color.a = fcolor.a;
	}

	SDL_RenderGeometry(
			screen->renderer,
			NULL,
			vertexes, ARRAY_SIZE(vertexes),
			vertices_indexes, ARRAY_SIZE(vertices_indexes));

	screen_set_color(screen, SCREEN_COLOR_PRIMARY);
	SDL_RenderLines(screen->renderer, points, ARRAY_SIZE(points));
	screen_draw_text(screen, x_left+CORNER_CUT+10, y_top+CORNER_CUT+10, g_config.screen_font_size_l, name);
}

static void screen_handle_keypress(struct Screen *screen, SDL_Keycode *key)
{
	switch(*key) {
		case SDLK_Q      : screen->quit = true; break;
		case SDLK_ESCAPE : screen->quit = true; break;
	}
}



#define Y_OFFSET_CHOOSER 120
#define CHOOSER_HEIGHT   50
#define CHOOSER_INT_WIDTH 50
void screen_draw_option(
	struct Screen *screen,
	int name_width,
	int value_width,
	int y_index,
	bool is_selected,
	const char *name,
	const char *fmt_value, ...)
{
	int x = SCREEN_BORDER_WIDTH;
	int y = Y_OFFSET_CHOOSER + (y_index * (g_config.screen_font_size_l+2*TEXT_BORDER+10));

	screen_draw_text(screen, x, y, g_config.screen_font_size_l, "%s", name);
	x += name_width;

	char buffer[1024];

	va_list arg_list;
	va_start(arg_list, fmt_value);
	vsnprintf(buffer, sizeof(buffer), fmt_value, arg_list);
	va_end(arg_list);

	screen_draw_text_boxed(screen, x, y, g_config.screen_font_size_l, value_width, is_selected, "%s", buffer);

}

Result screen_init(struct Screen *screen, int width, int height)
{

	if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
	{
		return result_make(false, "SDL could not be initialized!\n"
			"SDL_Error: %s\n", SDL_GetError());
	}

#if defined linux && SDL_VERSION_ATLEAST(2, 0, 8)
	// Disable compositor bypass
	if(!SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0"))
	{
		return result_make(false, "SDL can not disable compositor bypass!");
	}
#endif

	// Create window
	screen->window = SDL_CreateWindow(
			"Dartosphere",
			width,
			height,
			0 /*| SDL_WINDOW_FULLSCREEN_DESKTOP*/);

	if(!screen->window) {
		return result_make(false, "Window could not be created!\n"
				"SDL_Error: %s\n", SDL_GetError());
	}

	// Set the window to fullscreen
	//SDL_SetWindowFullscreen(screen->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	if (g_config.screen_hide_cursor) {
		SDL_HideCursor();
	}


	// Create renderer
	screen->renderer  = SDL_CreateRenderer(screen->window, NULL);
	if(!screen->renderer) {
		return result_make(false, "Renderer could not be created!\n"
				"SDL_Error: %s\n", SDL_GetError());
	}

	SDL_SetRenderLogicalPresentation(
		screen->renderer,
		SCREEN_LOGICAL_WIDTH,
		SCREEN_LOGICAL_HEIGHT,
		SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

	SDL_SetRenderDrawBlendMode(screen->renderer, SDL_BLENDMODE_BLEND);

	char font_filepath[1024];
	snprintf(font_filepath, sizeof(font_filepath), "%s/%s",  g_config.resources_dir, g_config.font_file);
	Result r = screen_load_ttf_font(screen, font_filepath);
	if (!r.success) {
		//log_error("failed to load font: %s\n", r.msg);
		return r;
	}

	screen->quit = false;
	return result_make(true, "");
}

void screen_destroy(struct Screen *screen)
{
	log_info("destroying SDL Window\n", NULL);
	SDL_DestroyRenderer(screen->renderer);
	SDL_DestroyWindow(screen->window);
	SDL_Quit();
}

void screen_rendering_start(struct Screen *screen)
{
	screen->ticks = SDL_GetTicks();

	SDL_Event event;

	SDL_GetMouseState(&screen->mouse_x, &screen->mouse_y);
	screen->mouse_clicked = false;

	while (SDL_PollEvent(&event)) {

		switch (event.type) {

		case SDL_EVENT_QUIT:
			screen->quit = true;
			break;

		case SDL_EVENT_KEY_DOWN:
			screen_handle_keypress(screen, &event.key.key);
			break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			// we dont' care which mouse button was clicked
			screen->mouse_clicked = true;
			break;

		case SDL_EVENT_FINGER_DOWN:
			screen->mouse_clicked = true;
			int w,h;
			SDL_GetCurrentRenderOutputSize(screen->renderer, &w, &h);
			screen->mouse_x = (int)((float)w*event.tfinger.x);
			screen->mouse_y = (int)((float)h*event.tfinger.y);
			break;
		}
	}

	// Initialize renderer color white for the background
	struct Color *bg = &g_config.screen_color_background;
	check_sdl(SDL_SetRenderDrawColor(screen->renderer, (uint8_t) bg->r, (uint8_t) bg->g, (uint8_t) bg->b, (uint8_t) bg->a));

	// Clear screen
	check_sdl(SDL_RenderClear(screen->renderer));
}

void screen_rendering_stop(struct Screen *screen)
{
	SDL_RenderPresent(screen->renderer);

	const uint64_t ticks_used = SDL_GetTicks() - screen->ticks;

	int64_t delta_ms = (1000/SCREEN_FPS)-ticks_used;

	if (delta_ms < 0) return;

	SDL_Delay((uint32_t)delta_ms);
}

