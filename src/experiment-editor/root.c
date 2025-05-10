#include <sdk.h>
#include <stdio.h>

#include "../../src/experiment/tile.h"
#include <stdlib.h>
#include <string.h>

#define WHITE rgb_to_rgb565(255, 255, 255)

extern sdk_scene_t scene_level;

static uint8_t map_ids[MAX_MAPS];
static int map_count = 0;
static int map_selected = 0;

Tile level[MAP_ROWS][MAP_COLS];
int level_id = -1;

static int bayer4x4[4][4] = {
	{ 0, 8, 2, 10 },
	{ 12, 4, 14, 6 },
	{ 3, 11, 1, 9 },
	{ 15, 7, 13, 5 },
};

static FIL file;

static void root_paint(float dt, int depth)
{
	(void)dt;
	(void)depth;

	if (0 != depth)
		return;

	for (int y = 0; y < TFT_HEIGHT; y++) {
		int base_gray = TFT_HEIGHT + 64 - y;

		for (int x = 0; x < TFT_WIDTH; x++) {
			int gray = base_gray - bayer4x4[y & 3][x & 3];
			uint16_t color = rgb_to_rgb565(gray >> 2, gray >> 1, gray);
			tft_draw_pixel(x, y, color);
		}
	}

	if (!map_count) {
		tft_draw_string_center(TFT_WIDTH / 2, TFT_HEIGHT / 2 - 8, WHITE, "No maps.");
		return;
	}

	int page = map_selected / 6;

	tft_draw_rect(4, 12, 25, 12 + 16 * 6 - 1, rgb_to_rgb565(0, 0, 63));

	for (int i = 0; i < 6; i++) {
		int idx = (page * 6) + i;

		if (idx >= map_count)
			break;

		if (idx == map_selected)
			tft_draw_rect(4, 12 + 16 * i, 25, 12 + 16 * (i + 1) - 1,
				      rgb_to_rgb565(63, 63, 255));

		tft_draw_string(8, 12 + 16 * i, WHITE, "%02x", map_ids[idx]);
	}

	if (level_id != map_selected) {
		int err;
		char path[32];
		sprintf(path, "/assets/maps/map%02x.bin", map_ids[map_selected]);

		if ((err = f_open(&file, path, FA_OPEN_ALWAYS | FA_READ))) {
			printf("f_open %s failed: %s\n", path, f_strerror(err));
			goto after_level;
		}

		memset(level, 0, sizeof(level));

		unsigned rb;
		if ((err = f_read(&file, level, sizeof(level), &rb))) {
			printf("f_read %s failed: %s\n", path, f_strerror(err));
			goto after_level;
		}

		if ((err = f_close(&file))) {
			printf("f_close %s failed: %s\n", path, f_strerror(err));
			// We have the data, so no need to skip I guess.
		}

		level_id = map_selected;
	}

	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			uint32_t tile_id = level[y][x].tile_id;
			color_t color = (tile_id * 0x9e3779b9) >> 16;
			int x0 = 36 + x * 6;
			int y0 = 12 + y * 6;
			tft_draw_rect(x0, y0, x0 + 5, y0 + 5, color);
		}
	}

after_level:
}

static int cmp_uint8(const void *a, const void *b)
{
	return (int)*(uint8_t *)a - (int)*(uint8_t *)b;
}

static bool root_handle(sdk_event_t event)
{
	switch (event) {
	case SDK_TICK_SOUTH:
		map_selected = (map_selected + 1) % map_count;
		return true;

	case SDK_TICK_NORTH:
		map_selected = (map_selected + map_count - 1) % map_count;
		return true;

	case SDK_TICK_EAST:
		map_selected = clamp(map_selected + 6, 0, map_count - 1);
		return true;

	case SDK_TICK_WEST:
		map_selected = clamp(map_selected - 6, 0, map_count - 1);
		return true;

	case SDK_PRESSED_START:
	case SDK_PRESSED_A:
		sdk_scene_push(&scene_level);
		return true;

	default:
		return false;
	}
}

static void root_pushed(void)
{
	int err;
	DIR dir;

	map_count = 0;

	if ((err = f_opendir(&dir, "/assets/maps"))) {
		printf("f_opendir /assets/maps failed: %s\n", f_strerror(err));
		goto fail;
	}

	for (int i = 0; i < MAX_MAPS; i++) {
		FILINFO info;

		if ((err = f_readdir(&dir, &info))) {
			printf("f_readdir /assets/maps failed: %s\n", f_strerror(err));
			goto close_fail;
		}

		if (!info.fname[0])
			break;

		uint8_t id;
		if (1 != sscanf(info.fname, "map%02hhx.bin", &id))
			continue;

		if (id >= MAX_MAPS)
			continue;

		map_ids[map_count++] = id;
	}

	qsort(map_ids, map_count, sizeof(*map_ids), cmp_uint8);

close_fail:
	if ((err = f_closedir(&dir))) {
		printf("f_closedir /assets/maps failed: %s\n", f_strerror(err));
		goto fail;
	}

fail:

	map_selected = clamp(map_selected, 0, map_count);
	return;
}

sdk_scene_t scene_root = {
	.paint = root_paint,
	.handle = root_handle,
	.pushed = root_pushed,
};
