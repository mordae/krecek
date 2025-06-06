#include <limits.h>
#include <math.h>
#include <pico/stdlib.h>

#include <sdk.h>
#include <stdlib.h>
#include <tft.h>

typedef enum {
	MODE_INPUT = 0,
	MODE_OUTPUT,
	MODE_IR,
	NUM_MODES,
} Mode;

typedef enum {
	WAVE_SINE = 0,
	WAVE_SQUARE = 1,
	NUM_WAVES,
} Wave;

typedef enum {
	SCALE_LINEAR = 0,
	SCALE_LOG,
	NUM_SCALES,
} Scale;

static Mode mode = MODE_INPUT;
static Wave wave = WAVE_SINE;
static Scale scale = SCALE_LINEAR;

static int16_t audio_left[TFT_WIDTH];
static int16_t audio_right[TFT_WIDTH];
static int16_t audio_demod[TFT_WIDTH];
static int audio_idx;
static float test_freq = 1000;

// Demodulated bits.
static uint32_t bits;

static uint32_t my_code;

void game_start(void)
{
	my_code = rand();
}

void game_inbox(sdk_message_t msg)
{
	if (SDK_MSG_IR == msg.type) {
		bits = msg.ir.data;

		if (bits != my_code)
			sdk_melody_play("/i:prnl /bpm:160 (fff) e_ e_ e_");
	}
}

void game_audio(int nsamples)
{
	if (MODE_INPUT == mode) {
		for (int i = 0; i < nsamples; i++) {
			sdk_read_sample(&audio_left[audio_idx], &audio_right[audio_idx]);
			sdk_write_sample(audio_left[audio_idx], audio_right[audio_idx]);
			audio_idx = (audio_idx + 1) % TFT_WIDTH;
		}

		for (int i = 0; i < nsamples; i++)
			sdk_write_sample(0, 0);
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
	} else if (MODE_IR == mode) {
		for (int i = 0; i < nsamples; i++) {
			int16_t left, right;
			sdk_melody_sample(&left, &right);
			sdk_write_sample(left, right);

			sdk_read_sample(&left, &right);

			int I, Q, dm = INT_MIN;
			sdk_decode_ir_raw(left, &I, &Q, &dm);

			if (dm != INT_MIN) {
				audio_left[audio_idx] = I;
				audio_right[audio_idx] = Q;
				audio_demod[audio_idx] = dm;
				audio_idx = (audio_idx + 1) % TFT_WIDTH;
			}
		}
	}
}

void game_input(unsigned dt_usec)
{
	(void)dt_usec;

	if (sdk_inputs_delta.select > 0)
		mode = (mode + 1) % NUM_MODES;

	if (MODE_INPUT == mode) {
		// Nothing yet.
	} else if (MODE_OUTPUT == mode) {
		if (sdk_inputs_delta.start > 0)
			wave = (wave + 1) % NUM_WAVES;

		if (sdk_inputs.y)
			test_freq = MIN(test_freq * 1.01f, 20000);

		if (sdk_inputs.a)
			test_freq = MAX(test_freq * 0.99f, 100);
	} else if (MODE_IR == mode) {
		if (sdk_inputs_delta.start > 0)
			sdk_send_ir(0xcafecafe);

		if (sdk_inputs_delta.a > 0)
			sdk_send_ir(0xdefea7ed);

		if (sdk_inputs_delta.y > 0)
			sdk_send_ir(my_code);
	}

	if (sdk_inputs_delta.b > 0)
		scale = (scale + 1) % NUM_SCALES;
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;

	tft_fill(0);

	uint16_t left_color = (MODE_INPUT == mode) ? rgb_to_rgb565(255, 255, 63) :
						     rgb_to_rgb565(255, 63, 63);
	uint16_t right_color = (MODE_INPUT == mode) ? rgb_to_rgb565(127, 127, 255) :
						      rgb_to_rgb565(63, 255, 63);
	uint16_t demod_color = rgb_to_rgb565(127, 127, 255);

	if (MODE_OUTPUT == mode) {
		tft_draw_string_right(TFT_WIDTH - 1, 0, rgb_to_rgb565(127, 127, 127), "%s %.0f Hz",
				      wave == WAVE_SINE ? "sine" : "square", test_freq);
	} else if (MODE_INPUT == mode) {
		tft_draw_string_right(TFT_WIDTH - 1, TFT_HEIGHT - 13, rgb_to_rgb565(127, 127, 255),
				      "%.2f mV", sdk_inputs.hps_mv);
	} else if (MODE_IR == mode) {
		tft_draw_string_right(TFT_WIDTH - 1, TFT_HEIGHT - 13, rgb_to_rgb565(127, 127, 255),
				      "%08x", (unsigned)bits);
	}

	int left_baseline = TFT_HEIGHT / 4;
	int right_baseline = TFT_HEIGHT * 3 / 4;
	int demod_baseline = right_baseline;

	if (MODE_IR == mode)
		right_baseline = left_baseline;

	for (int x = 0; x < TFT_WIDTH; x++) {
		tft_draw_pixel(x, left_baseline, rgb_to_rgb565(31, 31, 31));
		tft_draw_pixel(x, right_baseline, rgb_to_rgb565(31, 31, 31));
		tft_draw_pixel(x, demod_baseline, rgb_to_rgb565(31, 31, 31));

		int i = (audio_idx + TFT_WIDTH + x) % TFT_WIDTH;

		int left = 0, right = 0, demod = 0;

		if (SCALE_LINEAR == scale) {
			left = audio_left[i] / 1093;
			right = audio_right[i] / 1093;
			demod = audio_demod[i] / 1093;
		} else if (SCALE_LOG == scale) {
			left = sign(audio_left[i]) * 2 * log2f(abs(audio_left[i] | 1));
			right = sign(audio_right[i]) * 2 * log2f(abs(audio_right[i] | 1));
			demod = sign(audio_demod[i]) * 2 * log2f(abs(audio_demod[i] | 1));
		}

		tft_draw_pixel(x, left_baseline + left, left_color);
		tft_draw_pixel(x, right_baseline + right, right_color);

		if (MODE_IR == mode)
			tft_draw_pixel(x, demod_baseline + demod, demod_color);
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
