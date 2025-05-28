#include <string.h>
#include <stdio.h>

#include "common.h"

#include <tileset.png.h>

#define WHITE rgb_to_rgb565(255, 255, 255)
#define GRAY rgb_to_rgb565(127, 127, 127)

static int saving = -1;
static int saved_tile_id = 0;

static void level_paint(float dt, int depth)
{
	(void)dt;
	(void)depth;

	if (depth)
		return;

	tft_fill(GRAY);

	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE, &ts_tileset_png,
				      root_cursor.map[y][x].tile_id);

			if (root_cursor.map[y][x].collides_left) {
				tft_draw_rect(x * TILE_SIZE, y * TILE_SIZE, x * TILE_SIZE,
					      (y + 1) * TILE_SIZE - 1, rgb_to_rgb565(255, 0, 0));
			}

			if (root_cursor.map[y][x].collides_right) {
				tft_draw_rect((x + 1) * TILE_SIZE - 1, y * TILE_SIZE,
					      (x + 1) * TILE_SIZE - 1, (y + 1) * TILE_SIZE - 1,
					      rgb_to_rgb565(255, 0, 0));
			}

			if (root_cursor.map[y][x].collides_up) {
				tft_draw_rect(x * TILE_SIZE, y * TILE_SIZE, (x + 1) * TILE_SIZE - 1,
					      y * TILE_SIZE, rgb_to_rgb565(255, 0, 0));
			}

			if (root_cursor.map[y][x].collides_down) {
				tft_draw_rect(x * TILE_SIZE, (y + 1) * TILE_SIZE - 1,
					      (x + 1) * TILE_SIZE - 1, (y + 1) * TILE_SIZE - 1,
					      rgb_to_rgb565(255, 0, 0));
			}
		}
	}

	Tile *tile = &root_cursor.map[root_cursor.py][root_cursor.px];

	tft_draw_rect(root_cursor.px * TILE_SIZE + TILE_SIZE / 2.0 - 1,
		      root_cursor.py * TILE_SIZE + TILE_SIZE / 2.0 - 1,
		      root_cursor.px * TILE_SIZE + TILE_SIZE / 2.0,
		      root_cursor.py * TILE_SIZE + TILE_SIZE / 2.0, rgb_to_rgb565(255, 63, 63));

	char text[32] = "";

	if (tile->effect == TILE_EFFECT_TELEPORT) {
		snprintf(text, sizeof text, "%i =%02x", tile->tile_id, tile->map);
	} else if (tile->effect == TILE_EFFECT_DAMAGE) {
		snprintf(text, sizeof text, "%i !%i", tile->tile_id, tile->damage);
	} else {
		snprintf(text, sizeof text, "%i", tile->tile_id);
	}

	if (root_cursor.px >= MAP_COLS / 2) {
		if (root_cursor.py >= MAP_ROWS / 2) {
			tft_draw_string_right(root_cursor.px * TILE_SIZE + TILE_SIZE,
					      root_cursor.py * TILE_SIZE - 2 * TILE_SIZE,
					      rgb_to_rgb565(255, 63, 63), "%s", text);
		} else {
			tft_draw_string_right(root_cursor.px * TILE_SIZE + TILE_SIZE,
					      root_cursor.py * TILE_SIZE + TILE_SIZE,
					      rgb_to_rgb565(255, 63, 63), "%s", text);
		}
	} else {
		if (root_cursor.py >= MAP_ROWS / 2) {
			tft_draw_string(root_cursor.px * TILE_SIZE,
					root_cursor.py * TILE_SIZE - 2 * TILE_SIZE,
					rgb_to_rgb565(255, 63, 63), "%s", text);
		} else {
			tft_draw_string(root_cursor.px * TILE_SIZE,
					root_cursor.py * TILE_SIZE + TILE_SIZE,
					rgb_to_rgb565(255, 63, 63), "%s", text);
		}
	}

	if (saving >= 0) {
		tft_draw_string_center(TFT_WIDTH / 2, TFT_HEIGHT / 2 - 8, WHITE, "Saving in %3.1f",
				       3.0f - 3.0f * saving / (1 << 16));

		saving += dt * (1 << 16);

		if (saving > (1 << 16)) {
			saving = -1;
			save_level(&root_cursor);
		}
	}
}

static bool level_handle(sdk_event_t event, int depth)
{
	if (depth)
		return false;

	Tile *tile = &root_cursor.map[root_cursor.py][root_cursor.px];

	switch (event) {
	case SDK_TICK_NORTH:
		root_cursor.py = clamp(root_cursor.py - 1, 0, MAP_ROWS - 1);
		return true;

	case SDK_TICK_SOUTH:
		root_cursor.py = clamp(root_cursor.py + 1, 0, MAP_ROWS - 1);
		return true;

	case SDK_TICK_WEST:
		root_cursor.px = clamp(root_cursor.px - 1, 0, MAP_COLS - 1);
		return true;

	case SDK_TICK_EAST:
		root_cursor.px = clamp(root_cursor.px + 1, 0, MAP_COLS - 1);
		return true;

	case SDK_PRESSED_A:
		tile->tile_id = (tile->tile_id + 1) % ts_tileset_png.count;
		return true;

	case SDK_PRESSED_B:
		tile->tile_id = (ts_tileset_png.count + tile->tile_id - 1) % ts_tileset_png.count;
		return true;

	case SDK_PRESSED_Y:
		memset(tile, 0, sizeof(*tile));
		return true;

	case SDK_PRESSED_X:
		sdk_scene_push(&scene_target);
		return true;

	case SDK_PRESSED_AUX1:
		tile->collides_left ^= 1;
		return true;

	case SDK_PRESSED_AUX2:
		tile->collides_up ^= 1;
		return true;

	case SDK_PRESSED_AUX3:
		tile->collides_down ^= 1;
		return true;

	case SDK_PRESSED_AUX4:
		tile->collides_right ^= 1;
		return true;

	case SDK_PRESSED_AUX5:
		saved_tile_id = tile->tile_id;
		return true;

	case SDK_PRESSED_AUX6:
		tile->tile_id = saved_tile_id;
		return true;

	case SDK_PRESSED_SELECT:
		sdk_scene_pop();
		return true;

	case SDK_PRESSED_START:
		saving = 0;
		return true;

	case SDK_RELEASED_START:
		saving = -1;
		return true;

	default:
		return false;
	}
}

static void level_revealed(void)
{
	if (target_cursor.level < 0 || target_cursor.level >= MAX_MAPS)
		return;

	Tile *tile = &root_cursor.map[root_cursor.py][root_cursor.px];
	tile->effect = TILE_EFFECT_TELEPORT;
	tile->map = target_cursor.level;
	tile->px = target_cursor.px;
	tile->py = target_cursor.py;
}

static void level_pushed(void)
{
}

sdk_scene_t scene_level = {
	.paint = level_paint,
	.handle = level_handle,
	.pushed = level_pushed,
	.revealed = level_revealed,
};
