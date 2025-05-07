#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <sdk.h>
#include <stdio.h>

#include <tiles.png.h>
#include <gametiles.png.h>
#include <janek.png.h>
#include <player.png.h>
#include <playerrun.png.h>

#define TILE_SIZE 16
#define MAP_ROWS 7
#define MAP_COLS 10

#define SPEED_WORLD 40

#define SPEED_GAME 8
#define MAX_SPEED_GAME 30
#define JUMP_STRENGTH 50
#define GRAVITY 60

#define BLUE rgb_to_rgb565(0, 0, 255)
#define WHITE rgb_to_rgb565(255, 255, 255)

static float volume = 0;

typedef enum {
	EMPTY = 0,
	GRASS = 1,
	PLATFORM_UP_1,
	PLATFORM_DOWN_1,
	PLATFORM_UP_2,
	PLATFORM_DOWN_2,
	PLATFORM_UP_3,
	PLATFORM_DOWN_3,
	DIRT = 8,
} TileType_world;

TileType_world world_map[MAP_ROWS][MAP_COLS] = {
	{ 1, 1, 1, 1, 2, 4, 6, 1, 1, 1 }, //
	{ 1, 1, 1, 1, 3, 5, 7, 1, 1, 1 }, //
	{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 }, //
	{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 }, //
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, //
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, //
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, //

};

typedef enum {
	NOTHING = 0,
	ROCKS = 1,
	PIPE_UP,
	PIPE_DOWN,
	PIPE_LEFT,
	PIPE_RIGHT,
} TileType_game;

TileType_game game_map[MAP_ROWS][MAP_COLS] = {
	{ 1, 2, 2, 2, 2, 2, 2, 2, 2, 1 }, //
	{ 4, 0, 0, 0, 0, 0, 0, 0, 0, 5 }, //
	{ 4, 0, 0, 0, 0, 0, 0, 0, 0, 5 }, //
	{ 4, 0, 0, 0, 0, 0, 0, 0, 0, 5 }, //
	{ 4, 0, 0, 0, 0, 0, 0, 0, 0, 5 }, //
	{ 4, 0, 0, 0, 0, 0, 0, 0, 0, 5 }, //
	{ 3, 3, 3, 3, 3, 3, 3, 3, 3, 1 }, //
};

enum game_mode {
	MODE_WORLD = 0,
	MODE_GAME = 1,
};

typedef struct {
	enum game_mode mode;
} Game;

static Game game;

typedef struct {
	float fx, fy;
	float speed;
	sdk_sprite_t s;
} Janek;

static Janek janek;

typedef struct {
	float fx, fy;
	float speed;
	sdk_sprite_t s;
} Player;

static Player player;

static void game_handle_audio(float dt);

static void janek_handle_world(float dt);
static void paint_janek();

static void player_handle_game(float dt);
static void paint_player();

static void paint_world();
static void paint_game();

void game_start(void)
{
	sdk_set_output_gain_db(volume);
}

static void game_start_world()
{
	janek.speed = SPEED_WORLD;

	janek.fx = 0;
	janek.fy = 0;

	janek.s.ts = &ts_janek_png;
	janek.s.x = 5 * TILE_SIZE;
	janek.s.y = 3.5 * TILE_SIZE;
}

static void game_start_player()
{
	player.speed = SPEED_GAME;

	player.s.ts = &ts_playerrun_png;
	player.s.x = 5 * TILE_SIZE;
	player.s.y = 3.5 * TILE_SIZE;
	player.s.ox = 16;
	player.s.oy = 31;
}

void game_reset(void)
{
	game_start_world();
	game_start_player();

	game.mode = MODE_WORLD;
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;
	game_handle_audio(dt);

	switch (game.mode) {
	case MODE_WORLD:
		janek_handle_world(dt);
		break;

	case MODE_GAME:
		player_handle_game(dt);
		break;
	}
}

static void paint_world()
{
	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			if (world_map[y][x])
				sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE, &ts_tiles_png,
					      world_map[y][x] - 1);
		}
	}
}

static void paint_game()
{
	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			if (game_map[y][x])
				sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE, &ts_gametiles_png,
					      game_map[y][x] - 1);
		}
	}
}

