#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <sdk.h>
#include <math.h>
//#include <stdlib.h>
#include <stdio.h>

#define TILE_SIZE 8
#define MAP_ROWS 8
#define MAP_COLS 8
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
void game_start(void)
{
	sdk_set_output_gain_db(volume);
}

void game_reset(void)
{
	game_start();
}

static uint16_t tones[256];
static const char tune[] = "";
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

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;
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
void game_paint(unsigned dt_usec)
{
	(void)dt_usec;

	tft_fill(0);

	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			//sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE, &ts_platforms_png,
			//	      map[y][x] - 1);
			//if (!map[y][x]) {
			//	tft_draw_rect(x * TILE_SIZE, y * TILE_SIZE,
			//		      x * TILE_SIZE + TILE_SIZE - 1,
			//		      y * TILE_SIZE + TILE_SIZE - 1,
			//		      rgb_to_rgb565(0, 0, 0));
			//	continue;
			//}
		}
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
	tone_init();
	sdk_main(&config);
}
