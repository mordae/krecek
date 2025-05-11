#include "common.h"
#include <math.h>

#define WHITE rgb_to_rgb565(255, 255, 255)
#define GREEN rgb_to_rgb565(0, 191, 0)

static bool inside_map = false;
static int selected = 0;

static int bayer4x4[4][4] = {
	{ 0, 8, 2, 10 },
	{ 12, 4, 14, 6 },
	{ 3, 11, 1, 9 },
	{ 15, 7, 13, 5 },
};

static void target_paint(float dt, int depth)
{
	(void)dt;
	(void)depth;

	if (0 != depth)
		return;

	for (int y = 0; y < TFT_HEIGHT; y++) {
		int base_gray = TFT_HEIGHT + 64 - y;

		for (int x = 0; x < TFT_WIDTH; x++) {
			int gray = base_gray - bayer4x4[y & 3][x & 3];
			uint16_t color = rgb_to_rgb565(gray >> 2, gray, gray >> 2);
			tft_draw_pixel(x, y, color);
		}
	}

	if (!map_count) {
		tft_draw_string_center(TFT_WIDTH / 2, TFT_HEIGHT / 2 - 8, WHITE, "No maps.");
		return;
	}

	/* Make sure we have the right map loaded. */
	target_cursor.level = map_ids[selected];
	load_level(&target_cursor);

	int page = selected / 6;

	tft_draw_rect(4, 12, 25, 12 + 16 * 6 - 1, rgb_to_rgb565(0, 63, 0));

	for (int i = 0; i < 6; i++) {
		int idx = (page * 6) + i;

		if (idx >= map_count)
			break;

		if (idx == selected)
			tft_draw_rect(4, 12 + 16 * i, 25, 12 + 16 * (i + 1) - 1,
				      rgb_to_rgb565(63, 191, 63));

		tft_draw_string(8, 12 + 16 * i, WHITE, "%02x", map_ids[idx]);
	}

	tft_draw_rect(34, 12, 157, 105, inside_map ? WHITE : GREEN);

	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			uint32_t tile_id = target_cursor.map[y][x].tile_id;
			color_t color = preview_colors[tile_id];
			int x0 = 36 + x * 6;
			int y0 = 14 + y * 6;
			tft_draw_rect(x0, y0, x0 + 5, y0 + 5, color);
		}
	}

	float light = sinf(((time_us_32() << 12) >> 12) * 2 * M_PI / (1 << 20));
	int gray = 128 + 127 * light;
	color_t color = rgb_to_rgb565(gray, 255, gray);

	tft_draw_rect(36 + target_cursor.px * 6, 14 + target_cursor.py * 6,
		      36 + (target_cursor.px + 1) * 6 - 1, 14 + (target_cursor.py + 1) * 6 - 1,
		      inside_map ? color : WHITE);
}

static bool target_handle(sdk_event_t event, int depth)
{
	if (depth)
		return false;

	switch (event) {
	case SDK_TICK_NORTH:
		if (inside_map) {
			target_cursor.py = clamp(target_cursor.py - 1, 0, MAP_ROWS - 1);
		} else {
			selected = (selected + map_count - 1) % map_count;
		}
		return true;

	case SDK_TICK_SOUTH:
		if (inside_map) {
			target_cursor.py = clamp(target_cursor.py + 1, 0, MAP_ROWS - 1);
		} else {
			selected = (selected + 1) % map_count;
		}
		return true;

	case SDK_TICK_WEST:
		if (inside_map) {
			target_cursor.px = clamp(target_cursor.px - 1, 0, MAP_COLS - 1);
		} else {
			selected = clamp(selected - 6, 0, map_count - 1);
		}
		return true;

	case SDK_TICK_EAST:
		if (inside_map) {
			target_cursor.px = clamp(target_cursor.px + 1, 0, MAP_COLS - 1);
		} else {
			selected = clamp(selected + 6, 0, map_count - 1);
		}
		return true;

	case SDK_PRESSED_SELECT:
	case SDK_PRESSED_B:
		if (inside_map) {
			inside_map = false;
		} else {
			target_cursor.level = NO_LEVEL;
			sdk_scene_pop();
		}
		return true;

	case SDK_PRESSED_START:
	case SDK_PRESSED_A:
		if (inside_map) {
			sdk_scene_pop();
		} else {
			inside_map = true;
		}
		return true;

	default:
		return false;
	}
}

static void target_pushed(void)
{
	// Make sure the root cursor has correct map data.
	load_level(&root_cursor);

	// Inspect the tile we came from.
	const Tile *tile = &root_cursor.map[root_cursor.py][root_cursor.px];

	if (TILE_EFFECT_TELEPORT == tile->effect) {
		// We came from a teleport tile, set our cursor to its target.
		target_cursor.level = tile->map;
		target_cursor.px = tile->px;
		target_cursor.py = tile->py;
	} else {
		// Otherwise just begin on the same map.
		target_cursor = root_cursor;
	}

	// Update the map selection as well.
	selected = 0;

	for (int i = 0; i < map_count; i++) {
		if (map_ids[i] == target_cursor.level) {
			selected = i;
			break;
		}
	}

	// We are initially not moving the cursor, but selecting the map.
	inside_map = false;
}

sdk_scene_t scene_target = {
	.paint = target_paint,
	.handle = target_handle,
	.pushed = target_pushed,
};
