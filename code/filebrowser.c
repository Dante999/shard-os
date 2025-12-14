#include "filebrowser.h"

#include "libcutils/logger.h"
#include "libcutils/util_makros.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>

static void filebrowser_append_node(struct Filebrowser *fb, const char *name, enum Node_Type type)
{
	if (fb->node_count < ARRAY_SIZE(fb->nodes)) {
		struct Node *ptr = &fb->nodes[fb->node_count++];

		strncpy(ptr->name, name, sizeof(ptr->name));
		ptr->type = type;
	}
}

static void filebrowser_load(struct Filebrowser *fb)
{
	char path[1024];
	fb->node_count = 0;
	if (strlen(fb->sub_path) > 0) {
		snprintf(path, sizeof(path), "%s/%s", fb->root_path, fb->sub_path);
	}
	else {
		strncpy(path, fb->root_path, sizeof(path));
	}
	DIR *cur_dir = opendir(path);

	if (cur_dir == NULL) {
		log_error("Unable to open %s\n", path);
		return;
	}

	struct dirent *ent = NULL;
	while ((ent = readdir(cur_dir)) != NULL) {

		if (strcmp(ent->d_name, ".") == 0) continue;

		if (ent->d_type == DT_DIR) {
			filebrowser_append_node(fb, ent->d_name, NODE_TYPE_DIR);
			printf("\tdir : %s\n", ent->d_name);
		}
		else if (ent->d_type == DT_REG) {
			filebrowser_append_node(fb, ent->d_name, NODE_TYPE_FILE);
			printf("\tfile: %s\n", ent->d_name);
#if 0
			char symlink[1024];
			char symlink_target[1024];

			snprintf(symlink, sizeof(symlink), "/proc/asound/%s", ent->d_name);
			ssize_t ret = readlink(symlink, symlink_target, sizeof(symlink_target));
			int errval = errno;
			if (ret == -1) {
				fprintf(stderr, "ERROR: unable to read symlink %s: %s\n",
						symlink, strerror(errval));
			}
			printf("\t* %s -> %s\n", ent->d_name, symlink_target);
#endif
		}
	}

	closedir(cur_dir);
}
void filebrowser_enter(struct Filebrowser *fb, const char *dir_name)
{
	if (strcmp(dir_name, "..") == 0) {
		log_info("going one folder up...\n");
		char *pos = strrchr(fb->sub_path, '/');

		if (pos != NULL) {
			*pos = '\0';
		}
		else {
			fb->sub_path[0] = '\0';
		}
	}
	else if (strlen(fb->sub_path) + strlen(dir_name) < sizeof(fb->sub_path)+2) {
		strcat(fb->sub_path, "/");
		strcat(fb->sub_path, dir_name);
	}

	filebrowser_load(fb);
}

void filebrowser_init(struct Filebrowser *fb, const char *root_path)
{
	strncpy(fb->root_path, root_path, sizeof(fb->root_path));

	fb->sub_path[0] = '\0';
	fb->node_count  = 0;
	filebrowser_load(fb);
}

