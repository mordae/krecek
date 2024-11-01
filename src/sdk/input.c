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
#include <sdk/slave.h>

/* From video.c */
void sdk_video_start(void);

struct sdk_inputs sdk_inputs = {};
struct sdk_inputs sdk_inputs_delta = {};

static struct sdk_inputs prev_inputs = {};

inline static int slave_gpio_get(int pin)
{
	uint32_t tmp = sdk_peek(IO_BANK0_BASE + IO_BANK0_GPIO0_STATUS_OFFSET + 8 * pin);
	return (tmp >> IO_BANK0_GPIO0_STATUS_INFROMPAD_LSB) & 1;
}

inline static int slave_gpio_qspi_get(int pin)
{
	uint32_t tmp = sdk_peek(IO_QSPI_BASE + IO_QSPI_GPIO_QSPI_SCLK_STATUS_OFFSET + 8 * pin);
	return (tmp >> IO_QSPI_GPIO_QSPI_SCLK_STATUS_INFROMPAD_LSB) & 1;
}

inline static __unused int slave_adc_read(int gpio)
{
	sdk_poke(ADC_BASE + ADC_CS_OFFSET,
		 ((gpio - 26) << ADC_CS_AINSEL_LSB) | ADC_CS_EN_BITS | ADC_CS_START_MANY_BITS);

	uint32_t sample = sdk_peek(ADC_BASE + ADC_RESULT_OFFSET);

	return sample;
}

void sdk_input_init(void)
{
	/* Enable ADC. */
	sdk_poke(ADC_BASE + ADC_CS_OFFSET, ADC_CS_EN_BITS);

	/* A, B, X, Y */
	sdk_poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_A_PIN,
		 PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS |
			 PADS_BANK0_GPIO0_SCHMITT_BITS);
	sdk_poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_B_PIN,
		 PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS |
			 PADS_BANK0_GPIO0_SCHMITT_BITS);
	sdk_poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_X_PIN,
		 PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS |
			 PADS_BANK0_GPIO0_SCHMITT_BITS);
	sdk_poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_Y_PIN,
		 PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS |
			 PADS_BANK0_GPIO0_SCHMITT_BITS);

	/* VOL_UP, VOL_DOWN, VOL_SW */
	sdk_poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_VOL_UP_PIN,
		 PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS |
			 PADS_BANK0_GPIO0_SCHMITT_BITS);
	sdk_poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_VOL_DOWN_PIN,
		 PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS |
			 PADS_BANK0_GPIO0_SCHMITT_BITS);
	sdk_poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_VOL_SW_PIN,
		 PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS |
			 PADS_BANK0_GPIO0_SCHMITT_BITS);

	/* JOY_SW, SELECT */
	sdk_poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_JOY_SW_PIN,
		 PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS |
			 PADS_BANK0_GPIO0_SCHMITT_BITS);
	sdk_poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_SELECT_PIN,
		 PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS |
			 PADS_BANK0_GPIO0_SCHMITT_BITS);

	/* AUX[0-3] */
	sdk_poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_AUX0_PIN,
		 PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS |
			 PADS_BANK0_GPIO0_SCHMITT_BITS);
	sdk_poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_AUX1_PIN,
		 PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS |
			 PADS_BANK0_GPIO0_SCHMITT_BITS);
	sdk_poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_AUX2_PIN,
		 PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS |
			 PADS_BANK0_GPIO0_SCHMITT_BITS);
	sdk_poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_AUX3_PIN,
		 PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS |
			 PADS_BANK0_GPIO0_SCHMITT_BITS);

	/* AUX[4-7] */
	sdk_poke(PADS_QSPI_BASE + PADS_QSPI_GPIO_QSPI_SCLK_OFFSET + 4 * SLAVE_AUX4_QSPI_PIN,
		 PADS_QSPI_GPIO_QSPI_SCLK_IE_BITS | PADS_QSPI_GPIO_QSPI_SCLK_PUE_BITS |
			 PADS_QSPI_GPIO_QSPI_SCLK_SCHMITT_BITS);
	sdk_poke(PADS_QSPI_BASE + PADS_QSPI_GPIO_QSPI_SCLK_OFFSET + 4 * SLAVE_AUX5_QSPI_PIN,
		 PADS_QSPI_GPIO_QSPI_SCLK_IE_BITS | PADS_QSPI_GPIO_QSPI_SCLK_PUE_BITS |
			 PADS_QSPI_GPIO_QSPI_SCLK_SCHMITT_BITS);
	sdk_poke(PADS_QSPI_BASE + PADS_QSPI_GPIO_QSPI_SCLK_OFFSET + 4 * SLAVE_AUX6_QSPI_PIN,
		 PADS_QSPI_GPIO_QSPI_SCLK_IE_BITS | PADS_QSPI_GPIO_QSPI_SCLK_PUE_BITS |
			 PADS_QSPI_GPIO_QSPI_SCLK_SCHMITT_BITS);
	sdk_poke(PADS_QSPI_BASE + PADS_QSPI_GPIO_QSPI_SCLK_OFFSET + 4 * SLAVE_AUX7_QSPI_PIN,
		 PADS_QSPI_GPIO_QSPI_SCLK_IE_BITS | PADS_QSPI_GPIO_QSPI_SCLK_PUE_BITS |
			 PADS_QSPI_GPIO_QSPI_SCLK_SCHMITT_BITS);

	/* BRACK_R, BRACK_L, JOY_X, JOY_Y */
	sdk_poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_BRACK_R_PIN,
		 PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS);
	sdk_poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_BRACK_L_PIN,
		 PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS);
	sdk_poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_JOY_X_PIN,
		 PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS);
	sdk_poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_JOY_Y_PIN,
		 PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS);
}

