#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <sdk.h>
#include <math.h>
//#include <stdlib.h>
#include <stdio.h>

#include "common.h"

#include <petr.png.h>
#include <kurecidzokej.png.h>
#include <platforms.png.h>
#include <menu.png.h>
#include <levels.png.h>
#include <death.png.h>
// Physics constants
#define GRAVITY 90	  // Gravity for falling
#define JUMP_STRENGTH -80 // Jump strength
#define MAX_SPEED 100	  // Max speed for movement
#define MAX_JOCKEY_SPEED 40
#define MIN_JOCKEY_SPEED 5
#define ACCELERATION 80 // Acceleration for movement
#define FRICTION 8	// Friction to stop sliding quickly

#define multiply332 \
	(x, f) rgb_to_rgb332(rgb332_red((x)) * f, rgb332_green((x)) * f, rgb332_blue((x)) * f)

// Color definitions (using palette indices)
#define RED rgb_to_rgb565(255, 0, 0)
#define RED_POWER rgb_to_rgb565(255, 63, 63)
#define YELLOW rgb_to_rgb565(255, 255, 0)
#define GREEN rgb_to_rgb565(0, 255, 0)
#define GREEN_POWER rgb_to_rgb565(63, 255, 63)
#define BLUE rgb_to_rgb565(0, 0, 255)
#define GRAY rgb_to_rgb565(127, 127, 127)
#define WHITE rgb_to_rgb565(255, 255, 255)

extern TileType maps_map0[MAP_ROWS][MAP_COLS];
extern TileType maps_map1[MAP_ROWS][MAP_COLS];
extern TileType maps_map2[MAP_ROWS][MAP_COLS];
extern TileType maps_map3[MAP_ROWS][MAP_COLS];
extern TileType maps_map4[MAP_ROWS][MAP_COLS];
extern TileType maps_map1c[MAP_ROWS][MAP_COLS];
extern TileType maps_mapwin[MAP_ROWS][MAP_COLS];
extern TileType maps_map2c[MAP_ROWS][MAP_COLS];
extern TileType maps_map3c[MAP_ROWS][MAP_COLS];
extern TileType maps_map4c[MAP_ROWS][MAP_COLS];
TileType (*map)[MAP_COLS] = maps_map1;

// --- Mario's state in the tile map game ---
typedef struct {
	float px, py; // Pixel coordinates
	float vx, vy; // Velocity
	int score;
	int alive;
	int won;
	int friction;
	char mode;
	float time;
	int fast;
	sdk_sprite_t s;
} Mario;

static Mario mario_p;

struct menu {
	int levels;
	int select;
};
static struct menu menu;
typedef struct {
	float vx, vy;
	int alive;
	bool right;
	sdk_sprite_t s;
} Jockey;
static Jockey jockey;
static float volume = SDK_GAIN_STD;

static void enemy_jockey(float dt);

// --- Audio: Updated tune ---
static const char music1[] = "/i:square /bpm:60 "
			     "{ "
			     "ACGgbGA _ ACEGgFD _ ACGgbEC _ FGAGFED _ "
			     "CEGAEC  _ GBgfeD  _ ACGEFGA _ DEFGEDC _ "
			     "ACGgbGA _ ACEGgFD _ ACGgbEC _ FGAGFED _ "
			     "EGCEGA  _ DFABAG  _ CEGBAF  _ DFACBG  _ "
			     "GABCDEF _ EGABCDE _ BAGFEDC _ ACEGFED _ "
			     "CEGAGEC _ GBgfeD  _ ACGEFGA _ DEFGEDC _ "
			     "}";

static const char music2[] = "/i:noise /bpm:60 "
			     "{ "
			     "cc_c_cc _ cc_c_cc _ cc_c_cc _ cc_c_cc _ "
			     "cc_c_c  _ cc_c__  _ cc_c_cc _ cc_c_cc _ "
			     "cc_c_cc _ cc_c_cc _ cc_c_cc _ cc_c_cc _ "
			     "cc_c_c  _ cc_c__  _ cc_c_c  _ cc_c_c  _ "
			     "cc_c_cc _ cc_c_cc _ cc_c_cc _ cc_c_cc _ "
			     "cc_c_cc _ cc_c_c  _ cc_c_cc _ cc_c_cc _ "
			     "}";

