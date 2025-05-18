#include <math.h>
#include <pico/stdlib.h>

#include <hardware/adc.h>
#include <hardware/clocks.h>
#include <hardware/dma.h>
#include <hardware/gpio.h>

#include <hardware/regs/adc.h>
#include <hardware/regs/clocks.h>
#include <hardware/regs/io_qspi.h>
#include <hardware/regs/pads_qspi.h>
#include <hardware/regs/pll.h>
#include <hardware/regs/pwm.h>
#include <hardware/regs/resets.h>
#include <hardware/regs/xosc.h>

#include <stdio.h>

#include <stdlib.h>
#include <task.h>

#include <sdk.h>
#include <sdk/slave.h>

/* From video.c */
void sdk_video_start(void);

struct sdk_inputs sdk_inputs = {};
struct sdk_inputs sdk_inputs_delta = {};

bool sdk_requested_screenshot = false;

static struct sdk_inputs prev_inputs = {};

static uint32_t select_held_since = 0;

void sdk_input_init(void)
{
}

static int adc_repeated_read(int n)
{
	int value = 0;

	for (int i = 0; i < n; i++)
		value += adc_read();

	return value;
}

void sdk_input_task(void)
{
	game_reset();
	sdk_video_start();

	static int batt_hist[16], batt_idx = 0, batt_avg = 0;
	static int temp_hist[16], temp_idx = 0, temp_avg = 0;
	static int cc1_hist[16], cc1_idx = 0, cc1_avg = 0;
	static int cc2_hist[16], cc2_idx = 0, cc2_avg = 0;
	static int hps_hist[16], hps_idx = 0, hps_avg = 0;

	uint32_t now = time_us_32();
	uint32_t last_event = now - 1;

	while (true) {
		now = time_us_32();
		uint32_t dt_usec = now - last_event;
		last_event = now;

		float fdt = dt_usec / 1000000.0f;

		static struct mailbin mailbin;
		static uint32_t stdout_tail = 0;
		static bool ready = false;

		if (!sdk_slave_fetch_mailbin(&mailbin))
			continue;

		if (MAILBIN_MAGIC != mailbin.magic)
			continue;

		if (!ready) {
			ready = true;
			puts("sdk: mailbin ready");
		}

		if (stdout_tail != mailbin.stdout_head) {
			printf("\x1b[44m");

			while (stdout_tail != mailbin.stdout_head) {
				int c = mailbin.stdout_buffer[stdout_tail++ % MAILBIN_STDOUT_SIZE];
				putchar(c);
			}

			printf("\x1b[0m");
		}

		sdk_inputs.a = !((mailbin.gpio_input >> SLAVE_A_PIN) & 1);
		sdk_inputs.b = !((mailbin.gpio_input >> SLAVE_B_PIN) & 1);
		sdk_inputs.x = !((mailbin.gpio_input >> SLAVE_X_PIN) & 1);
		sdk_inputs.y = !((mailbin.gpio_input >> SLAVE_Y_PIN) & 1);

		sdk_inputs.vol_up = !((mailbin.qspi_input >> SLAVE_VOL_UP_QSPI_PIN) & 1);
		sdk_inputs.vol_down = !((mailbin.qspi_input >> SLAVE_VOL_DOWN_QSPI_PIN) & 1);
		sdk_inputs.vol_sw = !((mailbin.qspi_input >> SLAVE_VOL_SW_QSPI_PIN) & 1);

		sdk_inputs.aux[0] = !((mailbin.gpio_input >> SLAVE_AUX0_PIN) & 1);
		sdk_inputs.aux[1] = !((mailbin.gpio_input >> SLAVE_AUX1_PIN) & 1);
		sdk_inputs.aux[2] = !((mailbin.gpio_input >> SLAVE_AUX2_PIN) & 1);
		sdk_inputs.aux[3] = !((mailbin.gpio_input >> SLAVE_AUX3_PIN) & 1);
		sdk_inputs.aux[0] = !((mailbin.gpio_input >> SLAVE_AUX4_PIN) & 1);
		sdk_inputs.aux[1] = !((mailbin.gpio_input >> SLAVE_AUX5_PIN) & 1);
		sdk_inputs.aux[2] = !((mailbin.gpio_input >> SLAVE_AUX6_PIN) & 1);
		sdk_inputs.aux[3] = !((mailbin.gpio_input >> SLAVE_AUX7_PIN) & 1);

		sdk_inputs.start = !((mailbin.gpio_input >> SLAVE_START_PIN) & 1);
		sdk_inputs.select = !((mailbin.qspi_input >> SLAVE_SELECT_QSPI_PIN) & 1);

		float tx = 1.0f - mailbin.touch[0] / (float)UINT16_MAX;
		float ty = mailbin.touch[1] / (float)UINT16_MAX;
		float z1 = 1.0f - mailbin.touch[2] / (float)UINT16_MAX;
		float z2 = mailbin.touch[3] / (float)UINT16_MAX;

		float tp = 0.9f * z2 * (2.0f + 1.8f * tx + 0.7f * ty) / ty;
		tp = powf(clamp(tp, 0, 1), 2);

		tx = clamp(tx / 0.9f, 0, 1);
		ty = clamp((ty - 0.1f) / 0.9f, 0, 1);

		sdk_inputs.tx = tx;
		sdk_inputs.ty = ty;
		sdk_inputs.t1 = z1;
		sdk_inputs.t2 = z2;
		sdk_inputs.tp = tp >= 0.01f ? tp : 0.0f;

		int joy_x = INT16_MAX - mailbin.adc[SLAVE_JOY_X_PIN - 26];
		int joy_y = INT16_MAX - mailbin.adc[SLAVE_JOY_Y_PIN - 26];
		int brack_l = INT16_MAX - mailbin.adc[SLAVE_BRACK_L_PIN - 26];
		int brack_r = INT16_MAX - mailbin.adc[SLAVE_BRACK_R_PIN - 26];

		sdk_inputs.joy_x = joy_x >> 4;
		sdk_inputs.joy_y = joy_y >> 4;
		sdk_inputs.brack_l = brack_l >> 4;
		sdk_inputs.brack_r = brack_r >> 4;

		sdk_inputs.jx = clamp(joy_x / (float)INT16_MAX, -1, 1);
		sdk_inputs.jy = clamp(joy_y / (float)INT16_MAX, -1, 1);
		sdk_inputs.bl = clamp(brack_l / (float)INT16_MAX, -1, 1);
		sdk_inputs.br = clamp(brack_r / (float)INT16_MAX, -1, 1);

		if (abs(sdk_inputs.joy_x) >= 512) {
			if (!sdk_inputs.horizontal ||
			    (prev_inputs.horizontal > 0) != (sdk_inputs.horizontal > 0)) {
				sdk_inputs.horizontal = sdk_inputs.joy_x > 0 ? 256 : -1;
				prev_inputs.horizontal = 0;
			} else {
				sdk_inputs.horizontal += (sdk_inputs.joy_x * (int)dt_usec) >> 21;
			}
		} else {
			sdk_inputs.horizontal = 0;
			prev_inputs.horizontal = 0;
		}

		if (abs(sdk_inputs.joy_y) >= 512) {
			if (!sdk_inputs.vertical ||
			    (prev_inputs.vertical > 0) != (sdk_inputs.vertical > 0)) {
				sdk_inputs.vertical = sdk_inputs.joy_y > 0 ? 256 : -1;
				prev_inputs.vertical = 0;
			} else {
				sdk_inputs.vertical += (sdk_inputs.joy_y * (int)dt_usec) >> 21;
			}
		} else {
			sdk_inputs.vertical = 0;
			prev_inputs.vertical = 0;
		}

		/* Read battery voltage. */
		adc_select_input(BAT_VSENSE_PIN - 26);
		int batt = adc_repeated_read(16);
		batt_avg -= batt_hist[batt_idx];
		batt_avg += batt;
		batt_hist[batt_idx] = batt;
		batt_idx = (batt_idx + 1) & 15;

		sdk_inputs.batt_mv = batt_avg * (1.6f / 256.0f);

		/* Read current control USB pins. */
		adc_select_input(USB_CC1_PIN - 26);
		int cc1 = adc_repeated_read(16);
		cc1_avg -= cc1_hist[cc1_idx];
		cc1_avg += cc1;
		cc1_hist[cc1_idx] = cc1;
		cc1_idx = (cc1_idx + 1) & 15;
		float cc1_mv = cc1_avg * (3300.0f / (4095 * 256.0f));

		adc_select_input(USB_CC2_PIN - 26);
		int cc2 = adc_repeated_read(16);
		cc2_avg -= cc2_hist[cc2_idx];
		cc2_avg += cc2;
		cc2_hist[cc2_idx] = cc2;
		cc2_idx = (cc2_idx + 1) & 15;
		float cc2_mv = cc2_avg * (3300.0f / (4095 * 256.0f));

		sdk_inputs.cc_mv = MAX(cc1_mv, cc2_mv);

		/* Read headphone sense. */
		adc_select_input(HP_SENSE_PIN - 26);
		int hps = adc_repeated_read(16);
		hps_avg -= hps_hist[hps_idx];
		hps_avg += hps;
		hps_hist[hps_idx] = hps;
		hps_idx = (hps_idx + 1) & 15;
		float hps_mv = hps_avg * (3300.0f / (4095 * 256.0f));

		sdk_inputs.hps_mv = hps_mv;
		sdk_inputs.hps = hps_mv < 1900.0f;

		/* Read current temperature. */
		adc_select_input(4);
		int temp = adc_repeated_read(16);
		temp_avg -= temp_hist[temp_idx];
		temp_avg += temp;
		temp_hist[temp_idx] = temp;
		temp_idx = (temp_idx + 1) & 15;

		float temp_v = temp_avg * (3.300f / (4095 * 256.0f));
		sdk_inputs.temp = 27.0f - (temp_v - 0.706f) * (1.0f / 0.001721f);

		/* Calculate input deltas */
		sdk_inputs_delta.a = sdk_inputs.a - prev_inputs.a;
		sdk_inputs_delta.b = sdk_inputs.b - prev_inputs.b;
		sdk_inputs_delta.x = sdk_inputs.x - prev_inputs.x;
		sdk_inputs_delta.y = sdk_inputs.y - prev_inputs.y;

		sdk_inputs_delta.joy_x = roundf(sdk_inputs.joy_x * fdt);
		sdk_inputs_delta.joy_y = roundf(sdk_inputs.joy_y * fdt);

		sdk_inputs_delta.jx = roundf(sdk_inputs.jx * fdt);
		sdk_inputs_delta.jy = roundf(sdk_inputs.jx * fdt);

		sdk_inputs_delta.horizontal =
			(sdk_inputs.horizontal >> 8) - (prev_inputs.horizontal >> 8);
		sdk_inputs_delta.vertical =
			(sdk_inputs.vertical >> 8) - (prev_inputs.vertical >> 8);

		sdk_inputs_delta.vol_up = sdk_inputs.vol_up - prev_inputs.vol_up;
		sdk_inputs_delta.vol_down = sdk_inputs.vol_down - prev_inputs.vol_down;
		sdk_inputs_delta.vol_sw = sdk_inputs.vol_sw - prev_inputs.vol_sw;

		sdk_inputs_delta.hps = sdk_inputs.hps - prev_inputs.hps;

		sdk_inputs_delta.brack_l = roundf(sdk_inputs.brack_l * fdt);
		sdk_inputs_delta.brack_r = roundf(sdk_inputs.brack_r * fdt);

		sdk_inputs_delta.bl = roundf(sdk_inputs.bl * fdt);
		sdk_inputs_delta.br = roundf(sdk_inputs.br * fdt);

		for (int i = 0; i < 8; i++)
			sdk_inputs_delta.aux[i] = sdk_inputs.aux[i] - prev_inputs.aux[i];

		sdk_inputs_delta.start = sdk_inputs.start - prev_inputs.start;
		sdk_inputs_delta.select = sdk_inputs.select - prev_inputs.select;

		prev_inputs = sdk_inputs;

		sdk_inputs_delta.batt_mv = sdk_inputs.batt_mv - prev_inputs.batt_mv;
		sdk_inputs_delta.hps_mv = sdk_inputs.hps_mv - prev_inputs.hps_mv;

		if (sdk_inputs.cc_mv < 250.0f) {
			if (sdk_config.off_on_select && sdk_inputs.select) {
				if (!select_held_since) {
					select_held_since = time_us_32();
				} else {
					uint32_t select_held_for = time_us_32() - select_held_since;
					if (select_held_for > 3000000) {
						puts("sdk: SELECT held for 3s, turning off...");
						sdk_turn_off();
						select_held_since = 0;
					}
				}
			} else {
				select_held_since = 0;
			}
		}

		static int charging_mode = 0;

		if (sdk_inputs.cc_mv > 700.0f) {
			/* We can draw 1.5A, so max the charger at 1A. */
			gpio_put(CHG_ISET2_PIN, 0);
			gpio_set_dir(CHG_ISET2_PIN, GPIO_OUT);

			if (charging_mode != 2) {
				printf("\x1b[1;31msdk: charging at 1A\e[0m\n");
				charging_mode = 2;
			}
		} else if (sdk_inputs.cc_mv > 250.0f) {
			/* We can draw 500-900mA, so limit charger to 500mA. */
			gpio_put(CHG_ISET2_PIN, 1);
			gpio_set_dir(CHG_ISET2_PIN, GPIO_OUT);

			if (charging_mode != 1) {
				printf("\x1b[1;31msdk: charging at 500mA\e[0m\n");
				charging_mode = 1;
			}
		} else {
			/* Limit charger to 100mA. */
			gpio_set_dir(CHG_ISET2_PIN, GPIO_IN);

			if (charging_mode != 0) {
				printf("\x1b[1;31msdk: charging at 100mA\e[0m\n");
				charging_mode = 0;
			}
		}

		if (sdk_inputs_delta.hps > 0) {
			// Enable headphones, disable speaker.
			sdk_enable_headphones(true);
		} else if (sdk_inputs_delta.hps < 0) {
			// Disable headphones, enable speaker.
			sdk_enable_headphones(false);
		}

		// On select + start reboot to the slot 0.
		if (sdk_inputs.select && sdk_inputs.start > 0)
			sdk_reboot_into_slot(0);

		// On select + Y take a screenshot.
		if (sdk_inputs.select && sdk_inputs_delta.y > 0) {
			sdk_requested_screenshot = true;
			sdk_inputs_delta.y = 0;
		}

		// Prevent inputs while holding select.
		if (sdk_inputs.select) {
			sdk_inputs.a = prev_inputs.a;
			sdk_inputs.b = prev_inputs.b;
			sdk_inputs.x = prev_inputs.x;
			sdk_inputs.y = prev_inputs.y;
			sdk_inputs.start = prev_inputs.start;

			sdk_inputs_delta.a = 0;
			sdk_inputs_delta.b = 0;
			sdk_inputs_delta.x = 0;
			sdk_inputs_delta.y = 0;
			sdk_inputs_delta.start = 0;
		}

		/* Let the game process inputs as soon as possible. */
		game_input(dt_usec);

		task_sleep_ms(9);
	}
}
