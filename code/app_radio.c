#include "app_radio.h"

#include "ui_elements.h"
#include "config.h"
#include "audio.h"

#include <linux/limits.h>
#include <time.h>
#include <stdio.h>

#include "libcutils/util_makros.h"
#include "libcutils/config_file.h"
#include "libcutils/logger.h"

struct Radio_Station {
	char name[40];
	char url[2048];
};

struct Radio_Station_List {
	struct Radio_Station items[255];
	size_t count;
	int selected_item;
};

static struct Ui_Clickable_List  g_clickable_list = {0};
static struct Radio_Station_List g_radio_stations = {0};
static struct Ui_Media_Player    g_player         = {0};

static void on_radio_station_clicked(int index)
{
	g_radio_stations.selected_item = index;

	struct Radio_Station *radio = &g_radio_stations.items[index];
	log_debug("Radiostation clicked: [%zu] %s\n", index, radio->name);
	audio_pause(); // TODO: block until curl thread stopped

	Result res = audio_play_url(radio->url);

	if (!res.success) {
		log_error("failed to play radio station: %s", res.msg);
	}

	g_player.is_playing = true;
	g_player.track_pos_sec = 0;
	strncpy(g_player.first_line , radio->name, sizeof(g_player.first_line));
}

static void on_mediaplayer_clicked(const char *button_id)
{
	if (strcmp(button_id, UI_MEDIA_PLAYER_PLAY_BUTTON_ID) == 0) {
		if (audio_is_playing()) {
			audio_pause();
			g_player.is_playing = false;
		}
		else {
			int index = g_radio_stations.selected_item;

			if (index != -1) {
				struct Radio_Station *radio = &g_radio_stations.items[index];
				Result res = audio_play_url(radio->url);

				if (!res.success) {
					log_error("failed to play radio station: %s", res.msg);
				}
				g_player.is_playing = true;
				g_player.track_pos_sec = 0;
				strncpy(g_player.first_line , radio->name, sizeof(g_player.first_line));
			}
		}
	}
}

static void radiostation_add(struct Radio_Station_List *list, const char *name, const char *url)
{
	if (list->count >= ARRAY_SIZE(list->items)-1) return;

	size_t i = list->count++;

	strncpy(list->items[i].name, name, sizeof(list->items[i].name));
	strncpy(list->items[i].url , url , sizeof(list->items[i].url));
}

void app_radio_init(struct Screen *screen, const char *filepath)
{
	(void) filepath;
	char fullpath[PATH_MAX];

	snprintf(fullpath, sizeof(fullpath), "%s/%s", g_config.resources_dir, filepath);
	log_debug("Trying to load %s\n", fullpath);
	struct Config_File cfg = {0};
	Result res = config_file_init(&cfg, fullpath);

	if (!res.success) {
		log_error("failed to load radio stations: %s\n", res.msg);
		return;
	}

	g_radio_stations.count         = 0;
	g_radio_stations.selected_item = -1;
	for (size_t i=0; i < cfg.count; ++i) {
		radiostation_add(&g_radio_stations, cfg.keys[i], cfg.values[i]);
	}

	ui_clickable_list_clear(&g_clickable_list);

	const int y_start = 200;
	const int height  = 350;

	ui_clickable_list_init(screen, &g_clickable_list, 520, y_start, 460, height);
	for (size_t i=0; i < g_radio_stations.count; ++i) {
		ui_clickable_list_append(&g_clickable_list, g_radio_stations.items[i].name);
	}

	g_clickable_list.on_click = on_radio_station_clicked;

	ui_media_player_init(screen, &g_player, 50, y_start, 450, height, on_mediaplayer_clicked);
}

void app_radio_render(struct Screen *screen)
{
	static int frame_count = 0;

	if (g_player.is_playing) {
		// TODO: not really stable upcounting...
		++frame_count;
		
		if (frame_count >= SCREEN_FPS) {
			g_player.track_pos_sec++;
			frame_count = 0;
		}
	}

	snprintf(
		g_player.last_line,
		sizeof(g_player.last_line),
		"Buffered: %d kB",
		audio_get_buffered_bytes()/1024);

	//static time_t timestamp_last = 0;

	//if (g_player.is_playing) {
	//	time_t timestamp_current = time(NULL);

	//	if (timestamp_last-timestamp_current > 1) {
	//		g_player.track_pos_sec++;
	//	}
	//}


	
	ui_clickable_list_render(screen, &g_clickable_list);
	ui_media_player_render(screen, &g_player);
}

void app_radio_close(struct Screen *screen)
{
	audio_close();
	(void) screen;
}

void app_radio_open(struct Screen *screen)
{
	audio_open();
	(void) screen;
}
