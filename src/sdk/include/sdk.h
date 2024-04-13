#pragma once
#include <pico/stdlib.h>

struct sdk_config {
	bool wait_for_usb;
	bool show_fps;
	bool off_on_select;

	uint8_t brightness;
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

struct sdk_file {
	const void *data;
	size_t len;
};

#define embed_file(name, path)                       \
	extern struct sdk_file name;                 \
	__asm__(".section \".text.files\", \"a\"\n"  \
		"_" #name "_start:\n"                \
		".incbin \"" path "\"\n" #name ":\n" \
		".int _" #name "_start\n"            \
		".int " #name " - _" #name "_start\n")

extern struct sdk_inputs sdk_inputs;
extern struct sdk_inputs sdk_inputs_delta;

extern struct sdk_config sdk_config;

void game_start(void);
void game_reset(void);
void game_audio(int nsamples);
void game_input(void);
void game_paint(unsigned dt);

void sdk_main(struct sdk_config *conf);
void sdk_set_screen_brightness(uint8_t level);
void sdk_set_output_gain_db(float gain);
bool sdk_write_sample(int16_t sample);
bool sdk_read_sample(int16_t *sample);
int sdk_write_samples(const int16_t *buf, int len);
int sdk_read_samples(int16_t *buf, int len);
