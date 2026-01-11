#include "config.h"

#include <string.h>

#define CONFIG_FILE_IMPLEMENTATION
#include "libcutils/config_file.h"


struct Config g_config = {0};

static Result config_parse_color(struct Config_File *cfg, struct Color *color, const char *key)
{
	const char *value = config_file_gets(cfg, key);

	if (value == NULL) {
		return result_make(false, "No value for color '%s' is set!", key);
	}

	if (*value != '#') {
		return result_make(false, "color strings does not start with '#': %s", value);
	}

	int parsed_values_count  = sscanf(value+1, "%2x%2x%2x%2x", (unsigned int*)&color->r, (unsigned int*)&color->g, (unsigned int*)&color->b, (unsigned int*)&color->a);

	if (parsed_values_count != 4) {
		return result_make(false, "Expected to parse 4 hex values but got %d: %s", parsed_values_count, value);
	}

	return result_make_success();
}

Result config_load_colorscheme(const char *filepath)
{
	struct Config_File cfg = {0};
	Result r = config_file_init(&cfg, filepath);
	if (!r.success) return r;

	r = config_parse_color(&cfg, &g_config.screen_color_primary, "screen_color_primary");
	if (!r.success) return r;

	r = config_parse_color(&cfg, &g_config.screen_color_highlight, "screen_color_highlight");
	if (!r.success) return r;

	r = config_parse_color(&cfg, &g_config.screen_color_background, "screen_color_background");
	if (!r.success) return r;

	return result_make_success();
}

Result config_init(const char *resources_path)
{
	strncpy(g_config.resources_dir, resources_path, sizeof(g_config.resources_dir));

	char config_file_path[1024];
	snprintf(config_file_path, sizeof(config_file_path), "%s/shard-os.conf", g_config.resources_dir);

	struct Config_File cfg = {0};
	Result r = config_file_init(&cfg, config_file_path);
	if (!r.success) return r;

	snprintf(config_file_path, sizeof(config_file_path), "%s/%s",
		g_config.resources_dir, config_file_gets(&cfg, "screen_colorscheme"));
	r = config_load_colorscheme(config_file_path);
	if (!r.success) return r;

	strncpy(g_config.font_file, config_file_gets(&cfg, "screen_font_file"), sizeof(g_config.font_file));

	g_config.screen_font_size_xl = config_file_geti(&cfg, "screen_font_size_xl");
	g_config.screen_font_size_l  = config_file_geti(&cfg, "screen_font_size_l");
	g_config.screen_font_size_m  = config_file_geti(&cfg, "screen_font_size_m");
	g_config.screen_font_size_s  = config_file_geti(&cfg, "screen_font_size_s");
	g_config.screen_font_size_xs = config_file_geti(&cfg, "screen_font_size_xs");

	strncpy(g_config.audio_device_name, config_file_gets(&cfg, "audio_device_name"), sizeof(g_config.audio_device_name));
	return result_make_success();
}
