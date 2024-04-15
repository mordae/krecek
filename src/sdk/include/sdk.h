#pragma once
#include <pico/stdlib.h>

#include <sdk/util.h>
#include <sdk/embed.h>

struct sdk_config {
	bool wait_for_usb;
	bool show_fps;
	bool off_on_select;

	uint16_t fps_color;
	unsigned brightness; // 0-256
};

struct sdk_inputs {
	int8_t a, b, x, y;
	int8_t start, select;
	int8_t joy_sw;
	int8_t vol_up, vol_down, vol_sw;
	int8_t aux[8];

	int16_t joy_x, joy_y;
	int16_t brack_l, brack_r;

	float batt_mv;
};

extern struct sdk_inputs sdk_inputs;
extern struct sdk_inputs sdk_inputs_delta;

extern struct sdk_config sdk_config;

void game_start(void);
void game_reset(void);
void game_audio(int nsamples);
void game_input(unsigned dt);
void game_paint(unsigned dt);

void sdk_main(struct sdk_config *conf);

/*
 * Set screen brightness (actually the PWM threshold).
 * Acceptable values are 0 to 256.
 */
void sdk_set_screen_brightness(unsigned level);
#define SDK_BRIGHTNESS_MIN 2
#define SDK_BRIGHTNESS_STD 128
#define SDK_BRIGHTNESS_MAX 256

void sdk_set_output_gain_db(float gain);
bool sdk_write_sample(int16_t sample);
bool sdk_read_sample(int16_t *sample);
int sdk_write_samples(const int16_t *buf, int len);
int sdk_read_samples(int16_t *buf, int len);

/*
 * Can be called very often, will actually only yield after
 * given number of microseconds since the last actual yield.
 */
void sdk_yield_every_us(unsigned us);
