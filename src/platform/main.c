#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>

// Screen and tile dimensions
#define SCREEN_WIDTH 140
#define SCREEN_HEIGHT 120
#define TILE_SIZE 8
#define MAP_ROWS 15
#define MAP_COLS 20

// Color definitions
#define RED 255
#define GREEN 244
#define GRAY 8

// Define tile types
typedef enum { EMPTY = 0, FLOOR = 1 } TileType;

// Tile map
TileType map[MAP_ROWS][MAP_COLS] = { 0 };

// Mario's state
typedef struct {
	int x, y;
	int alive;
	int won;
} Mario;
static Mario mario_p;

void game_start(void)
{
	mario_p.x = 2;
	mario_p.y = 12;
	mario_p.alive = 1;
	mario_p.won = 0;
}

void game_reset(void)
{
	game_start();
}

// Audio settings
static uint16_t tones[256];
static const char tune[] = " ACGgbGA  ACEGgFD  ACGgBEC FGAGFED ";
static bool play_music = true;

void tone_init(void)
{
	tones['c'] = 131;
	tones['d'] = 147;
	tones['e'] = 165;
	tones['f'] = 175;
	tones['g'] = 196;
	tones['a'] = 220;
	tones['h'] = 247;
	tones['C'] = 261;
	tones['D'] = 293;
	tones['E'] = 329;
	tones['F'] = 349;
	tones['G'] = 392;
	tones['A'] = 440;
	tones['H'] = 494;
}

void game_audio(int nsamples)
{
	static int elapsed = 0;
	static int tone_pos = 0;
	if (!play_music) {
		tone_pos = 0;
		elapsed = 0;
		for (int s = 0; s < nsamples; s++)
			sdk_write_sample(0);
		return;
	}
	for (int s = 0; s < nsamples; s++) {
		if (elapsed > SDK_AUDIO_RATE / 4) {
			tone_pos++;
			elapsed = 0;
		}
		if (!tune[tone_pos]) {
			tone_pos = 0;
		}
		int freq = tones[(unsigned)tune[tone_pos]];
		if (freq) {
			int period = SDK_AUDIO_RATE / freq;
			int half_period = 2 * elapsed / period;
			int modulo = half_period & 1;
			sdk_write_sample(4000 * (modulo ? 1 : -1));
		} else {
			sdk_write_sample(0);
		}
		elapsed++;
	}
}

// Game Input
void game_input(unsigned __unused dt_usec)
{
	if (!mario_p.alive || mario_p.won) {
		if (sdk_inputs.start)
			game_reset();
		return;
	}
	if (sdk_inputs_delta.x > 0)
		mario_p.x -= 1;
	else if (sdk_inputs_delta.a > 0)
		mario_p.x += 1;
	else if (sdk_inputs_delta.y > 0)
		mario_p.y -= 1;
	else if (sdk_inputs_delta.b > 0)
		mario_p.y += 1;

	if (mario_p.x < 0)
		mario_p.x = 0;
	if (mario_p.x >= MAP_COLS)
		mario_p.x = MAP_COLS - 1;
	if (mario_p.y < 0)
		mario_p.y = 0;
	if (mario_p.y >= MAP_ROWS)
		mario_p.y = MAP_ROWS - 1;

	if (mario_p.x == MAP_COLS - 1)
		mario_p.won = 1;
	if (map[mario_p.y][mario_p.x] == FLOOR)
		mario_p.alive = 0;
}

// Draw the game
void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);
	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			if (map[y][x] == FLOOR)
				tft_draw_rect(x * TILE_SIZE, y * TILE_SIZE, (x + 1) * TILE_SIZE,
					      (y + 1) * TILE_SIZE, GREEN);
		}
	}
	tft_draw_rect(mario_p.x * TILE_SIZE, mario_p.y * TILE_SIZE, (mario_p.x + 1) * TILE_SIZE,
		      (mario_p.y + 1) * TILE_SIZE, RED);
	if (!mario_p.alive)
		tft_draw_rect(30, 50, 110, 70, RED);
	if (mario_p.won)
		tft_draw_rect(30, 50, 110, 70, GREEN);
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = true,
		.fps_color = GRAY,
	};
	game_start();
	tone_init();
	sdk_main(&config);
}
