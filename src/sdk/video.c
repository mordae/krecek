#include <pico/stdlib.h>
#include <pico/sync.h>

#include <hardware/pwm.h>
#include <hardware/dma.h>

#include <hardware/regs/pwm.h>

#include <sdk.h>
#include <sdk/slave.h>

#include <tft.h>
#include <task.h>

#include <stdio.h>
#include <math.h>

static semaphore_t paint_sema;
static semaphore_t sync_sema;

#define PWM_CHx_TOP_REG(x) \
	(PWM_BASE + PWM_CH0_TOP_OFFSET + (x) * (PWM_CH1_TOP_OFFSET - PWM_CH0_TOP_OFFSET))

#define PWM_EN_REG (PWM_BASE + PWM_EN_OFFSET)

#define IO_BANK0_GPIOx_CTRL_REG(x)                    \
	(IO_BANK0_BASE + IO_BANK0_GPIO0_CTRL_OFFSET + \
	 (x) * (IO_BANK0_GPIO1_CTRL_OFFSET - IO_BANK0_GPIO0_CTRL_OFFSET))

void sdk_video_init(void)
{
	/* Initialize the TFT panel. */
	tft_init();

	/* Prepare semaphores to guard buffer access. */
	sem_init(&paint_sema, 0, 1);
	sem_init(&sync_sema, 0, 1);

	/* Enable backlight PWM control. */
	int slice = pwm_gpio_to_slice_num(SLAVE_TFT_LED_PIN);
	sdk_poke(IO_BANK0_GPIOx_CTRL_REG(SLAVE_TFT_LED_PIN),
		 IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PWM_A_0 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);
	sdk_poke(PWM_CHx_TOP_REG(slice), 256);
	sdk_set_backlight(SDK_BACKLIGHT_STD);
	sdk_poke(PWM_EN_REG, 1u << slice);
}

void sdk_set_backlight(unsigned level)
{
	level = clamp(level, 0, 256);
	sdk_config.backlight = level;

	int slice = pwm_gpio_to_slice_num(SLAVE_TFT_LED_PIN);
	int chan = pwm_gpio_to_channel(SLAVE_TFT_LED_PIN);

	sdk_poke(PWM_BASE + (PWM_CH0_CC_OFFSET + slice * (PWM_CH1_CC_OFFSET - PWM_CH0_CC_OFFSET)),
		 (uint32_t)level << (chan * PWM_CH0_CC_B_LSB));
}

void sdk_paint_task(void)
{
	/* Wait for game_reset() from the input task. */
	sem_acquire_blocking(&paint_sema);
	sem_release(&paint_sema);

	uint32_t last_sync = time_us_32();
	int delta = 1000 * 1000 / 30;

	uint32_t last_updated = 0;
	float active_fps = 30;
	float fps = active_fps;

	while (true) {
		sem_acquire_blocking(&paint_sema);

		game_paint(delta ? delta : 1);

		if (sdk_config.show_fps) {
			char buf[16];
			snprintf(buf, sizeof buf, "%.0f", floorf(active_fps));
			tft_draw_string_right(TFT_WIDTH - 1, 0, sdk_config.fps_color, buf);
		}

		sem_release(&sync_sema);

		if (sdk_config.show_fps) {
			uint32_t this_sync = time_us_32();
			delta = this_sync - last_sync;
			fps = 0.95 * fps + 0.05 * (1000000.0f / delta);
			last_sync = this_sync;

			if ((this_sync - last_updated) >= 1000000) {
				last_updated = this_sync;
				active_fps = fps;
			}
		}

		task_yield();
	}
}

void sdk_video_start(void)
{
	sem_release(&paint_sema);
}

void sdk_tft_task(void)
{
	while (true) {
		sem_acquire_blocking(&sync_sema);
		tft_swap_buffers();
		sem_release(&paint_sema);

		tft_sync();
		task_yield();
	}
}

void tft_dma_channel_wait_for_finish_blocking(int dma_ch)
{
	while (dma_channel_is_busy(dma_ch))
		task_wait_for_dma(dma_ch);
}