// --- Initialize the game ---
void game_start(void)
{
	sdk_set_output_gain_db(volume);
	sdk_melody_play(music1);
	sdk_melody_play(music2);
}

void game_reset(void)
{
	map = maps_map0;
	mario_p.mode = 0;
	menu.select = 0;

	mario_p.s.ts = &ts_petr_png;
	mario_p.s.x = mario_p.px;
	mario_p.s.y = mario_p.py;
	mario_p.s.ox = 3.5;
	mario_p.s.oy = 7;
	mario_p.s.tile = 0;

	jockey.s.ts = &ts_kurecidzokej_png;
	jockey.s.ox = 3.5;
	jockey.s.oy = 7;
	jockey.s.tile = 0;
}

// --- Game Input ---
void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;
	if (map != maps_mapwin) {
		mario_p.time += dt;
	}
	if (mario_p.mode == 4) {
		if (sdk_inputs_delta.start > 0) {
			game_reset();
			return;
		}
	} else if (mario_p.mode == 0) {
		mario_p.fast = 0;
		map = maps_map0;
		if (sdk_inputs_delta.y > 0 || sdk_inputs.joy_y < -500) {
			menu.select -= 1;
		}
		if (sdk_inputs_delta.a > 0 || sdk_inputs.joy_y > 500) {
			menu.select += 1;
		}

		if (menu.select >= 4) {
			menu.select = 0;
		}
		if (menu.select == -1) {
			menu.select = 3;
		}

		if (sdk_inputs_delta.start > 0) {
			if (menu.select == 0) {
				mario_p.px = 1 * TILE_SIZE;
				mario_p.py = 12 * TILE_SIZE;
				mario_p.vx = 0;
				mario_p.vy = 0;
				mario_p.score = 0;
				mario_p.alive = 1;
				mario_p.won = 0;
				mario_p.mode = 1;
				mario_p.fast = 0;
				map = maps_map1;
				jockey.s.y = 14 * TILE_SIZE - 0.1;
				jockey.s.x = 3 * TILE_SIZE;
				jockey.right = 1;
				return;
			} else if (menu.select == 1) {
				mario_p.time = 0;
				mario_p.px = 1 * TILE_SIZE;
				mario_p.py = 12 * TILE_SIZE;
				mario_p.vx = 0;
				mario_p.vy = 0;
				mario_p.score = 0;
				mario_p.alive = 1;
				mario_p.won = 0;
				mario_p.mode = 1;
				mario_p.fast = 1;
				map = maps_map1;
				jockey.s.y = 14 * TILE_SIZE - 0.1;
				jockey.s.x = 3 * TILE_SIZE;
				jockey.right = 1;
				return;
			}
			if (menu.select == 2) {
				mario_p.mode = 3;
				menu.levels = 0;
				return;
			}
		}
	}
	if (mario_p.mode == 1 || mario_p.mode == 2) {
		if (!mario_p.alive) {
			mario_p.mode = 4;
		}
		if (mario_p.won) {
			if (sdk_inputs.start) {
				mario_p.won = 0;
				if (map == maps_map1) {
					map = maps_map2;
					mario_p.px = 3.5;
					mario_p.py = 7;
				} else if (map == maps_map2) {
					map = maps_map3;
					mario_p.px = 3.5;
					mario_p.py = 7;
				} else if (map == maps_map3) {
					map = maps_map4;
					mario_p.px = 3.5;
					mario_p.py = 7;
				} else if (map == maps_map4) {
					map = maps_map1c;
					mario_p.px = 1;
					mario_p.py = 7;
					jockey.s.y = 14 * TILE_SIZE - 0.1;
					jockey.s.x = 3 * TILE_SIZE;
					jockey.right = 1;
				} else if (map == maps_map1c) {
					map = maps_map2c;
					mario_p.px = 1;
					mario_p.py = 7;
				} else if (map == maps_map2c) {
					map = maps_map3c;
					mario_p.px = 0 * TILE_SIZE;
					mario_p.py = 12 * TILE_SIZE;
					mario_p.vx = 0;
					mario_p.vy = 0;
				} else if (map == maps_map3c) {
					map = maps_map4c;
					mario_p.px = 0 * TILE_SIZE;
					mario_p.py = 12 * TILE_SIZE;
					mario_p.vx = 0;
					mario_p.vy = 0;
				} else if (map == maps_map4c) {
					map = maps_mapwin;
					mario_p.px = 1;
					mario_p.py = 7;
				}
			}
		}
		if (map == maps_map1c) {
			enemy_jockey(dt);
		}
		if (map == maps_map1) {
			enemy_jockey(dt);
		}
		if (sdk_inputs.vol_up) {
			volume += 12.0 * dt;
		}

		if (sdk_inputs.vol_down) {
			volume -= 12.0 * dt;
		}

		if (sdk_inputs_delta.select > 0) {
			game_reset();
			return;
		}
		if (sdk_inputs_delta.vol_sw > 0) {
			if (volume <= SDK_GAIN_MIN) {
				volume = SDK_GAIN_STD;
			} else {
				volume = SDK_GAIN_MIN;
			}
		}

		volume = clamp(volume, SDK_GAIN_MIN, SDK_GAIN_MAX);

		if (sdk_inputs.vol_up || sdk_inputs.vol_down || sdk_inputs.vol_sw) {
			sdk_set_output_gain_db(volume);
		}

		// Horizontal movement
		if (sdk_inputs.x > 0 || sdk_inputs.joy_x < -500) { // Left
			mario_p.vx -= ACCELERATION * dt;
			if (mario_p.vx < -MAX_SPEED)
				mario_p.vx = -MAX_SPEED;
		} else if (sdk_inputs.b > 0 || sdk_inputs.joy_x > 500) { // Right
			mario_p.vx += ACCELERATION * dt;
			if (mario_p.vx > MAX_SPEED)
				mario_p.vx = MAX_SPEED;

		} else {
			if (mario_p.friction == 0) {
				mario_p.vx -= mario_p.vx * FRICTION * dt;
				if (fabs(mario_p.vx) < 0.1) {
					mario_p.vx = 0;
				}
			}
		}

		if (mario_p.vy != 0) {
			mario_p.vx = mario_p.vx * 0.98;
		}

		// Apply gravity
		mario_p.vy += GRAVITY * dt;

		mario_p.px += mario_p.vx * dt;
		mario_p.py += mario_p.vy * dt;

		// Collision detection with floor tiles
		int tile_x = mario_p.px / TILE_SIZE;
		int tile_y = mario_p.py / TILE_SIZE + 1.0f / TILE_SIZE;

		mario_p.friction = 0;

		if (mario_p.vy >= 0 && tile_y >= 0) {
			switch (map[tile_y][tile_x]) {
			case GRASS:
			case FLOOR_MID:
			case FLOOR_R:
			case FLOOR_L:
				mario_p.py = tile_y * TILE_SIZE - 1.0f / TILE_SIZE;
				mario_p.vy = 0;

				if (sdk_inputs.y) {
					mario_p.vy = JUMP_STRENGTH;
				}

				break;

			case FLOOR_JUMP_MID:
			case FLOOR_JUMP_L:
			case FLOOR_JUMP_R:
				mario_p.py = tile_y * TILE_SIZE - 1.0f / TILE_SIZE;
				mario_p.vy = 0;

				if (sdk_inputs.y) {
					mario_p.vy = 1.2 * JUMP_STRENGTH;
				}

				break;

			case FLOOR_WIN_MID:
			case FLOOR_WIN_L:
			case FLOOR_WIN_R:
				mario_p.py = tile_y * TILE_SIZE - 1.0f / TILE_SIZE;
				mario_p.vy = 0;
				mario_p.won = 1;
				break;

			case FLOOR_MID_COLD:
			case FLOOR_R_COLD:
			case FLOOR_CUBE_COLD:
			case FLOOR_L_COLD:
				mario_p.py = tile_y * TILE_SIZE - 1.0f / TILE_SIZE;
				mario_p.vy = 0;
				mario_p.friction = 1;
				if (sdk_inputs.y) {
					mario_p.vy = JUMP_STRENGTH;
				}

				break;

			case FLOOR_JUMP_MID_COLD:
			case FLOOR_JUMP_L_COLD:
			case FLOOR_JUMP_CUBE_COLD:
			case FLOOR_JUMP_R_COLD:
				mario_p.py = tile_y * TILE_SIZE - 1.0f / TILE_SIZE;
				mario_p.vy = 0;
				mario_p.friction = 1;
				if (sdk_inputs.y) {
					mario_p.vy = 1.2 * JUMP_STRENGTH;
				}

				break;

			case FLOOR_WIN_MID_COLD:
			case FLOOR_WIN_L_COLD:
			case FLOOR_WIN_CUBE_COLD:
			case FLOOR_WIN_R_COLD:
				mario_p.py = tile_y * TILE_SIZE - 1.0f / TILE_SIZE;
				mario_p.vy = 0;
				mario_p.friction = 1;
				mario_p.won = 1;
				break;

			case EMPTY:
			case SPAWNER_MARIO:
			case SPAWNER_SONIC:
			case CHEST:
				break;
			}
		}

		// Boundaries
		if (mario_p.px < 0)
			mario_p.px = 0;

		if (mario_p.px >= MAP_COLS * TILE_SIZE - TILE_SIZE)
			mario_p.px = MAP_COLS * TILE_SIZE - TILE_SIZE;

		if (mario_p.py >= MAP_ROWS * TILE_SIZE - TILE_SIZE + TILE_SIZE - 1) {
			mario_p.alive = false;
		}
	}
	if (mario_p.mode == 3) {
		if (sdk_inputs_delta.y > 0 || sdk_inputs.joy_y < -500) {
			menu.levels -= 1;
		}
		if (sdk_inputs_delta.a > 0 || sdk_inputs.joy_y > 500) {
			menu.levels += 1;
		}
		if (menu.levels >= 8) {
			menu.levels = 0;
		}
		if (menu.levels == -1) {
			menu.levels = 7;
		}

		if (sdk_inputs_delta.start > 0) {
			mario_p.px = 1 * TILE_SIZE;
			mario_p.py = 12 * TILE_SIZE;
			mario_p.vx = 0;
			mario_p.vy = 0;
			mario_p.score = 0;
			mario_p.alive = 1;
			mario_p.won = 0;
			mario_p.mode = 1;
			if (menu.levels == 0) {
				map = maps_map1;
				jockey.s.y = 14 * TILE_SIZE - 1;
				jockey.s.x = 3 * TILE_SIZE;
				jockey.right = 1;
			} else if (menu.levels == 1) {
				map = maps_map2;
			} else if (menu.levels == 2) {
				map = maps_map3;
			} else if (menu.levels == 3) {
				map = maps_map4;
			} else if (menu.levels == 4) {
				map = maps_map1c;
				jockey.s.y = 14 * TILE_SIZE - 1;
				jockey.s.x = 3 * TILE_SIZE;
				jockey.right = 1;
			} else if (menu.levels == 5) {
				map = maps_map2c;
			} else if (menu.levels == 6) {
				map = maps_map3c;
			} else if (menu.levels == 7) {
				map = maps_map4c;
			}
		}
	}
}

