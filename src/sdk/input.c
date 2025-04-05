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

#include <task.h>

#include <sdk.h>
#include <sdk/remote.h>

/* From video.c */
void sdk_video_start(void);

struct sdk_inputs sdk_inputs = {};
struct sdk_inputs sdk_inputs_delta = {};

static struct sdk_inputs prev_inputs = {};

static uint32_t select_held_since = 0;

void sdk_input_init(void)
{
	/* Enable ADC. */
	remote_poke(ADC_BASE + ADC_CS_OFFSET, ADC_CS_EN_BITS);

	/* A, B, X, Y, START - pullup */
	remote_gpio_pad_set(0, 1, 1, 1, 0, 1, 0, SLAVE_A_PIN);
	remote_gpio_pad_set(0, 1, 1, 1, 0, 1, 0, SLAVE_B_PIN);
	remote_gpio_pad_set(0, 1, 1, 1, 0, 1, 0, SLAVE_X_PIN);
	remote_gpio_pad_set(0, 1, 1, 1, 0, 1, 0, SLAVE_Y_PIN);
	remote_gpio_pad_set(0, 1, 1, 1, 0, 1, 0, SLAVE_START_PIN);

	/* AUX[0-7] - pullup */
	remote_gpio_pad_set(0, 1, 1, 1, 0, 1, 0, SLAVE_AUX0_PIN);
	remote_gpio_pad_set(0, 1, 1, 1, 0, 1, 0, SLAVE_AUX1_PIN);
	remote_gpio_pad_set(0, 1, 1, 1, 0, 1, 0, SLAVE_AUX2_PIN);
	remote_gpio_pad_set(0, 1, 1, 1, 0, 1, 0, SLAVE_AUX3_PIN);
	remote_gpio_pad_set(0, 1, 1, 1, 0, 1, 0, SLAVE_AUX4_PIN);
	remote_gpio_pad_set(0, 1, 1, 1, 0, 1, 0, SLAVE_AUX5_PIN);
	remote_gpio_pad_set(0, 1, 1, 1, 0, 1, 0, SLAVE_AUX6_PIN);
	remote_gpio_pad_set(0, 1, 1, 1, 0, 1, 0, SLAVE_AUX7_PIN);

	/* BRACK_R, BRACK_L, JOY_X, JOY_Y - no pulls */
	remote_gpio_pad_set(0, 1, 1, 0, 0, 1, 0, SLAVE_BRACK_L_PIN);
	remote_gpio_pad_set(0, 1, 1, 0, 0, 1, 0, SLAVE_BRACK_R_PIN);
	remote_gpio_pad_set(0, 1, 1, 0, 0, 1, 0, SLAVE_JOY_X_PIN);
	remote_gpio_pad_set(0, 1, 1, 0, 0, 1, 0, SLAVE_JOY_Y_PIN);

	/* VOL_UP, VOL_DOWN, VOL_SW, SELECT - pullup */
	remote_qspi_pad_set(1, 1, 1, 1, 0, 1, 0, SLAVE_VOL_UP_QSPI_PIN);
	remote_qspi_pad_set(1, 1, 1, 1, 0, 1, 0, SLAVE_VOL_DOWN_QSPI_PIN);
	remote_qspi_pad_set(1, 1, 1, 1, 0, 1, 0, SLAVE_VOL_SW_QSPI_PIN);
	remote_qspi_pad_set(1, 1, 1, 1, 0, 1, 0, SLAVE_SELECT_QSPI_PIN);
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

	uint32_t last_event = time_us_32();

	static int batt_hist[16], batt_idx = 0, batt_avg = 0;
	static int temp_hist[16], temp_idx = 0, temp_avg = 0;
	static int cc1_hist[16], cc1_idx = 0, cc1_avg = 0;
	static int cc2_hist[16], cc2_idx = 0, cc2_avg = 0;

	while (true) {
		sdk_inputs.a = !remote_gpio_get(SLAVE_A_PIN);
		sdk_inputs.b = !remote_gpio_get(SLAVE_B_PIN);
		sdk_inputs.x = !remote_gpio_get(SLAVE_X_PIN);
		sdk_inputs.y = !remote_gpio_get(SLAVE_Y_PIN);

		sdk_inputs.vol_up = !remote_qspi_get(SLAVE_VOL_UP_QSPI_PIN);
		sdk_inputs.vol_down = !remote_qspi_get(SLAVE_VOL_DOWN_QSPI_PIN);
		sdk_inputs.vol_sw = !remote_qspi_get(SLAVE_VOL_SW_QSPI_PIN);

		sdk_inputs.aux[0] = !remote_gpio_get(SLAVE_AUX0_PIN);
		sdk_inputs.aux[1] = !remote_gpio_get(SLAVE_AUX1_PIN);
		sdk_inputs.aux[2] = !remote_gpio_get(SLAVE_AUX2_PIN);
		sdk_inputs.aux[3] = !remote_gpio_get(SLAVE_AUX3_PIN);
		sdk_inputs.aux[0] = !remote_gpio_get(SLAVE_AUX4_PIN);
		sdk_inputs.aux[1] = !remote_gpio_get(SLAVE_AUX5_PIN);
		sdk_inputs.aux[2] = !remote_gpio_get(SLAVE_AUX6_PIN);
		sdk_inputs.aux[3] = !remote_gpio_get(SLAVE_AUX7_PIN);

		sdk_inputs.start = !remote_gpio_get(SLAVE_START_PIN);
		sdk_inputs.select = !remote_qspi_get(SLAVE_SELECT_QSPI_PIN);

		sdk_inputs.joy_x = 2047 - remote_adc_read(SLAVE_JOY_X_PIN);
		sdk_inputs.joy_y = 2047 - remote_adc_read(SLAVE_JOY_Y_PIN);
		sdk_inputs.brack_l = 2047 - remote_adc_read(SLAVE_BRACK_L_PIN);
		sdk_inputs.brack_r = 2047 - remote_adc_read(SLAVE_BRACK_R_PIN);

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

		sdk_inputs_delta.joy_x = sdk_inputs.joy_x - prev_inputs.joy_x;
		sdk_inputs_delta.joy_y = sdk_inputs.joy_y - prev_inputs.joy_y;

		sdk_inputs_delta.vol_up = sdk_inputs.vol_up - prev_inputs.vol_up;
		sdk_inputs_delta.vol_down = sdk_inputs.vol_down - prev_inputs.vol_down;
		sdk_inputs_delta.vol_sw = sdk_inputs.vol_sw - prev_inputs.vol_sw;

		sdk_inputs_delta.brack_l = sdk_inputs.brack_l - prev_inputs.brack_l;
		sdk_inputs_delta.brack_r = sdk_inputs.brack_r - prev_inputs.brack_r;

		for (int i = 0; i < 8; i++)
			sdk_inputs_delta.aux[i] = sdk_inputs.aux[i] - prev_inputs.aux[i];

		sdk_inputs_delta.start = sdk_inputs.start - prev_inputs.start;
		sdk_inputs_delta.select = sdk_inputs.select - prev_inputs.select;

		prev_inputs = sdk_inputs;

		sdk_inputs_delta.batt_mv = sdk_inputs.batt_mv - prev_inputs.batt_mv;

		if (sdk_inputs.cc_mv < 200.0f) {
			if (sdk_config.off_on_select && sdk_inputs.select) {
				if (!select_held_since) {
					select_held_since = time_us_32();
				} else {
					uint32_t select_held_for = time_us_32() - select_held_since;
					if (select_held_for > 3000000) {
						puts("sdk: SELECT held for 3s, turning off...");
						remote_gpio_set(SLAVE_OFF_PIN, 0);
						select_held_since = 0;
					}
				}
			} else {
				select_held_since = 0;
			}
		}

		/* Let the game process inputs as soon as possible. */
		uint32_t now = time_us_32();
		game_input(now - last_event);
		last_event = now;

		task_sleep_ms(9);
	}
}
