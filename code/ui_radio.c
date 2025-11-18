#include "ui_radio.h"

#include "ui_elements.h"
#include "libcutils/util_makros.h"

// TODO: implemenet Ui_List
struct Ui_Button g_buttons[] = {
{.x = 520, .y = 200, .w = 450, .h = 50, .text="Rock Antenne - National" },
{.x = 520, .y = 250, .w = 450, .h = 50, .text="Rock Antenne - Hardrock" },
{.x = 520, .y = 300, .w = 450, .h = 50, .text="Rock Antenne - Modern Metal" },
{.x = 520, .y = 350, .w = 450, .h = 50, .text="Bayern 1" },
{.x = 520, .y = 400, .w = 450, .h = 50, .text="Ego FM" },
{.x = 520, .y = 450, .w = 450, .h = 50, .text="Radio Bob!" },
{.x = 520, .y = 500, .w = 450, .h = 50, .text="Light FM" }
};

void ui_radio_render(struct Screen *screen)
{
	for( size_t i=0; i < ARRAY_SIZE(g_buttons); ++i) {
		ui_button_render(screen, &g_buttons[i]);
	}
}