static void enemy_jockey(float dt)
{
	if (sdk_inputs_delta.a > 0) {
		jockey.vx = 0;
		jockey.right = !jockey.right;
	}

	jockey.vx += jockey.right ? dt * 50 : dt * -50;
	jockey.vx = clamp(jockey.vx, -MAX_JOCKEY_SPEED, MAX_JOCKEY_SPEED);

	float next_x = jockey.s.x + jockey.vx * dt;
	int next_tile_x = next_x / TILE_SIZE;
	int next_tile_y = jockey.s.y / TILE_SIZE + 1;

	if (next_x < 0.0f || next_x >= MAP_COLS * TILE_SIZE) {
		goto turn;
	}

	TileType next_tile = map[next_tile_y][next_tile_x];
	bool can_move;

	switch (next_tile) {
	case GRASS:
	case FLOOR_MID:
	case FLOOR_R:
	case FLOOR_L:
	case FLOOR_JUMP_MID:
	case FLOOR_JUMP_L:
	case FLOOR_JUMP_R:
	case FLOOR_WIN_MID:
	case FLOOR_WIN_L:
	case FLOOR_WIN_R:
	case FLOOR_MID_COLD:
	case FLOOR_R_COLD:
	case FLOOR_CUBE_COLD:
	case FLOOR_L_COLD:
	case FLOOR_JUMP_MID_COLD:
	case FLOOR_JUMP_L_COLD:
	case FLOOR_JUMP_CUBE_COLD:
	case FLOOR_JUMP_R_COLD:
	case FLOOR_WIN_MID_COLD:
	case FLOOR_WIN_L_COLD:
	case FLOOR_WIN_CUBE_COLD:
	case FLOOR_WIN_R_COLD:
		can_move = true;
		break;

	case EMPTY:
	case SPAWNER_MARIO:
	case SPAWNER_SONIC:
	case CHEST:
		can_move = false;
		break;
	}

	if (can_move) {
		jockey.s.x = next_x;
	} else {
turn:
		jockey.right = !jockey.right;
		jockey.vx = jockey.right ? MIN_JOCKEY_SPEED : -MIN_JOCKEY_SPEED;
	}

	jockey.s.tile = jockey.right;
}

