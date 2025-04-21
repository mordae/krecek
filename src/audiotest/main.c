#include <math.h>
#include <pico/stdlib.h>

#include <sdk.h>
#include <stdio.h>
#include <tft.h>

typedef enum {
	MODE_INPUT = 0,
	MODE_OUTPUT,
	NUM_MODES,
} Mode;

typedef enum {
	WAVE_SINE = 0,
	WAVE_SQUARE = 1,
	NUM_WAVES,
} Wave;

static Mode mode = MODE_INPUT;
static Wave wave = WAVE_SINE;

static int16_t audio_left[TFT_WIDTH];
static int16_t audio_right[TFT_WIDTH];
static int audio_idx;
static float test_freq = 1000;

void game_audio(int nsamples)
{
	if (MODE_INPUT == mode) {
		for (int i = 0; i < nsamples; i++) {
			sdk_read_sample(&audio_left[audio_idx], &audio_right[audio_idx]);
			audio_idx = (audio_idx + 1) % TFT_WIDTH;
		}

		for (int i = 0; i < nsamples; i++) {
			sdk_write_sample(0, 0);
		}
	} else if (MODE_OUTPUT == mode) {
		static uint32_t phase_left;
		static uint32_t phase_right;

		uint32_t step_left = UINT32_MAX / (SDK_AUDIO_RATE / test_freq);
		uint32_t step_right = UINT32_MAX / (SDK_AUDIO_RATE / (test_freq * 1.25));

		for (int i = 0; i < nsamples; i++) {
			phase_left += step_left;
			phase_right += step_right;

			int left, right;

			if (WAVE_SINE == wave) {
				left = sinf((int)phase_left * M_PI / INT32_MAX) * 32767;
				right = sinf((int)phase_right * M_PI / INT32_MAX) * 32767;
			} else if (WAVE_SQUARE == wave) {
				left = (phase_left >> 31) ? -32767 : 32767;
				right = (phase_right >> 31) ? -32767 : 32767;
			} else {
				left = 0;
				right = 0;
			}

			audio_left[audio_idx] = left;
			audio_right[audio_idx] = right;
			audio_idx = (audio_idx + 1) % TFT_WIDTH;

			sdk_write_sample(left, right);
		}
	}
}

void game_input(unsigned dt_usec)
{
	(void)dt_usec;

	if (sdk_inputs_delta.start > 0)
		mode = (mode + 1) % NUM_MODES;

	if (sdk_inputs_delta.select > 0)
		wave = (wave + 1) % NUM_WAVES;

	if (sdk_inputs.y)
		test_freq = MIN(test_freq * 1.01f, 20000);

	if (sdk_inputs.a)
		test_freq = MAX(test_freq * 0.99f, 100);
}

int sign(int x)
{
	return x > 0 ? 1 : -1;
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;

	tft_fill(0);

	uint16_t left_color = (MODE_INPUT == mode) ? rgb_to_rgb565(255, 255, 63) :
						     rgb_to_rgb565(255, 63, 63);
	uint16_t right_color = (MODE_INPUT == mode) ? rgb_to_rgb565(127, 127, 255) :
						      rgb_to_rgb565(63, 255, 63);

	if (MODE_OUTPUT == mode) {
		char buf[32];
		sprintf(buf, "%s %.0f Hz", wave == WAVE_SINE ? "sine" : "square", test_freq);
		tft_draw_string_right(TFT_WIDTH - 1, 0, rgb_to_rgb565(127, 127, 127), buf);
	}

	for (int x = 0; x < TFT_WIDTH; x++) {
		tft_draw_pixel(x, TFT_HEIGHT / 4, rgb_to_rgb565(63, 63, 63));
		tft_draw_pixel(x, TFT_HEIGHT * 3 / 4, rgb_to_rgb565(63, 63, 63));

		int i = (audio_idx + TFT_WIDTH + x) % TFT_WIDTH;

		int left = audio_left[i] / 1093;
		int right = audio_right[i] / 1093;

		// int left = sign(audio_left[i]) * 2 * log2f(abs(audio_left[i]));
		// int right = sign(audio_right[i]) * 2 * log2f(abs(audio_right[i]));

		tft_draw_pixel(x, TFT_HEIGHT / 4 + left, left_color);
		tft_draw_pixel(x, TFT_HEIGHT * 3 / 4 + right, right_color);
	}
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = false,
		.off_on_select = true,
		.fps_color = rgb_to_rgb565(31, 31, 31),
	};

	sdk_main(&config);
}
