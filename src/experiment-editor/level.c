#include <pico/stdlib.h>
#include <sdk.h>
#include <string.h>
#include <tft.h>
#include <sdk.h>
#include <stdlib.h>
#include <stdio.h>

#include "../../src/experiment/tile.h"

#include <tileset.png.h>

#define WHITE rgb_to_rgb565(255, 255, 255)
#define GRAY rgb_to_rgb565(127, 127, 127)

static int sel_x = 0;
static int sel_y = 0;

static float cur_x = 0;
static float cur_y = 0;

static int moving_x = -1;
static int moving_y = -1;

extern Tile level[MAP_ROWS][MAP_COLS];
extern int level_id;

static int saving = -1;

static FIL file;

static void save_level(void)
{
	int err;
	char path[32];
	sprintf(path, "/assets/maps/map%02x.bin", level_id);

	if ((err = f_open(&file, path, FA_CREATE_ALWAYS | FA_WRITE))) {
		printf("f_open %s failed: %s\n", path, f_strerror(err));
		return;
	}

	unsigned wb;
	if ((err = f_write(&file, level, sizeof(level), &wb))) {
		printf("f_write %s failed: %s\n", path, f_strerror(err));
		goto fail;
	}

	printf("Wrote %s\n", path);

fail:
	if ((err = f_close(&file)))
		printf("f_close %s failed: %s\n", path, f_strerror(err));
}

static void level_paint(float dt, int depth)
{
	(void)dt;
	(void)depth;

	tft_fill(GRAY);

	if (moving_x >= 0) {
		level[moving_y][moving_x].px = sel_x;
		level[moving_y][moving_x].py = sel_y;
	}

	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE, &ts_tileset_png,
				      level[y][x].tile_id);

			if (level[y][x].collides_left) {
				tft_draw_rect(x * TILE_SIZE, y * TILE_SIZE, x * TILE_SIZE,
					      (y + 1) * TILE_SIZE - 1, rgb_to_rgb565(255, 0, 0));
			}

			if (level[y][x].collides_right) {
				tft_draw_rect((x + 1) * TILE_SIZE - 1, y * TILE_SIZE,
					      (x + 1) * TILE_SIZE - 1, (y + 1) * TILE_SIZE - 1,
					      rgb_to_rgb565(255, 0, 0));
			}

			if (level[y][x].collides_up) {
				tft_draw_rect(x * TILE_SIZE, y * TILE_SIZE, (x + 1) * TILE_SIZE - 1,
					      y * TILE_SIZE, rgb_to_rgb565(255, 0, 0));
			}

			if (level[y][x].collides_down) {
				tft_draw_rect(x * TILE_SIZE, (y + 1) * TILE_SIZE - 1,
					      (x + 1) * TILE_SIZE - 1, (y + 1) * TILE_SIZE - 1,
					      rgb_to_rgb565(255, 0, 0));
			}
		}
	}

	Tile tile = level[sel_y][sel_x];

	tft_draw_rect(sel_x * TILE_SIZE + TILE_SIZE / 2.0 - 1,
		      sel_y * TILE_SIZE + TILE_SIZE / 2.0 - 1, sel_x * TILE_SIZE + TILE_SIZE / 2.0,
		      sel_y * TILE_SIZE + TILE_SIZE / 2.0,
		      moving_x < 0 ? rgb_to_rgb565(255, 63, 63) : rgb_to_rgb565(63, 63, 255));

	char text[32] = "";

	if (tile.effect == TILE_EFFECT_TELEPORT) {
		snprintf(text, sizeof text, "%i ->%i", tile.tile_id, tile.map);
	} else if (tile.effect == TILE_EFFECT_DAMAGE) {
		snprintf(text, sizeof text, "%i !%i", tile.tile_id, tile.damage);
	} else {
		snprintf(text, sizeof text, "%i", tile.tile_id);
	}

	if (sel_x >= MAP_COLS / 2) {
		if (sel_y >= MAP_ROWS / 2) {
			tft_draw_string_right(sel_x * TILE_SIZE + TILE_SIZE,
					      sel_y * TILE_SIZE - 2 * TILE_SIZE,
					      rgb_to_rgb565(255, 63, 63), "%s", text);
		} else {
			tft_draw_string_right(sel_x * TILE_SIZE + TILE_SIZE,
					      sel_y * TILE_SIZE + TILE_SIZE,
					      rgb_to_rgb565(255, 63, 63), "%s", text);
		}
	} else {
		if (sel_y >= MAP_ROWS / 2) {
			tft_draw_string(sel_x * TILE_SIZE, sel_y * TILE_SIZE - 2 * TILE_SIZE,
					rgb_to_rgb565(255, 63, 63), "%s", text);
		} else {
			tft_draw_string(sel_x * TILE_SIZE, sel_y * TILE_SIZE + TILE_SIZE,
					rgb_to_rgb565(255, 63, 63), "%s", text);
		}
	}

	if (saving >= 0) {
		tft_draw_string_center(TFT_WIDTH / 2, TFT_HEIGHT / 2 - 8, WHITE, "Saving in %3.1f",
				       3.0f - 3.0f * saving / (1 << 16));

		saving += dt * (1 << 16);

		if (saving > (1 << 16)) {
			saving = -1;
			save_level();
		}
	}
}