// --- Game Paint ---
void game_paint(unsigned dt_usec)
{
	(void)dt_usec;

	tft_fill(0);

	tft_set_origin(mario_p.px - TFT_WIDTH / 2.0 + 3.5, 0);

	//sdk_draw_tile(0, 0, &ts_menu_png, 0);

	// Draw tile map
	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE, &ts_platforms_png,
				      map[y][x] - 1);
			if (!map[y][x]) {
				tft_draw_rect(x * TILE_SIZE, y * TILE_SIZE,
					      x * TILE_SIZE + TILE_SIZE - 1,
					      y * TILE_SIZE + TILE_SIZE - 1,
					      rgb_to_rgb565(0, 0, 0));
				continue;
			}
		}
	}
	//tft_draw_rect(mario_p.px, mario_p.py, mario_p.px + TILE_SIZE, mario_p.py - TILE_SIZE, RED);
	mario_p.s.x = mario_p.px;
	mario_p.s.y = mario_p.py;

	if (mario_p.vx > 0)
		mario_p.s.tile = 0;
	else if (mario_p.vx < 0)
		mario_p.s.tile = 1;
	if (!mario_p.alive)
		mario_p.s.tile = 4;

	sdk_draw_sprite(&mario_p.s);
	if (map == maps_map1 || map == maps_map1c) {
		sdk_draw_sprite(&jockey.s);
	}
	tft_set_origin(0, 0);
	char buf[16];
	snprintf(buf, sizeof buf, "%-.2f", mario_p.time);
	if (mario_p.fast) {
		tft_draw_string(0, 0, rgb_to_rgb565(255, 255, 0), buf);
	}
	if (mario_p.mode == 0) {
		sdk_draw_tile(0, 0, &ts_menu_png, menu.select);
	}
	if (mario_p.mode == 3) {
		sdk_draw_tile(0, 0, &ts_levels_png, menu.levels);
	}
	if (mario_p.mode == 4) {
		sdk_draw_tile(0, 0, &ts_death_png, 1);
	}
	//tft_draw_pixel(mario_p.px + 0.5, mario_p.py - 0.5, WHITE);
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = false,
		.fps_color = GRAY,
	};
	sdk_main(&config);
}
