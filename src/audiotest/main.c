#include <math.h>
#include <pico/stdlib.h>

#include <sdk.h>
#include <stdlib.h>
#include <tft.h>

typedef enum {
	MODE_INPUT = 0,
	MODE_OUTPUT,
	NUM_MODES,
} Mode;

static Mode mode = MODE_INPUT;

static int16_t audio_left[TFT_WIDTH];
static int16_t audio_right[TFT_WIDTH];
static int audio_idx;

static uint32_t phase = 0;

/*
 * Given phase, outputs sine approximation in the range of [-8192, +8192].
 */
inline static __unused int fakesin(uint32_t phase)
{
	int x = (int)phase >> 16;
	return x - ((x * abs(x)) >> 15);
}

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
		for (int i = 0; i < nsamples; i++) {
			uint32_t step = UINT32_MAX / (SDK_AUDIO_RATE / 1000);
			phase += step;

			int left = fakesin(phase);
			//int right = sinf((int32_t)phase * (M_PI / INT32_MAX)) * 8192;
			int right = left;

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
