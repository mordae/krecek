#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <stdio.h>
#include <math.h> // Include math.h for fabs

// Screen and tile dimensions
#define SCREEN_WIDTH 140
#define SCREEN_HEIGHT 120
#define TILE_SIZE 8
#define MAP_ROWS 15
#define MAP_COLS 20

// Physics constants
#define GRAVITY 1.5	  // Gravity for falling
#define JUMP_STRENGTH -12 // Jump strength
#define MAX_SPEED 2.0	  // Max speed for movement
#define ACCELERATION 0.3  // Acceleration for movement
#define FRICTION 0.7	  // Friction to stop sliding quickly

// Color definitions (using palette indices)
#define RED 255
#define YELLOW 242
#define GREEN 244
#define BLUE 250
#define GRAY 8
#define WHITE 15

// Define tile types using typedef enum
typedef enum { EMPTY = 0, FLOOR = 1 } TileType;

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
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, 1,	1,     EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
};

// --- Draw a single tile based on its type ---
static void draw_tile(TileType type, int x, int y)
{
	switch (type) {
	case FLOOR:
		tft_draw_rect(x, y, x + TILE_SIZE, y + TILE_SIZE, GREEN);
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

	// Update pixel position
	mario_p.px += mario_p.vx;
	mario_p.py += mario_p.vy;

	// Collision detection with floor tiles
	int tile_x = mario_p.px / TILE_SIZE;
	int tile_y = mario_p.py / TILE_SIZE + 1.0f / TILE_SIZE;

	// Check if Mario is standing on a FLOOR tile
	if (FLOOR == map[tile_y][tile_x]) {
		// Snap to the top of the tile
		mario_p.py = tile_y * TILE_SIZE - 1.0f / TILE_SIZE;
		mario_p.vy = 0;

		if (sdk_inputs.y) {
			mario_p.vy = JUMP_STRENGTH;
		}
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

	// Draw Mario as a red block
	tft_draw_rect(mario_p.px - TILE_SIZE / 2.0, mario_p.py, mario_p.px + TILE_SIZE / 2.0,
		      mario_p.py - TILE_SIZE, RED);
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