void sdk_input_task(void)
{
	game_reset();
	sdk_video_start();

	uint32_t last_event = time_us_32();

	while (true) {
		sdk_inputs.a = !slave_gpio_get(SLAVE_A_PIN);
		sdk_inputs.b = !slave_gpio_get(SLAVE_B_PIN);
		sdk_inputs.x = !slave_gpio_get(SLAVE_X_PIN);
		sdk_inputs.y = !slave_gpio_get(SLAVE_Y_PIN);

		sdk_inputs.vol_up = !slave_gpio_get(SLAVE_VOL_UP_PIN);
		sdk_inputs.vol_down = !slave_gpio_get(SLAVE_VOL_DOWN_PIN);
		sdk_inputs.vol_sw = !slave_gpio_get(SLAVE_VOL_SW_PIN);

		sdk_inputs.joy_sw = !slave_gpio_get(SLAVE_JOY_SW_PIN);

		sdk_inputs.aux[0] = !slave_gpio_get(SLAVE_AUX0_PIN);
		sdk_inputs.aux[1] = !slave_gpio_get(SLAVE_AUX1_PIN);
		sdk_inputs.aux[2] = !slave_gpio_get(SLAVE_AUX2_PIN);
		sdk_inputs.aux[3] = !slave_gpio_get(SLAVE_AUX3_PIN);

		sdk_inputs.aux[4] = !slave_gpio_qspi_get(SLAVE_AUX4_QSPI_PIN);
		sdk_inputs.aux[5] = !slave_gpio_qspi_get(SLAVE_AUX5_QSPI_PIN);
		sdk_inputs.aux[6] = !slave_gpio_qspi_get(SLAVE_AUX6_QSPI_PIN);
		sdk_inputs.aux[7] = !slave_gpio_qspi_get(SLAVE_AUX7_QSPI_PIN);

		sdk_inputs.start = slave_gpio_get(SLAVE_START_PIN);
		sdk_inputs.select = !slave_gpio_get(SLAVE_SELECT_PIN);

		sdk_inputs.joy_x = 2048 - slave_adc_read(SLAVE_JOY_X_PIN);
		sdk_inputs.joy_y = 2048 - slave_adc_read(SLAVE_JOY_Y_PIN);
		sdk_inputs.brack_l = 2048 - slave_adc_read(SLAVE_BRACK_L_PIN);
		sdk_inputs.brack_r = 2048 - slave_adc_read(SLAVE_BRACK_R_PIN);

		adc_select_input(2);
		int bat = 0;

		for (int i = 0; i < 32; i++)
			bat += adc_read();

		float mv = (float)bat * 1.6f / 32.0f;

		if (!sdk_inputs.batt_mv)
			sdk_inputs.batt_mv = mv;
		else
			sdk_inputs.batt_mv = sdk_inputs.batt_mv * 0.9 + 0.1 * mv;

		/* Calculate input deltas */
		sdk_inputs_delta.a = sdk_inputs.a - prev_inputs.a;
		sdk_inputs_delta.b = sdk_inputs.b - prev_inputs.b;
		sdk_inputs_delta.x = sdk_inputs.x - prev_inputs.x;
		sdk_inputs_delta.y = sdk_inputs.y - prev_inputs.y;

		sdk_inputs_delta.joy_x = sdk_inputs.joy_x - prev_inputs.joy_x;
		sdk_inputs_delta.joy_y = sdk_inputs.joy_y - prev_inputs.joy_y;
		sdk_inputs_delta.joy_sw = sdk_inputs.joy_sw - prev_inputs.joy_sw;

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

		prev_inputs = sdk_inputs;

		if (sdk_config.off_on_select) {
			if (sdk_inputs_delta.select > 0) {
				puts("sdk: SELECT pressed, turning off...");
				sdk_poke(IO_QSPI_BASE + IO_QSPI_GPIO_QSPI_SCLK_CTRL_OFFSET +
						 8 * SLAVE_OFF_QSPI_PIN,
					 (IO_QSPI_GPIO_QSPI_SCLK_CTRL_OEOVER_BITS |
					  IO_QSPI_GPIO_QSPI_SCLK_CTRL_OUTOVER_BITS |
					  IO_QSPI_GPIO_QSPI_SCLK_CTRL_FUNCSEL_BITS));
			}
		}

		/* Let the game process inputs as soon as possible. */
		uint32_t now = time_us_32();
		game_input(now - last_event);
		last_event = now;

		task_sleep_ms(9);
	}
}
