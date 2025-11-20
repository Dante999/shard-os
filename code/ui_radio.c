#include "ui_radio.h"

#include "ui_elements.h"

struct Ui_Clickable_List g_clickable_list = {0};

void ui_radio_render(struct Screen *screen)
{
	static bool is_initialized = false;

	if (!is_initialized) {
		ui_clickable_list_init(screen, &g_clickable_list, 520, 180, 460, 6);

		ui_clickable_list_append(screen, &g_clickable_list, "Rock Antenne - National");
		ui_clickable_list_append(screen, &g_clickable_list, "Rock Antenne - Hardrock");
		ui_clickable_list_append(screen, &g_clickable_list, "Rock Antenne - Modern Metal");
		ui_clickable_list_append(screen, &g_clickable_list, "Bayern 1");
		ui_clickable_list_append(screen, &g_clickable_list, "Ego FM");
		ui_clickable_list_append(screen, &g_clickable_list, "Radio Bob!");
		ui_clickable_list_append(screen, &g_clickable_list, "Light FM");
		is_initialized = true;
	}

	ui_clickable_list_render(screen, &g_clickable_list);
}

