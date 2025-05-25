#include "common.h"
#include <assert.h>
#include <stdint.h>
#include <sdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tileset.png.h>

int map_count = 0;
uint8_t map_ids[MAX_MAPS];

color_t preview_colors[TS_TILESET_PNG_COUNT];

struct cursor root_cursor = { .level = NO_LEVEL, .has_level = -NO_LEVEL };
struct cursor target_cursor = { .level = NO_LEVEL, .has_level = -NO_LEVEL };

static FIL file;

void load_level(struct cursor *cursor)
{
	assert(cursor);
	assert(cursor->level >= 0);
	assert(cursor->level < MAX_MAPS);

	if (abs(cursor->has_level) == cursor->level)
		return;

	memset(cursor->map, 0, sizeof(cursor->map));
	cursor->has_level = cursor->level;

	int err;
	char path[32];
	sprintf(path, "/assets/maps/map%02x.bin", cursor->level);

	if ((err = f_open(&file, path, FA_OPEN_ALWAYS | FA_READ))) {
		printf("f_open %s failed: %s\n", path, f_strerror(err));
		cursor->has_level = -cursor->level;
		return;
	}

	unsigned rb;
	if ((err = f_read(&file, cursor->map, sizeof(cursor->map), &rb))) {
		printf("f_read %s failed: %s\n", path, f_strerror(err));
		cursor->has_level = -cursor->level;
	}

	if ((err = f_close(&file))) {
		printf("f_close %s failed: %s\n", path, f_strerror(err));
		cursor->has_level = -cursor->level;
	}
}

void save_level(struct cursor *cursor)
{
	assert(cursor);
	assert(cursor->level >= 0);
	assert(cursor->level < MAX_MAPS);

	int err;
	char path[32];
	sprintf(path, "/assets/maps/map%02x.bin", cursor->level);

	if ((err = f_open(&file, path, FA_CREATE_ALWAYS | FA_WRITE))) {
		printf("f_open %s failed: %s\n", path, f_strerror(err));
		return;
	}

	unsigned wb;
	if ((err = f_write(&file, cursor->map, sizeof(cursor->map), &wb))) {
		printf("f_write %s failed: %s\n", path, f_strerror(err));
		goto fail;
	}

	printf("Wrote %s\n", path);

fail:
	if ((err = f_close(&file)))
		printf("f_close %s failed: %s\n", path, f_strerror(err));
}

static int cmp_uint8(const void *a, const void *b)
{
	return (int)*(uint8_t *)a - (int)*(uint8_t *)b;
}

void load_map_ids(void)
{
	int err;
	static DIR dir;

	map_count = 0;

	if ((err = f_opendir(&dir, "/assets/maps"))) {
		printf("f_opendir /assets/maps failed: %s\n", f_strerror(err));
		return;
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
	}
}

void load_preview_colors(void)
{
	int pixels = ts_tileset_png.width * ts_tileset_png.height;

	for (int i = 0; i < TS_TILESET_PNG_COUNT; i++) {
		const color_t *data = sdk_get_tile_data(&ts_tileset_png, i);
		int r = 0, g = 0, b = 0;

		for (int j = 0; j < pixels; j++) {
			color_t px = data[j];
			r += (px & 0b1111100000000000) >> 11;
			g += (px & 0b0000011111100000) >> 5;
			b += (px & 0b0000000000011111) >> 0;
		}

		preview_colors[i] = rgb565(r / pixels, g / pixels, b / pixels);
	}
}