static void game_handle_audio(float dt)
{
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
		if (volume < SDK_GAIN_MIN) {
			volume = 0;
		} else {
			volume = SDK_GAIN_MIN - 1;
		}
	}

	volume = clamp(volume, SDK_GAIN_MIN - 1.0, 6);

	if (sdk_inputs.vol_up || sdk_inputs.vol_down || sdk_inputs.vol_sw) {
		sdk_set_output_gain_db(volume);
	}
}

static void janek_handle_world(float dt)
{
	if (sdk_inputs.joy_x > 500 || sdk_inputs.joy_x < -500)
		janek.fx = janek.speed * sdk_inputs.joy_x / 2048;

	if (sdk_inputs.joy_y > 500 || sdk_inputs.joy_y < -500)
		janek.fy = janek.speed * sdk_inputs.joy_y / 2048;

	janek.s.y += janek.fy * dt;
	janek.s.x += janek.fx * dt;

	if (sdk_inputs_delta.start > 0) {
		game.mode = MODE_GAME;
	}
}
static void player_handle_game(float dt)
{
	if (sdk_inputs.joy_x >= 500) {
		player.fx += player.speed;
	}

	if (sdk_inputs.joy_x <= -500) {
		player.fx -= player.speed;
	}
	if (player.fx >= MAX_SPEED_GAME) {
		player.fx = MAX_SPEED_GAME;
	}

	if (player.fx <= -MAX_SPEED_GAME) {
		player.fx = -MAX_SPEED_GAME;
	}
	player.fy += GRAVITY * dt;

	player.s.y += player.fy * dt;
	player.s.x += player.fx * dt;

	int tile_x = player.s.x / TILE_SIZE;
	int tile_y = player.s.y / TILE_SIZE - 1.0f / TILE_SIZE;

	switch (game_map[tile_y][tile_x]) {
	case PIPE_DOWN:
		player.s.y = tile_y * TILE_SIZE + 1.0f / TILE_SIZE;
		player.fy = 0;

		if (sdk_inputs.y) {
			player.fy = JUMP_STRENGTH;
		}

		break;
	case PIPE_LEFT:
	case PIPE_UP:
	case PIPE_RIGHT:
	case NOTHING:
	case ROCKS:
		break;
	}
	printf("tile_x %d\n", tile_x);
	printf("tile_y %d\n", tile_y);
}
static void paint_player()
{
	sdk_draw_sprite(&player.s);
	if (player.s.tile == 0 && player.fx < 0) {
		player.s.tile = 2;
	} else if (player.s.tile == 2 && player.fx < 0) {
		player.s.tile = 2;
	} else if (player.s.tile == 1 && player.fx > 0) {
		player.s.tile = 3;
	} else if (player.s.tile == 3 && player.fx > 0) {
		player.s.tile = 1;
	} else if (player.fx < 0) {
		player.s.tile = 1;
	} else if (player.fx > 0) {
		player.s.tile = 0;
	}

	tft_draw_string(0, 0, WHITE, "%-.2f", player.s.x);
	tft_draw_string(0, 10, WHITE, "%-.2f", player.fy);
	//tft_draw_pixel(player.s.x, player.s.y, white);
}
static void paint_janek()
{
	if (janek.fx >= 1) {
		janek.s.tile = 2;
	} else if (janek.fx <= -1) {
		janek.s.tile = 1;
	} else if (janek.fy >= 1) {
		janek.s.tile = 0;
	} else if (janek.fy <= -1) {
		janek.s.tile = 3;
	}

	janek.fy = 0;
	janek.fx = 0;

	sdk_draw_sprite(&janek.s);

	tft_draw_string(0, 0, WHITE, "%-.2f", janek.s.x);
	tft_draw_string(0, 10, WHITE, "%-.2f", janek.s.y);
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;

	tft_fill(0);

	switch (game.mode) {
	case MODE_WORLD:
		paint_world();
		paint_janek();
		break;

	case MODE_GAME:
		paint_game();
		paint_player();
		break;
	}
}
int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = false,
		.fps_color = BLUE,
	};
	sdk_main(&config);
}
