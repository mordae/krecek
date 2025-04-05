#include <pico/stdlib.h>
#include <pico/sync.h>

#include <hardware/dma.h>

#include <sdk.h>
#include <sdk/remote.h>

#include <tft.h>
#include <task.h>

#include <stdio.h>
#include <math.h>

static semaphore_t paint_sema;
static semaphore_t sync_sema;

void sdk_video_init(void)
{
	/* Initialize the TFT panel. */
	tft_init();

	/* Prepare semaphores to guard buffer access. */
	sem_init(&paint_sema, 0, 1);
	sem_init(&sync_sema, 0, 1);
}

void sdk_set_backlight(unsigned level)
{
	level = clamp(level, 0, 256);
	sdk_config.backlight = level;

	// TODO: Backlight is now controlled via display registers.
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
