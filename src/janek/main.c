#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <sdk.h>

#include <tiles.png.h>
#include <janek.png.h>

#define TILE_SIZE 16
#define MAP_ROWS 7
#define MAP_COLS 10
#define SPEED 40
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
} TileType;

TileType map[MAP_ROWS][MAP_COLS] = {
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },

};

//--Janek-Sigma--//
typedef struct {
	float fx, fy;
	float speed;
	sdk_sprite_t s;
} Janek;

static Janek janek;

static void game_tile();
static void game_audio_handaling(float dt);
static void janek_movement(float dt);
static void janek_sprites();

//--starting-my-sigma-game//
void game_start(void)
{
	sdk_set_output_gain_db(volume);

	janek.speed = SPEED;

	janek.fx = 0;
	janek.fy = 0;

	janek.s.ts = &ts_janek_png;
	janek.s.x = 0;
	janek.s.y = 0;
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
	janek_movement(dt);
}

//--Game-tile-stuff-I-dont-understand--//
static void game_tile()
{
	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE, &ts_tiles_png, map[y][x] - 1);
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
static void janek_movement(float dt)
{
	janek.fx = janek.speed * sdk_inputs.joy_x / 2048;
	janek.fy = janek.speed * sdk_inputs.joy_y / 2048;

	janek.s.y += janek.fy * dt;
	janek.s.x += janek.fx * dt;
}
//--He-looks-diffrent--//
static void janek_sprites()
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
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;
	game_tile();
	tft_fill(0);
	janek_sprites();

	sdk_draw_sprite(&janek.s);
	tft_draw_string(0, 0, WHITE, "%-.2f", janek.s.y);
	tft_draw_string(0, 10, WHITE, "%-.2f", janek.s.x);
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