static bool level_handle(sdk_event_t event)
{
	switch (event) {
	case SDK_READ_JOYSTICK:
		if (abs(sdk_inputs.joy_x) > 200)
			cur_x += sdk_inputs_delta.joy_x / 192.0f;

		if (abs(sdk_inputs.joy_y) > 200)
			cur_y += sdk_inputs_delta.joy_y / 192.0f;

		cur_x = clamp(cur_x, 0, MAP_COLS - 0.001);
		cur_y = clamp(cur_y, 0, MAP_ROWS - 0.001);

		if (abs(sdk_inputs.joy_x) < 200 && abs(sdk_inputs.joy_y) < 200) {
			cur_x = (int)cur_x + 0.5;
			cur_y = (int)cur_y + 0.5;
		}

		sel_x = cur_x;
		sel_y = cur_y;
		return true;

	case SDK_PRESSED_A:
		level[sel_y][sel_x].tile_id =
			(level[sel_y][sel_x].tile_id + 1) % ts_tileset_png.count;
		return true;

	case SDK_PRESSED_Y:
		memset(&level[sel_y][sel_x], 0, sizeof(Tile));
		return true;

	case SDK_PRESSED_X:
		if (moving_x < 0 && TILE_EFFECT_TELEPORT == level[sel_y][sel_x].effect) {
			moving_x = sel_x;
			moving_y = sel_y;
		} else {
			moving_x = -1;
			moving_y = -1;
		}
		return true;

	case SDK_PRESSED_AUX1:
		level[sel_y][sel_x].collides_left = !level[sel_y][sel_x].collides_left;
		return true;

	case SDK_PRESSED_AUX2:
		level[sel_y][sel_x].collides_up = !level[sel_y][sel_x].collides_up;
		return true;

	case SDK_PRESSED_AUX3:
		level[sel_y][sel_x].collides_down = !level[sel_y][sel_x].collides_down;
		return true;

	case SDK_PRESSED_AUX4:
		level[sel_y][sel_x].collides_right = !level[sel_y][sel_x].collides_right;
		return true;

	case SDK_PRESSED_AUX5:
		level[sel_y][sel_x].effect = (level[sel_y][sel_x].effect + 1) % NUM_TILE_EFFECTS;
		return true;

	case SDK_PRESSED_AUX6:
		if (level[sel_y][sel_x].effect == TILE_EFFECT_TELEPORT) {
			level[sel_y][sel_x].map = (level[sel_y][sel_x].map + 1) % MAX_MAPS;
		}
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

static void level_pushed(void)
{
}

sdk_scene_t scene_level = {
	.paint = level_paint,
	.handle = level_handle,
	.pushed = level_pushed,
};
