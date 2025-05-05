#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <sdk.h>

#include <tiles.png.h>
#include <gametiles.png.h>
#include <janek.png.h>
#include <player.png.h>

#define TILE_SIZE 16
#define MAP_ROWS 7
#define MAP_COLS 10

#define SPEED_WORLD 40

#define SPEED_GAME 60
#define JUMP_STRENGTH 50
#define GRAVITY 60

#define multiply332 \
	(x, f) rgb_to_rgb332(rgb332_red((x)) * f, rgb332_green((x)) * f, rgb332_blue((x)) * f)

#define RED rgb_to_rgb565(255, 0, 0)
#define RED_POWER rgb_to_rgb565(255, 63, 63)
#define YELLOW rgb_to_rgb565(255, 255, 0)
#define GREEN rgb_to_rgb565(0, 255, 0)
#define GREEN_POWER rgb_to_rgb565(63, 255, 63)
#define BLUE rgb_to_rgb565(0, 0, 255)
#define GRAY rgb_to_rgb565(127, 127, 127)
#define WHITE rgb_to_rgb565(255, 255, 255)

static float volume = 0;
typedef enum {
	EMPTY = 0,
	GRASS = 1,
} TileType_world;

TileType_world world_map[MAP_ROWS][MAP_COLS] = {
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },

};

typedef enum {
	NOTHING = 0,
	ROCKS = 1,
} TileType_game;

TileType_game game_map[MAP_ROWS][MAP_COLS] = {
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, { 4, 2, 2, 2, 2, 2, 2, 2, 2, 5 },
	{ 4, 0, 0, 0, 0, 0, 0, 0, 0, 5 }, { 4, 0, 0, 0, 0, 0, 0, 0, 0, 5 },
	{ 4, 0, 0, 0, 0, 0, 0, 0, 0, 5 }, { 4, 0, 0, 0, 0, 0, 0, 0, 0, 5 },
	{ 4, 3, 3, 3, 3, 3, 3, 3, 3, 5 },

};

//IMPORTENT//
typedef struct {
	float status;
} Game;

static Game game;

//--Janek-Sigma--//
typedef struct {
	float fx, fy;
	float speed;
	sdk_sprite_t s;
} Janek;

static Janek janek;

//--Janek-playing-a-game--//
typedef struct {
	float fx, fy;
	float speed;
	sdk_sprite_t s;
} Player;

static Player player;

static void game_audio_handaling(float dt);

static void game_start_world();
static void game_start_player();

static void janek_handaling_world(float dt);
static void janek_sprites_world();

static void player_handaling_game(float dt);
static void player_handaling_paint();

static void game_tile_world();
static void game_tile_game();

static void map_handaling();
//--starting-my-sigma-game//
void game_start(void)
{
	sdk_set_output_gain_db(volume);
	game_start_world();
}
//--starting-world-with-janek--//
static void game_start_world()
{
	game.status = 1;

	janek.speed = SPEED_WORLD;

	janek.fx = 0;
	janek.fy = 0;

	janek.s.ts = &ts_janek_png;
	janek.s.x = 5 * TILE_SIZE;
	janek.s.y = 3.5 * TILE_SIZE;
}
//--starting-any-player-game--//
static void game_start_player()
{
	game.status = 2;

	player.speed = SPEED_GAME;

	player.s.ts = &ts_player_png;
	player.s.x = 5 * TILE_SIZE;
	player.s.y = 3.5 * TILE_SIZE;
}
//--imagine-restarting--//
void game_reset(void)
{
	game_start();
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;
	game_audio_handaling(dt);
	if (game.status == 1) {
		janek_handaling_world(dt);
	} else if (game.status == 2) {
		game_start_player();
		player_handaling_game(dt);
	}
}

//--Game-tile-stuff-I-dont-understand--//
static void game_tile_world()
{
	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE, &ts_tiles_png,
				      world_map[y][x] - 1);
		}
	}
}

//--Game-tile-stuff-I-dont-understand--//
static void game_tile_game()
{
	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE, &ts_gametiles_png,
				      game_map[y][x] - 1);
		}
	}
}

//--Please-this-needs-to-be-a-norm--//
static void game_audio_handaling(float dt)
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
//--HE-CAN-MOOOOOOOOOOOOVE--//
static void janek_handaling_world(float dt)
{
	if (sdk_inputs.joy_x > 500 || sdk_inputs.joy_x < -500) {
		janek.fx = janek.speed * sdk_inputs.joy_x / 2048;
	}
	if (sdk_inputs.joy_y > 500 || sdk_inputs.joy_y < -500) {
		janek.fy = janek.speed * sdk_inputs.joy_y / 2048;
	}
	janek.s.y += janek.fy * dt;
	janek.s.x += janek.fx * dt;

	if (sdk_inputs_delta.start == 1) {
		game.status = 2;
	}
	janek_sprites_world();
}

//--Player-game-needs-to-work--//
static void player_handaling_game(float dt)
{
	if (sdk_inputs.joy_x > 500) {
		player.fx += player.speed * dt;
	}

	if (sdk_inputs.joy_x < -500) {
		player.fx -= player.speed * dt;
	}

	if (sdk_inputs_delta.a == 1) {
		player.fy = -JUMP_STRENGTH;
	}

	player.fy += dt * GRAVITY;

	player.s.x += player.fx;
	player.s.y += player.fy;

	player_handaling_paint();
}

//--What-game-tile-?--//
static void map_handaling()
{
	if (game.status == 1) {
		game_tile_world();
	}
	if (game.status == 2) {
		game_tile_game();
	}
}

//--game-panting--//
static void player_handaling_paint()
{
	sdk_draw_sprite(&player.s);
	tft_draw_string(0, 0, WHITE, "%-.2f", player.s.y);
	tft_draw_string(0, 10, WHITE, "%-.2f", player.s.x);
}

//--Handaling-my-world-paint--//
static void janek_sprites_world()
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
	tft_draw_string(0, 0, WHITE, "%-.2f", janek.s.y);
	tft_draw_string(0, 10, WHITE, "%-.2f", janek.s.x);
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;
	tft_fill(0);
	map_handaling();
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
