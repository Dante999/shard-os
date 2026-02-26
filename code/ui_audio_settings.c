#include "ui_audio_settings.h"

#include "ui_elements.h"
#include "config.h"
#include "audio.h"


static struct Ui_Chooser g_volume_chooser = {
	.name = "Volume",
	.outline = {
		.x = UI_DIALOG_X_START + SCREEN_BORDER_WIDTH,
		.y = UI_DIALOG_Y_START + SCREEN_BORDER_WIDTH,
		.w = (UI_DIALOG_X_END - UI_DIALOG_Y_START - SCREEN_BORDER_WIDTH)/2,
		.h = 0
	},
	.name_width = 100,
};
static struct Ui_Chooser g_device_chooser = {
	.name = "Device",
	.outline = {
		.x = UI_DIALOG_X_START + SCREEN_BORDER_WIDTH,
		.y = UI_DIALOG_Y_START + SCREEN_BORDER_WIDTH + 40,
		.w = (UI_DIALOG_X_END - UI_DIALOG_Y_START - SCREEN_BORDER_WIDTH)/2,
		.h = 0
	},
	.name_width = 100
};


void on_enter_audio_settings(void)
{
	ui_chooser_int_init(&g_volume_chooser,g_config.volume, 0, 100, 10);

	ui_chooser_str_init(&g_device_chooser);
	ui_chooser_str_append(&g_device_chooser, "Headset");
	ui_chooser_str_append(&g_device_chooser, "HDMI Screen");
	ui_chooser_str_append(&g_device_chooser, "FM Sender");
}

void on_render_audio_settings(struct Screen *screen)
{
	if (ui_chooser_render(screen, &g_volume_chooser) == UI_EVENT_MODIFIED) {
		g_config.volume = g_volume_chooser.data.int_chooser.cur_value;
		audio_set_volume(g_config.volume);
	}
	
	ui_chooser_render(screen, &g_device_chooser); // TODO

}
void on_exit_audio_settings(void)
{
}
