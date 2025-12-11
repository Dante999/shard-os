#ifndef FILEBROWSER_H
#define FILEBROWSER_H

#include <stddef.h>

enum Node_Type {
	NODE_TYPE_FILE,
	NODE_TYPE_DIR
};

struct Node {
	char name[1024];
	enum Node_Type type;
};

struct Filebrowser {
	char root_path[2048];
	char sub_path[2048];
	struct Node nodes[40];
	size_t node_count;
};

void filebrowser_init(struct Filebrowser *fb, const char *root_path);
void filebrowser_enter(struct Filebrowser *fb, const char *dir_name);

#endif // FILEBROWSER_H
