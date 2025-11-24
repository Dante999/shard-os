#include "ui_radio.h"

#include "ui_elements.h"
#include "config.h"

#include "libcutils/util_makros.h"
#include "libcutils/config_file.h"

struct Radio_Station {
	char name[40];
	char url[2048];
};

struct Radio_Station_List {
	struct Radio_Station items[255];
	size_t count;
};

struct Ui_Clickable_List  g_clickable_list = {0};
struct Radio_Station_List g_radio_stations = {0};

static void radiostation_add(struct Radio_Station_List *list, const char *name, const char *url)
{
	if (list->count >= ARRAY_SIZE(list->items)-1) return;

	size_t i = list->count++;

	strncpy(list->items[i].name, name, sizeof(list->items[i].name));
	strncpy(list->items[i].url , url , sizeof(list->items[i].url));
}

void ui_radio_init(struct Screen *screen, const char *filepath)
{
	(void) filepath;
	char fullpath[255];

	snprintf(fullpath, sizeof(fullpath), "%s/%s", g_config.resources_dir, filepath);
	printf("Trying to load %s\n", fullpath);
	struct Config_File cfg = {0};
	Result res = config_file_init(&cfg, fullpath);
	config_file_print(&cfg);

	if (!res.success) {
		printf("ERROR: failed to load radio stations: %s\n", res.msg);
		return;
	}

	g_radio_stations.count = 0;
	for (size_t i=0; i < cfg.count; ++i) {
		radiostation_add(&g_radio_stations, cfg.keys[i], cfg.values[i]);
	}

	ui_clickable_list_clear(&g_clickable_list);

	ui_clickable_list_init(screen, &g_clickable_list, 520, 180, 460, 6);
	for (size_t i=0; i < g_radio_stations.count; ++i) {
		ui_clickable_list_append(screen, &g_clickable_list, g_radio_stations.items[i].name);
	}
}

void ui_radio_render(struct Screen *screen)
{
	ui_clickable_list_render(screen, &g_clickable_list);
}

