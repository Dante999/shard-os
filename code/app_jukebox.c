#include "app_jukebox.h"

#include "ui_elements.h"
#include "config.h"
#include "filebrowser.h"

#include "libcutils/logger.h"

#define MAX_BROWSER_ENTRY_LEN 30

static char g_basepath[1024];
static struct Ui_Clickable_List  g_clickable_list = {0};
static struct Ui_Media_Player    g_player         = {0};
static struct Filebrowser        g_filebrowser    = {0};

static struct Screen *g_screen = NULL;

static void refresh_clickable_list(struct Screen *screen)
{
	ui_clickable_list_clear(&g_clickable_list);

	for (size_t i=0; i < g_filebrowser.node_count; ++i) {
		char entry[MAX_BROWSER_ENTRY_LEN];
		strncpy(entry, g_filebrowser.nodes[i].name, sizeof(entry));
		log_info("appending %s\n", entry);
		ui_clickable_list_append(screen, &g_clickable_list, entry);
	}
}

static void on_filebrowser_clicked(int index)
{
	struct Node *node = &g_filebrowser.nodes[index];
	log_debug("Filebrowser[%d] clicked: %s!\n", index, node->name);

	if (node->type == NODE_TYPE_DIR) {
		log_debug("Entering subdir!\n");
		filebrowser_enter(&g_filebrowser, node->name);
		refresh_clickable_list(g_screen);
	}
	// TODO:
}

void app_jukebox_init(struct Screen *screen, const char *filepath)
{
	snprintf(g_basepath, sizeof(g_basepath), "%s/%s", g_config.resources_dir, filepath);
	printf("Trying to load %s\n", g_basepath);

	filebrowser_init(&g_filebrowser, g_basepath);
	//ui_clickable_list_clear(&g_clickable_list);

	const int y_start = 200;
	const int height  = 350;

	ui_clickable_list_init(screen, &g_clickable_list, 520, y_start, 460, height);
	refresh_clickable_list(screen);
	g_clickable_list.on_click = on_filebrowser_clicked;

	ui_media_player_init(screen, &g_player, 50, y_start, 450, height);
}


void app_jukebox_open(struct Screen *screen)
{
	(void) screen;
}

enum App_Status app_jukebox_render(struct Screen *screen)
{
	//refresh_clickable_list(screen); // TODO: do only on changes
	ui_clickable_list_render(screen, &g_clickable_list);
	ui_media_player_render(screen, &g_player);

	return APP_STATUS_RUNNING;
}

void app_jukebox_close(struct Screen *screen)
{
	(void) screen;
}
