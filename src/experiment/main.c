#include <pico/stdlib.h>
#include <math.h>

#include <sdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tft.h>

#include "tile.h"

#include <tileset.png.h>
#include <player.png.h>
#include <swing.png.h>
#include <enemies/blob.png.h>

#include <maps/map00.bin.h>
#include <maps/map01.bin.h>

#include <cover.png.h>

sdk_game_info("experiment", &image_cover_png);

#define GRAY rgb_to_rgb565(127, 127, 127)

#define TILE_SIZE 8

#define NUM_MAPS 2
static const void *maps[NUM_MAPS] = {
	maps_map00_bin,
	maps_map01_bin,
};

static const Tile (*map)[MAP_COLS] = (const void *)maps_map00_bin;

typedef enum EnemyTypeId {
	ENEMY_BLOB = 0,
	ENEMY_SLIME_BOSS,
	NUM_ENEMY_TYPES,
} EnemyTypeId;

typedef struct EnemyType {
	const sdk_tileset_t *ts;
	int max_hp;
} EnemyType;

typedef struct Enemy {
	const EnemyType *type;
	sdk_sprite_t s;
	int hp;
} Enemy;

static const EnemyType enemy_type[NUM_ENEMY_TYPES] = {
	[ENEMY_BLOB] = {
		.max_hp = 100,
		.ts = &ts_enemies_blob_png,
	},
	[ENEMY_SLIME_BOSS] = {
		.max_hp = 1000,
		.ts = &ts_enemies_blob_png,
	},
};

#define NUM_ENEMIES 16
static Enemy enemy[NUM_ENEMIES];

#define SWING_START (1 << 17)
#define SWING_SHIFT 16

#define SHAKEN_START (1 << 18)

typedef struct Character {
	sdk_sprite_t s;
	sdk_sprite_t swing;
	int swing_phase;
	float speed;
	int shaken_phase;
} Character;

static Character player = {
	.s = {
		.ts = &ts_player_png,
		.ox = 3.5f,
		.oy = 6.5f,
	},
	.swing = {
		.ts = &ts_swing_png,
		.ox = 7.5f,
		.oy = 7.5f,
	},
};

void game_reset(void)
{
}

void game_start(void)
{
	player.s.tile = 0;
	player.s.x = TFT_WIDTH / 2.0f;
	player.s.y = TFT_HEIGHT / 2.0f;
	player.speed = 50.0f;
}

static void change_map(int map_id, int px, int py)
{
	// Change map
	map = maps[map_id];

	// Move player to destination coordinates
	player.s.x = (TILE_SIZE * px) + 3.5f;
	player.s.y = (TILE_SIZE * py) + 3.5f;

	int spawned = 0;

	memset(enemy, 0, sizeof(enemy));

	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			Tile tile = map[y][x];

			if (TILE_EFFECT_SPAWN != tile.effect)
				continue;

			// TODO: Add argument to select different enemy type.
			const EnemyType *type = &enemy_type[ENEMY_BLOB];
			enemy[spawned].type = type;
			enemy[spawned].hp = type->max_hp;
			enemy[spawned].s.ts = type->ts;
			enemy[spawned].s.x = x * TILE_SIZE + 3.5f;
			enemy[spawned].s.y = y * TILE_SIZE + 3.5f;
			enemy[spawned].s.ox = 3.5f;
			enemy[spawned].s.oy = 3.5f;

			spawned++;

			// Don't spawn more than we have space for.
			if (spawned >= NUM_ENEMIES)
				break;
		}
	}
}

