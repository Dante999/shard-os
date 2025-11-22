#define UTIL_STRINGS_IMPLEMENTATION
#include "libcutils/util_strings.h"

#define RESULT_IMPLEMENTATION
#include "libcutils/result.h"

#define LOGGER_IMPLEMENTATION
#include "libcutils/logger.h"

#include "config.h"
#include "screen.h"
#include "ui_main.h"

#include <stdio.h>

#define SCREEN_SCALE  1.0f
#define SCREEN_WIDTH  ((int)1024*SCREEN_SCALE)
#define SCREEN_HEIGHT ((int)600*SCREEN_SCALE)


int main(int argc, char *argv[])
{
	log_info("Application started!\n", NULL);

	char resources_path[255] = "../resources";

	if (argc > 1) {
		strncpy(resources_path, argv[1], sizeof(resources_path));
	}
	Result result = config_init(resources_path);

	if (!result.success) {
		log_error("Failed to load config: %s\n", result.msg);
		return 1;
	}

	struct Screen screen = {0};

	screen_init(&screen, SCREEN_WIDTH, SCREEN_HEIGHT);
	if (!result.success) {
		log_error("failed to load screen: %s\n", result.msg);
		return 1;
	}

	ui_main_init(&screen);

	while (!screen.quit) {
		screen_rendering_start(&screen);

		ui_main_draw_header(&screen);

		screen_rendering_stop(&screen);

	}

	screen_destroy(&screen);
}
