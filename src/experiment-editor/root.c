#include "common.h"

#define WHITE rgb_to_rgb565(255, 255, 255)

static int selected = 0;

static int bayer4x4[4][4] = {
	{ 0, 8, 2, 10 },
	{ 12, 4, 14, 6 },
	{ 3, 11, 1, 9 },
	{ 15, 7, 13, 5 },
};

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

	/* Make sure we have the right map loaded. */
	root_cursor.level = map_ids[selected];
	load_level(&root_cursor);

	int page = selected / 6;

	tft_draw_rect(4, 12, 25, 12 + 16 * 6 - 1, rgb_to_rgb565(0, 0, 63));

	for (int i = 0; i < 6; i++) {
		int idx = (page * 6) + i;

		if (idx >= map_count)
			break;

		if (idx == selected)
			tft_draw_rect(4, 12 + 16 * i, 25, 12 + 16 * (i + 1) - 1,
				      rgb_to_rgb565(63, 63, 255));

		tft_draw_string(8, 12 + 16 * i, WHITE, "%02x", map_ids[idx]);
	}

	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			uint32_t tile_id = root_cursor.map[y][x].tile_id;
			color_t color = (tile_id * 0x9e3779b9) >> 16;
			int x0 = 36 + x * 6;
			int y0 = 12 + y * 6;
			tft_draw_rect(x0, y0, x0 + 5, y0 + 5, color);
		}
	}
}

static bool root_handle(sdk_event_t event, int depth)
{
	if (depth)
		return false;

	switch (event) {
	case SDK_TICK_NORTH:
		selected = (selected + map_count - 1) % map_count;
		return true;

	case SDK_TICK_SOUTH:
		selected = (selected + 1) % map_count;
		return true;

	case SDK_TICK_WEST:
		selected = clamp(selected - 6, 0, map_count - 1);
		return true;

	case SDK_TICK_EAST:
		selected = clamp(selected + 6, 0, map_count - 1);
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
	load_map_ids();
	selected = clamp(selected, 0, map_count);
}

sdk_scene_t scene_root = {
	.paint = root_paint,
	.handle = root_handle,
	.pushed = root_pushed,
};
