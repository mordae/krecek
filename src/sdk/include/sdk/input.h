#pragma once
#include <pico/stdlib.h>

struct sdk_inputs {
	int8_t a, b, x, y;
	int8_t start, select;
	int8_t vol_up, vol_down, vol_sw;
	int8_t aux[8];
	int8_t hps;

	int horizontal, vertical;

	int16_t joy_x, joy_y;
	int16_t brack_l, brack_r;

	float batt_mv, cc_mv, temp, hps_mv;
	float tx, ty, t1, t2, tp;
	float jx, jy, bl, br;
};

extern struct sdk_inputs sdk_inputs;
extern struct sdk_inputs sdk_inputs_delta;

extern bool sdk_requested_screenshot;
