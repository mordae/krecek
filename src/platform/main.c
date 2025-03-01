#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <math.h>
#include <string.h>

#define SCREEN_WIDTH 140
#define SCREEN_HEIGHT 120

#define PLAYER_WIDTH 8
#define PLAYER_HEIGHT 10
#define GROUND_Y (SCREEN_HEIGHT - 10)
#define GRAVITY 0.3f
#define JUMP_STRENGTH -4.5f
#define MOVE_SPEED 2

#define NUM_PLATFORMS 5
#define NUM_ENEMIES 2

struct player {
	float x, y;
	float dy;
	int on_ground;
	int alive;
	int won;
};

struct platform {
	int x, y, width, height;
};

struct enemy {
	int x, y, width, height;
	int direction;
};

static struct player mario;
static struct platform platforms[NUM_PLATFORMS];
static struct enemy enemies[NUM_ENEMIES];

static uint16_t tones[256];
static const char tune[] =
	" ggahC g C g C g CChag D g D g D ggahC g C g C g CCDEF C F C F C FFEDC g C g C g CChag D g D g D";
static bool play_music = true;

void game_start(void)
{
	sdk_set_output_gain_db(6);
	tft_palette[1] = rgb_to_rgb565(255, 0, 0);
	tft_palette[2] = rgb_to_rgb565(0, 255, 0);
	tft_palette[3] = rgb_to_rgb565(255, 255, 0);
	tft_palette[4] = rgb_to_rgb565(0, 0, 255);

	mario.x = 10;
	mario.y = GROUND_Y - PLAYER_HEIGHT;
	mario.dy = 0;
	mario.on_ground = 1;
	mario.alive = 1;
	mario.won = 0;

	platforms[0] = (struct platform){ 30, 90, 40, 5 };
	platforms[1] = (struct platform){ 80, 70, 40, 5 };
	platforms[2] = (struct platform){ 50, 50, 30, 5 };
	platforms[3] = (struct platform){ 100, 30, 30, 5 };
	platforms[4] = (struct platform){ 120, 20, 20, 5 };

	enemies[0] = (struct enemy){ 60, GROUND_Y - 10, 10, 10, 1 };
	enemies[1] = (struct enemy){ 110, 50, 10, 10, -1 };

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

void game_input(unsigned __unused dt_usec)
{
	if (!mario.alive || mario.won) {
		if (sdk_inputs.start) {
			game_start();
		}
		return;
	}

	if (sdk_inputs.x)
		mario.x -= MOVE_SPEED;
	if (sdk_inputs.a)
		mario.x += MOVE_SPEED;
	if (sdk_inputs.y && mario.on_ground) {
		mario.dy = JUMP_STRENGTH;
		mario.on_ground = 0;
	}

	mario.dy += GRAVITY;
	mario.y += mario.dy;

	if (mario.y >= GROUND_Y - PLAYER_HEIGHT) {
		mario.y = GROUND_Y - PLAYER_HEIGHT;
		mario.dy = 0;
		mario.on_ground = 1;
	}
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);
	tft_draw_rect(mario.x, mario.y, mario.x + PLAYER_WIDTH, mario.y + PLAYER_HEIGHT, 1);
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = false,
		.off_on_select = true,
		.fps_color = 3,
	};
	sdk_main(&config);
}
