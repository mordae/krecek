#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <sdk.h>
#include <math.h>
#include <stdio.h>
#include "common.h"

#include <petr.png.h>
#include <sonic.png.h>
#include <kurecidzokej.png.h>
#include <platforms.png.h>
#include <menu.png.h>
#include <menu-cursor.png.h>
#include <levels1to4.png.h>
#include <levels5to8.png.h>
#include <death.png.h>
#include <heart.png.h>
#include <cover.png.h>

sdk_game_info("platform", &image_cover_png);

// Physics constants
#define GRAVITY 90	  // Gravity for falling
#define JUMP_STRENGTH -80 // Jump strength
#define MAX_SPEED 100	  // Max speed for movement
#define MAX_JOCKEY_SPEED 40
#define MIN_JOCKEY_SPEED 5
#define ACCELERATION 80 // Acceleration for movement
#define FRICTION 8	// Friction to stop sliding quickly

#define MARIO_HALF_WIDTH 3.0f
#define MARIO_HEIGHT 8.0f
#define CHEST_SCORE 50
#define JOCKEY_SCORE 100
#define START_LIVES 3

// Color definitions (using palette indices)
#define RED rgb_to_rgb565(194, 20, 20)
#define YELLOW rgb_to_rgb565(255, 255, 0)
#define GREEN rgb_to_rgb565(0, 255, 0)
#define BLUE rgb_to_rgb565(23, 62, 224)
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
	int lives;
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

static bool sonic_mode;
static float volume = SDK_GAIN_STD;

// Chests already picked up on the current level.
static bool chest_taken[MAP_ROWS][MAP_COLS];

// Best time of a completed fast run, 0 when there is none yet.
static float best_time;

static void enemy_jockey(float dt);
static void game_menu(float dt);
static void world_start();
static void mario_movement(float dt);
static void mario_win();

static bool jockey_active(void)
{
	return map == maps_map1 || map == maps_map1c;
}

static bool tile_is_solid(TileType tile)
{
	switch (tile) {
	case EMPTY:
	case SPAWNER_MARIO:
	case SPAWNER_SONIC:
	case CHEST:
		return false;

	default:
		return true;
	}
}

static bool map_solid_at(float x, float y)
{
	int tx = x / TILE_SIZE;
	int ty = y / TILE_SIZE;

	if (x < 0 || tx >= MAP_COLS || y < 0 || ty >= MAP_ROWS)
		return false;

	return tile_is_solid(map[ty][tx]);
}

// --- Audio: Updated tune ---
static const char music1[] = "/i:square /bpm:60 /pl "
			     "{ "
			     "ACGgbGA _ ACEGgFD _ ACGgbEC _ FGAGFED _ "
			     "CEGAEC  _ GBgfeD  _ ACGEFGA _ DEFGEDC _ "
			     "ACGgbGA _ ACEGgFD _ ACGgbEC _ FGAGFED _ "
			     "EGCEGA  _ DFABAG  _ CEGBAF  _ DFACBG  _ "
			     "GABCDEF _ EGABCDE _ BAGFEDC _ ACEGFED _ "
			     "CEGAGEC _ GBgfeD  _ ACGEFGA _ DEFGEDC _ "
			     "}";

static const char music2[] = "/i:noise /bpm:60 /pr "
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
	sonic_mode = false;
}