static bool player_collides_with_an_enemy(void)
{
	for (int i = 0; i < NUM_ENEMIES; i++) {
		if (!enemy[i].type)
			continue;

		if (sdk_sprites_collide(&player.s, &enemy[i].s))
			return true;
	}

	return false;
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	if (sdk_inputs_delta.a > 0) {
		player.swing_phase = SWING_START;
	}

	if (player.shaken_phase <= 0) {
		float move_x = abs(sdk_inputs.joy_x) >= 512 ?
				       player.speed * dt * sdk_inputs.joy_x / 2048.0f :
				       0;
		float move_y = abs(sdk_inputs.joy_y) >= 512 ?
				       player.speed * dt * sdk_inputs.joy_y / 2048.0f :
				       0;

		float old_x = player.s.x;
		float old_y = player.s.y;

		if ((move_x || move_y)) {
			int pos_x = player.s.x / TILE_SIZE;
			int pos_y = player.s.y / TILE_SIZE;

			float next_x = player.s.x + move_x;
			float next_y = player.s.y + move_y;

			next_x = clamp(next_x, TILE_SIZE / 4.0f, TFT_RIGHT - TILE_SIZE / 4.0f);
			next_y = clamp(next_y, TILE_SIZE / 4.0f, TFT_BOTTOM - TILE_SIZE / 4.0f);
			int next_pos_x = next_x / TILE_SIZE;
			int next_pos_y = next_y / TILE_SIZE;

			if (pos_x != next_pos_x && move_x) {
				if (move_x > 0) {
					// going right, to the next tile
					if (map[pos_y][next_pos_x].collides_left)
						next_x = ceilf(player.s.x) - 1e-3;
				} else {
					// going left, to the next tile
					if (map[pos_y][next_pos_x].collides_right)
						next_x = floorf(player.s.x);
				}
			}

			if (pos_y != next_pos_y && move_y) {
				if (move_y > 0) {
					// going down, to the next tile
					if (map[next_pos_y][pos_x].collides_up)
						next_y = ceilf(player.s.y) - 1e-3;
				} else {
					// going up, to the next tile
					if (map[next_pos_y][pos_x].collides_down)
						next_y = floorf(player.s.y);
				}
			}

			player.s.x = next_x;
			player.s.y = next_y;

			if (fabsf(move_x) > fabsf(move_y)) {
				if (move_x > 0) {
					player.s.tile = 6;
					player.swing.angle = 1;
					player.swing.ox = 4;
					player.swing.oy = 12;
				} else if (move_x < 0) {
					player.s.tile = 4;
					player.swing.angle = 3;
					player.swing.ox = 10;
					player.swing.oy = 10;
				}
			} else if (move_y > 0) {
				player.s.tile = 2;
				player.swing.angle = 2;
				player.swing.ox = 7;
				player.swing.oy = 8;
			} else if (move_y < 0) {
				player.s.tile = 0;
				player.swing.angle = 0;
				player.swing.ox = 9;
				player.swing.oy = 12;
			}

			player.s.tile &= ~1;
			player.s.tile |= (time_us_32() >> 16) & 1;
		}

		if (player_collides_with_an_enemy()) {
			// Minor knock back effect.
			player.s.x -= (player.s.x - old_x) * 5;
			player.s.y -= (player.s.y - old_y) * 5;
			player.shaken_phase = SHAKEN_START;
		}
	} else {
		player.shaken_phase -= dt_usec;
	}

	int pos_x = player.s.x / TILE_SIZE;
	int pos_y = player.s.y / TILE_SIZE;

	static bool fresh_from_teleport = false;
	Tile tile = map[pos_y][pos_x];

	if (tile.effect == TILE_EFFECT_TELEPORT) {
		if (fresh_from_teleport) {
			// Do nothing, we just teleported in.
		} else if (tile.map < NUM_MAPS) {
			change_map(tile.map, tile.px, tile.py);
			fresh_from_teleport = true;
		} else {
			printf("Cannot teleport to map%02x\n", tile.map);
		}
	} else {
		fresh_from_teleport = false;
	}
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(rgb_to_rgb565(0, 0, 0));

	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE, &ts_tileset_png,
				      map[y][x].tile_id);
		}
	}

	for (int i = 0; i < NUM_ENEMIES; i++) {
		if (!enemy[i].type)
			continue;

		if (enemy[i].hp <= 0) {
			enemy[i].type = NULL;
			continue;
		}

		enemy[i].s.tile = (time_us_32() >> 19) & 1;

		sdk_draw_sprite(&enemy[i].s);

		int perc_hp = 100 * enemy[i].hp / enemy[i].type->max_hp;
		color_t color;

		if (perc_hp >= 66)
			color = rgb_to_rgb565(0, 255, 0);
		else if (perc_hp >= 33)
			color = rgb_to_rgb565(255, 255, 0);
		else
			color = rgb_to_rgb565(255, 0, 0);

		int lx = enemy[i].s.x - 3;
		int w = 7 * perc_hp / 100;

		tft_draw_rect(lx, enemy[i].s.y + 5, lx + w, enemy[i].s.y + 5, color);
	}

	if (player.swing_phase > 0) {
		player.swing.tile = 1 - (player.swing_phase >> SWING_SHIFT);
		player.swing.x = player.s.x;
		player.swing.y = player.s.y;
		sdk_draw_sprite(&player.swing);

		if (SWING_START == player.swing_phase) {
			for (int i = 0; i < NUM_ENEMIES; i++) {
				if (!enemy[i].type)
					continue;

				int px = sdk_sprites_collide(&player.swing, &enemy[i].s);
				enemy[i].hp -= px;
			}
		}

		player.swing_phase -= dt_usec;
	}

	sdk_draw_sprite(&player.s);
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = true,
		.fps_color = GRAY,
	};

	sdk_main(&config);
}
