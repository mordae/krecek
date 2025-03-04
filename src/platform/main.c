#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <sdk.h>
#include <string.h>
#include <stdio.h>
#include <math.h> // Include math.h for fabs

// Screen and tile dimensions
#define SCREEN_WIDTH 140
#define SCREEN_HEIGHT 120
#define TILE_SIZE 8
#define MAP_ROWS 15
#define MAP_COLS 20

//#include <petr.png.h>
//#include <platforms.png.h>

// Physics constants
#define GRAVITY 1.5	  // Gravity for falling
#define JUMP_STRENGTH -10 // Jump strength
#define MAX_SPEED 3.0	  // Max speed for movement
#define ACCELERATION 0.3  // Acceleration for movement
#define FRICTION 0.7	  // Friction to stop sliding quickly

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
// Define tile types using typedef enum
typedef enum {
	EMPTY = 0,
	FLOOR_MID = 1,
	FLOOR_L,
	FLOOR_R,
	FLOOR_WIN_MID,
	FLOOR_WIN_L,
	FLOOR_WIN_P,
	FLOOR_JUMP_MID,
	FLOOR_JUMP_L,
	FLOOR_JUMP_R

} TileType;
// New tile map layout: all tiles are set to 0 (EMPTY)
TileType map[MAP_ROWS][MAP_COLS] = {
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ 4,	 4,	6,     EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, 0,     0,     0,	    0,	   EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, 2,
	  1,	 1,	1,     1,     1,     1,	    3,	   EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, 0,	  0,	 0,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, 8,	7,     9,     EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
};

enum screen { LEVEL_1_1 = 1, LEVEL_1_1_SCORE };

// --- Draw a single tile based on its type ---
static void draw_tile(TileType type, int x, int y)
{
	switch (type) {
	case FLOOR_MID:
		tft_draw_rect(x, y, x + TILE_SIZE, y + TILE_SIZE, GREEN);
		break;

	case FLOOR_L:
		tft_draw_rect(x, y, x + TILE_SIZE, y + TILE_SIZE, GREEN);
		break;

	case FLOOR_R:
		tft_draw_rect(x, y, x + TILE_SIZE, y + TILE_SIZE, GREEN);
		break;

	case FLOOR_WIN_MID:
		tft_draw_rect(x, y, x + TILE_SIZE, y + TILE_SIZE, WHITE);
		break;

	case FLOOR_WIN_L:
		tft_draw_rect(x, y, x + TILE_SIZE, y + TILE_SIZE, WHITE);
		break;

	case FLOOR_WIN_P:
		tft_draw_rect(x, y, x + TILE_SIZE, y + TILE_SIZE, WHITE);
		break;
	case FLOOR_JUMP_MID:
		tft_draw_rect(x, y, x + TILE_SIZE, y + TILE_SIZE, YELLOW);
		break;
	case FLOOR_JUMP_L:
		tft_draw_rect(x, y, x + TILE_SIZE, y + TILE_SIZE, YELLOW);
		break;
	case FLOOR_JUMP_R:
		tft_draw_rect(x, y, x + TILE_SIZE, y + TILE_SIZE, YELLOW);
		break;
	default: // For EMPTY and unknown types, draw a gray block
		break;
	}
}

// --- Mario's state in the tile map game ---
typedef struct {
	float px, py; // Pixel coordinates
	float vx, vy; // Velocity
	int score;
	int alive;
	int won;
	sdk_sprite_t s;
} Mario;

static Mario mario_p;

// --- Initialize the game ---
void game_start(void)
{
	mario_p.px = 4 * TILE_SIZE;
	mario_p.py = 4 * TILE_SIZE;
	mario_p.vx = 0;
	mario_p.vy = 0;
	mario_p.score = 0;
	mario_p.alive = 1;
	mario_p.won = 0;
}

void game_reset(void)
{
	game_start();
}

// --- Audio: Updated tune ---
static uint16_t tones[256];
static const char tune[] = "ACGgbGA ACEGgFD ACGgbEC FGAGFED "
			   "CEGAEC GBgfeD ACGEFGA DEFGEDC "
			   "ACGgbGA ACEGgFD ACGgbEC FGAGFED "
			   "EGCEGA DFABAG CEGBAF DFACBG "
			   "GABCDEF EGABCDE BAGFEDC ACEGFED "
			   "CEGAGEC GBgfeD ACGEFGA DEFGEDC";
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

