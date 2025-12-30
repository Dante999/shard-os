#include "app_dice.h"

#include "ui_elements.h"

#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

/**
 * right side: list of buttons to select amount of dice sides (3,4,6,15,20, 100, ...)
 * middle    : dice result
 * bottom    : button for throwing the dice(s)
 *
 * OR
 * right side : selector amout of dices and amout of dice sides
 * middle     : dice throwing result
 * bottom     : button for throwing the dice(s)
 */

char g_roll_result[40];
static struct Ui_Box g_roll_dice_box = {0};
static size_t g_dice_sides = 6;

static uint64_t random_uint64(void) {
	int fd = open("/dev/urandom", O_RDONLY);
	uint64_t v;
	ssize_t r = read(fd, &v, sizeof(v));
	close(fd);
	assert(r == sizeof (v));
	return v;
}

static void on_click_roll_dices(struct Ui_Box *box)
{
	(void) box;
	uint64_t result = random_uint64()%g_dice_sides;

	snprintf(g_roll_result, sizeof(g_roll_result), "%ld", result+1);
}

void app_dice_init(struct Screen *screen)
{
	(void) screen;

	g_roll_dice_box.x = 300;
	g_roll_dice_box.y = 200;
	g_roll_dice_box.w = 300;
	g_roll_dice_box.h = 300;
	g_roll_dice_box.is_selectable = true;
	g_roll_dice_box.on_click = on_click_roll_dices;


	strcpy(g_roll_result, "[?]");

}

void app_dice_open(struct Screen *screen)
{
	(void) screen;
}

void app_dice_render(struct Screen *screen)
{
	const int font_size = 140;

	ui_box_render(screen, &g_roll_dice_box);
	struct Screen_Dimension dim = screen_get_text_dimension(
		screen,
		font_size,
		g_roll_result);

	const int x_center = g_roll_dice_box.x+(g_roll_dice_box.w/2);
	const int y_center = g_roll_dice_box.y+(g_roll_dice_box.h/2);
	screen_draw_text(screen, x_center-(dim.w/2), y_center-(font_size/2), font_size, g_roll_result);

}

void app_dice_close(struct Screen *screen)
{
	(void) screen;
}

