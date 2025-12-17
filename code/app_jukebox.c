#include "app_jukebox.h"

#include "ui_elements.h"
#include "config.h"
#include "filebrowser.h"
#include "audioplayer.h"

#include "libcutils/logger.h"

#define MAX_BROWSER_ENTRY_LEN 40

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

	if (node->type == NODE_TYPE_DIR || strcmp(node->name, "..") == 0) {
		log_debug("Entering subdir!\n");
		filebrowser_enter(&g_filebrowser, node->name);
		refresh_clickable_list(g_screen);
	}
	else if (node->type == NODE_TYPE_FILE) {
		char absolute_filepath[2048];
		snprintf(absolute_filepath, sizeof(absolute_filepath),
			"%s/%s/%s", g_filebrowser.root_path, g_filebrowser.sub_path, node->name);
		log_debug("Playing file: %s\n", absolute_filepath);

		struct Audio_File_Metadata metadata = {0};
		Result r = audioplayer_play_file(absolute_filepath, &metadata);
		if (r.success) {
			strncpy(g_player.first_line , metadata.artist, sizeof(g_player.first_line));
			strncpy(g_player.second_line, metadata.title , sizeof(g_player.second_line));
			g_player.track_len_sec = (int) metadata.length_secs;

			log_debug("title length: %ds\n", g_player.track_len_sec);
		}
	}
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
	audioplayer_init();
}


void app_jukebox_open(struct Screen *screen)
{
	(void) screen;
}

enum App_Status app_jukebox_render(struct Screen *screen)
{
	screen_draw_text(screen, 
		g_player.x, g_player.y-30,
		g_config.screen_font_size_xs, g_filebrowser.sub_path);

	g_player.track_pos_sec = audioplayer_get_current_pos_in_secs();
	ui_clickable_list_render(screen, &g_clickable_list);
	ui_media_player_render(screen, &g_player);

	return APP_STATUS_RUNNING;
}

void app_jukebox_close(struct Screen *screen)
{
	(void) screen;
}