// --- Game Input ---
void game_input(unsigned __unused dt_usec)
{
	//float dt = dt_usec / 1000000.0f;

	if (!mario_p.alive || mario_p.won) {
		if (sdk_inputs.start)
			game_reset();
		return;
	}

	// Toggle music on/off with volume buttons
	if (sdk_inputs.vol_up) {
		play_music = true;
	}
	if (sdk_inputs.vol_down) {
		play_music = false;
	}

	// Horizontal movement
	if (sdk_inputs.x > 0) { // Left
		mario_p.vx -= ACCELERATION;
		if (mario_p.vx < -MAX_SPEED)
			mario_p.vx = -MAX_SPEED;
	} else if (sdk_inputs.a > 0) { // Right
		mario_p.vx += ACCELERATION;
		if (mario_p.vx > MAX_SPEED)
			mario_p.vx = MAX_SPEED;
	} else {
		// Apply friction when no movement keys are pressed
		mario_p.vx *= FRICTION;
		if (fabs(mario_p.vx) < 0.1)
			mario_p.vx = 0; // Stop completely when velocity is very small
	}

	// Apply gravity
	mario_p.vy += GRAVITY;

	mario_p.px += mario_p.vx;
	mario_p.py += mario_p.vy;

	// Collision detection with floor tiles
	int tile_x = mario_p.px / TILE_SIZE;
	int tile_y = mario_p.py / TILE_SIZE + 1.0f / TILE_SIZE;

	// Check if Mario is standing on a FLOOR tile
	if (FLOOR_MID == map[tile_y][tile_x]) {
		mario_p.py = tile_y * TILE_SIZE - 1.0f / TILE_SIZE;
		mario_p.vy = 0;

		if (sdk_inputs.y) {
			mario_p.vy = JUMP_STRENGTH;
		}
	}

	if (FLOOR_L == map[tile_y][tile_x]) {
		mario_p.py = tile_y * TILE_SIZE - 1.0f / TILE_SIZE;
		mario_p.vy = 0;

		if (sdk_inputs.y) {
			mario_p.vy = JUMP_STRENGTH;
		}
	}

	if (FLOOR_R == map[tile_y][tile_x]) {
		mario_p.py = tile_y * TILE_SIZE - 1.0f / TILE_SIZE;
		mario_p.vy = 0;

		if (sdk_inputs.y) {
			mario_p.vy = JUMP_STRENGTH;
		}
	}

	if (FLOOR_JUMP_MID == map[tile_y][tile_x]) {
		mario_p.py = tile_y * TILE_SIZE - 1.0f / TILE_SIZE;
		mario_p.vy = 0;

		if (sdk_inputs.y) {
			mario_p.vy = 2 * JUMP_STRENGTH;
		}
	}

	if (FLOOR_JUMP_L == map[tile_y][tile_x]) {
		mario_p.py = tile_y * TILE_SIZE - 1.0f / TILE_SIZE;
		mario_p.vy = 0;

		if (sdk_inputs.y) {
			mario_p.vy = 2 * JUMP_STRENGTH;
		}
	}

	if (FLOOR_JUMP_R == map[tile_y][tile_x]) {
		mario_p.py = tile_y * TILE_SIZE - 1.0f / TILE_SIZE;
		mario_p.vy = 0;

		if (sdk_inputs.y) {
			mario_p.vy = 2 * JUMP_STRENGTH;
		}
	}

	if (FLOOR_WIN_MID == map[tile_y][tile_x]) {
		mario_p.py = tile_y * TILE_SIZE - 1.0f / TILE_SIZE;
		mario_p.vy = 0;
		mario_p.won = 1;
	}

	if (FLOOR_WIN_L == map[tile_y][tile_x]) {
		mario_p.py = tile_y * TILE_SIZE - 1.0f / TILE_SIZE;
		mario_p.vy = 0;
		mario_p.won = 1;
	}

	if (FLOOR_WIN_P == map[tile_y][tile_x]) {
		mario_p.py = tile_y * TILE_SIZE - 1.0f / TILE_SIZE;
		mario_p.vy = 0;
		mario_p.won = 1;
	}
	// Boundaries
	if (mario_p.px < 0)
		mario_p.px = 0;

	if (mario_p.px >= MAP_COLS * TILE_SIZE - TILE_SIZE)
		mario_p.px = MAP_COLS * TILE_SIZE - TILE_SIZE;

	if (mario_p.py < 0)
		mario_p.py = 0;

	if (mario_p.py >= MAP_ROWS * TILE_SIZE - TILE_SIZE) {
		mario_p.py = MAP_ROWS * TILE_SIZE - TILE_SIZE;
		mario_p.vy = 0;
	}
}

// --- Game Paint ---
void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	// Draw tile map
	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			draw_tile(map[y][x], x * TILE_SIZE, y * TILE_SIZE);
		}
	}

	tft_draw_rect(mario_p.px, mario_p.py, mario_p.px + TILE_SIZE, mario_p.py + TILE_SIZE, RED);
	// Draw Mario as a red block
	//sdk_draw_sprite(petr.p);

	tft_draw_pixel(mario_p.px + 0.5, mario_p.py - 0.5, WHITE);
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = true,
		.fps_color = GRAY,
	};
	tone_init();
	sdk_main(&config);
}
