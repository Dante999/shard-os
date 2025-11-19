#include "ui_radio.h"

#include "ui_elements.h"
#include "libcutils/util_makros.h"


struct Ui_Clickable_List g_clickable_list = {0};

void ui_radio_render(struct Screen *screen)
{
	static bool is_initialized = false;

	if (!is_initialized) {
		g_clickable_list.x = 520;
		g_clickable_list.y = 180;
		g_clickable_list.w = 460;
		g_clickable_list.h = 50;
		ui_clickable_list_init(&g_clickable_list);

		ui_clickable_list_append(&g_clickable_list, "Rock Antenne - National");
		ui_clickable_list_append(&g_clickable_list, "Rock Antenne - Hardrock");
		ui_clickable_list_append(&g_clickable_list, "Rock Antenne - Modern Metal");
		ui_clickable_list_append(&g_clickable_list, "Bayern 1");
		ui_clickable_list_append(&g_clickable_list, "Ego FM");
		ui_clickable_list_append(&g_clickable_list, "Radio Bob!");
		//ui_clickable_list_append(&g_clickable_list, "Light FM");

		is_initialized = true;
	}

#if 1
	ui_clickable_list_render(screen, &g_clickable_list);
#else
	for( size_t i=0; i < ARRAY_SIZE(g_buttons); ++i) {
		ui_button_render(screen, &g_buttons[i]);
	}
#endif
}