void game_reset(void)
{
	map = maps_map0;
	mario_p.mode = 0;
	mario_p.score = 0;
	mario_p.lives = START_LIVES;
	menu.select = 0;
	sonic_mode = false;

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

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;
	if (sdk_inputs.vol_up) {
		volume += 12.0 * dt;
	}

	if (sdk_inputs.vol_down) {
		volume -= 12.0 * dt;
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

	if (map != maps_mapwin) {
		mario_p.time += dt;
	}
	if (mario_p.mode == 4) {
		if (sdk_inputs_delta.start > 0) {
			if (mario_p.lives > 0) {
				// Retry the same level, keeping the score.
				world_start();
			} else {
				game_reset();
			}
			return;
		}
	} else if (mario_p.mode == 0) {
		game_menu(dt);
		return;
	}
	if (mario_p.mode == 1 || mario_p.mode == 2) {
		mario_movement(dt);
	}
	if (mario_p.mode == 3) {
		game_menu(dt);
		return;
	}
}
static void mario_win()
{
	mario_p.won = 0;
	if (map == maps_map1) {
		map = maps_map2;
		world_start();
	} else if (map == maps_map2) {
		map = maps_map3;
		world_start();
	} else if (map == maps_map3) {
		map = maps_map4;
		world_start();
	} else if (map == maps_map4) {
		map = maps_map1c;
		world_start();
	} else if (map == maps_map1c) {
		map = maps_map2c;
		world_start();
	} else if (map == maps_map2c) {
		map = maps_map3c;
		world_start();
	} else if (map == maps_map3c) {
		map = maps_map4c;
		world_start();
	} else if (map == maps_map4c) {
		map = maps_mapwin;
		mario_p.px = 1;
		mario_p.py = 7;

		if (mario_p.fast && (best_time == 0 || mario_p.time < best_time))
			best_time = mario_p.time;
	}
}

static void mario_movement(float dt)
{
	if (!mario_p.alive) {
		mario_p.mode = 4;

		if (mario_p.lives > 0)
			mario_p.lives--;

		return;
	}
	if (mario_p.won) {
		if (sdk_inputs.start) {
			mario_win();
		}
	}

	if (jockey_active() && jockey.alive) {
		enemy_jockey(dt);
	}

	if (sdk_inputs_delta.select > 0) {
		game_reset();
		return;
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

	mario_p.vy += GRAVITY * dt;

	// Horizontal movement, reverted when it would enter a solid tile.
	float prev_px = mario_p.px;
	mario_p.px += mario_p.vx * dt;

	if (mario_p.vx != 0) {
		float edge = mario_p.px + (mario_p.vx > 0 ? MARIO_HALF_WIDTH : -MARIO_HALF_WIDTH);

		if (map_solid_at(edge, mario_p.py - 1) ||
		    map_solid_at(edge, mario_p.py - MARIO_HEIGHT + 2)) {
			mario_p.px = prev_px;
			mario_p.vx = 0;
		}
	}

	float prev_py = mario_p.py;
	mario_p.py += mario_p.vy * dt;

	// Head bump.
	if (mario_p.vy < 0 && map_solid_at(mario_p.px, mario_p.py - MARIO_HEIGHT)) {
		mario_p.py = prev_py;
		mario_p.vy = 0;
	}

	// Fell out of the world; check before the map lookup below.
	if (mario_p.py >= MAP_ROWS * TILE_SIZE - 1) {
		mario_p.alive = false;
		return;
	}

	int tile_x = clamp((int)(mario_p.px / TILE_SIZE), 0, MAP_COLS - 1);
	int tile_y = clamp((int)(mario_p.py / TILE_SIZE + 1.0f / TILE_SIZE), 0, MAP_ROWS - 1);

	// Collect a chest we are standing in.
	int chest_y = clamp((int)((mario_p.py - MARIO_HEIGHT / 2) / TILE_SIZE), 0, MAP_ROWS - 1);

	if (map[chest_y][tile_x] == CHEST && !chest_taken[chest_y][tile_x]) {
		chest_taken[chest_y][tile_x] = true;
		mario_p.score += CHEST_SCORE;
		sdk_melody_play("/i:sine e g");
	}

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

	// Jockey contact: stomp it from above, otherwise it is fatal.
	if (jockey_active() && jockey.alive) {
		mario_p.s.x = mario_p.px;
		mario_p.s.y = mario_p.py;

		if (sdk_sprites_collide_bbox(&mario_p.s, &jockey.s)) {
			if (mario_p.vy > 0 && mario_p.py <= jockey.s.y) {
				jockey.alive = false;
				mario_p.score += JOCKEY_SCORE;
				mario_p.vy = JUMP_STRENGTH / 2;
				sdk_melody_play("/i:square (f) g e");
			} else {
				mario_p.alive = false;
			}
		}
	}
}
static void enemy_jockey(float dt)
{
	jockey.vx += jockey.right ? dt * 50 : dt * -50;
	jockey.vx = clamp(jockey.vx, -MAX_JOCKEY_SPEED, MAX_JOCKEY_SPEED);

	float next_x = jockey.s.x + jockey.vx * dt;
	int next_tile_x = next_x / TILE_SIZE;
	int next_tile_y = jockey.s.y / TILE_SIZE + 1;

	if (next_x < 0.0f || next_x >= MAP_COLS * TILE_SIZE) {
		goto turn;
	}

	TileType next_tile = map[next_tile_y][next_tile_x];
	bool can_move = false;

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
static void world_start()
{
	mario_p.px = 0 * TILE_SIZE;
	mario_p.py = 12 * TILE_SIZE;
	mario_p.alive = 1;
	mario_p.won = 0;
	mario_p.mode = 1;
	jockey.s.y = 14 * TILE_SIZE - 1;
	jockey.s.x = 3 * TILE_SIZE;
	jockey.right = 1;
	jockey.alive = 1;

	// Spawn where the map says to, if it says anything at all.
	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			chest_taken[y][x] = false;

			if (map[y][x] == SPAWNER_MARIO || map[y][x] == SPAWNER_SONIC) {
				mario_p.px = x * TILE_SIZE + TILE_SIZE / 2;
				mario_p.py = y * TILE_SIZE + TILE_SIZE - 1;
				sonic_mode = map[y][x] == SPAWNER_SONIC;
			}
		}
	}

	mario_p.s.ts = sonic_mode ? &ts_sonic_png : &ts_petr_png;
}

static void game_menu(float dt)
{
	(void)dt;

	int rx = sdk_inputs.tx * TFT_RIGHT;
	int ry = sdk_inputs.ty * TFT_RIGHT;
	if (mario_p.mode == 0) {
		mario_p.fast = 0;
		map = maps_map0;

		menu.select += sdk_inputs_delta.vertical;
		menu.select -= (sdk_inputs_delta.y > 0);
		menu.select += (sdk_inputs_delta.a > 0);

		if (sdk_inputs.tp >= 0.5f) {
			if (rx >= 6 && rx <= 75 && ry >= 28 && ry <= 47)
				goto start_game;
			if (rx >= 6 && rx <= 75 && ry >= 50 && ry <= 69)
				goto start_fast_game;
			if (rx >= 6 && rx <= 75 && ry >= 72 && ry <= 91)
				goto level_menu;
		}

		if (menu.select > 2) {
			menu.select = 0;
		} else if (menu.select < 0) {
			menu.select = 2;
		}

		if (sdk_inputs_delta.start > 0) {
			if (menu.select == 0) {
start_game:
				mario_p.px = 1 * TILE_SIZE;
				mario_p.py = 12 * TILE_SIZE;
				map = maps_map1;
				world_start();
				return;
			} else if (menu.select == 1) {
start_fast_game:
				mario_p.time = 0;
				mario_p.fast = 1;
				map = maps_map1;
				world_start();
				return;
			} else if (menu.select == 2) {
level_menu:
				mario_p.mode = 3;
				menu.levels = 0;
				return;
			}
		}
	} else if (mario_p.mode == 3) {
		menu.levels += sdk_inputs_delta.vertical;
		menu.levels -= (sdk_inputs_delta.y > 0);
		menu.levels += (sdk_inputs_delta.a > 0);

		if (menu.levels > 3 && menu.levels < 8 && sdk_inputs.tp >= 0.5f) {
			if (rx > 10 && rx < 69 && ry > 21 && ry < 89) {
				sonic_mode = true;
				map = maps_map1;
				world_start();
				return;
			}
		}
		if (menu.levels > 7) {
			menu.levels = 0;
		} else if (menu.levels < 0) {
			menu.levels = 7;
		}
		if (sdk_inputs_delta.start > 0) {
			if (menu.levels == 0) {
				map = maps_map1;
			} else if (menu.levels == 1) {
				map = maps_map2;
			} else if (menu.levels == 2) {
				map = maps_map3;
			} else if (menu.levels == 3) {
				map = maps_map4;
			} else if (menu.levels == 4) {
				map = maps_map1c;
			} else if (menu.levels == 5) {
				map = maps_map2c;
			} else if (menu.levels == 6) {
				map = maps_map3c;
			} else if (menu.levels == 7) {
				map = maps_map4c;
			}

			world_start();
		}
	}
}

// --- Game Paint ---
void game_paint(unsigned dt_usec)
{
	(void)dt_usec;

	if (mario_p.mode == 0) {
		sdk_draw_image(0, 0, &image_menu_png);
		sdk_draw_tile(82, 32 + 22 * menu.select, &ts_menu_cursor_png, 0);
	} else if (mario_p.mode == 3) {
		color_t bg = menu.levels >> 2 ? BLUE : RED;

		for (int i = 0; i < 4; i++) {
			tft_draw_rect(79, 6 + 28 * i, 79 + 70, 6 + 25 + 28 * i,
				      (menu.levels & 3) == i ? bg : WHITE);
		}

		if (menu.levels >> 2) {
			sdk_draw_image(0, 0, &image_levels5to8_png);
		} else {
			sdk_draw_image(0, 0, &image_levels1to4_png);
		}

	} else if (mario_p.mode == 4) {
		sdk_draw_tile(0, 0, &ts_death_png, 1);

		if (mario_p.lives > 0) {
			tft_draw_string(TFT_WIDTH / 2 - 30, TFT_HEIGHT - 20, WHITE, "LIVES %d",
					mario_p.lives);
		} else {
			tft_draw_string(TFT_WIDTH / 2 - 36, TFT_HEIGHT - 20, RED, "GAME OVER");
		}

		tft_draw_string(TFT_WIDTH / 2 - 30, TFT_HEIGHT - 10, WHITE, "SCORE %d",
				mario_p.score);
	} else {
		tft_fill(0);
		tft_set_origin(mario_p.px - TFT_WIDTH / 2.0 + 3.5, 0);

		// Draw tile map
		for (int y = 0; y < MAP_ROWS; y++) {
			for (int x = 0; x < MAP_COLS; x++) {
				TileType tile = map[y][x];

				// Spawners are markers and taken chests are gone.
				if (tile == SPAWNER_MARIO || tile == SPAWNER_SONIC ||
				    (tile == CHEST && chest_taken[y][x]))
					tile = EMPTY;

				if (!tile) {
					tft_draw_rect(x * TILE_SIZE, y * TILE_SIZE,
						      x * TILE_SIZE + TILE_SIZE - 1,
						      y * TILE_SIZE + TILE_SIZE - 1,
						      rgb_to_rgb565(0, 0, 0));
					continue;
				}

				sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE, &ts_platforms_png,
					      tile - 1);
			}
		}

		mario_p.s.x = mario_p.px;
		mario_p.s.y = mario_p.py;
		if (sonic_mode) {
		} else {
			if (mario_p.vx > 0)
				mario_p.s.tile = 0;
			else if (mario_p.vx < 0)
				mario_p.s.tile = 1;
			if (!mario_p.alive)
				mario_p.s.tile = 4;
		}
		mario_p.s.ts = sonic_mode ? &ts_sonic_png : &ts_petr_png;
		sdk_draw_sprite(&mario_p.s);

		if (jockey_active() && jockey.alive) {
			sdk_draw_sprite(&jockey.s);
		}

		tft_set_origin(0, 0);

		if (mario_p.fast) {
			tft_draw_string(0, 0, rgb_to_rgb565(255, 255, 0), "%-.2f", mario_p.time);
		}

		tft_draw_string(0, mario_p.fast ? 10 : 0, WHITE, "%d", mario_p.score);

		// Lives left, as one heart and a count.
		sdk_draw_image(TFT_WIDTH - image_heart_png.width, 0, &image_heart_png);
		tft_draw_string(TFT_WIDTH - image_heart_png.width - 12,
				image_heart_png.height / 2 - 4, WHITE, "x%d", mario_p.lives);

		if (map == maps_mapwin && best_time > 0) {
			tft_draw_string(TFT_WIDTH / 2 - 30, TFT_HEIGHT - 12, YELLOW, "BEST %-.2f",
					best_time);
		}

		//tft_draw_pixel(mario_p.px + 0.5, mario_p.py - 0.5, WHITE);
	}
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
