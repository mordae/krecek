#include <pico/stdlib.h>
#include <pico/sync.h>

#include <hardware/dma.h>

#include <sdk.h>
#include <sdk/remote.h>

#include <stdio.h>
#include <tft.h>
#include <task.h>

#include <math.h>

#include <assets/splash.png.h>

static semaphore_t paint_sema;
static semaphore_t sync_sema;

static FIL file; /* For screenshots. */

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

		if (sdk_requested_screenshot) {
			sdk_requested_screenshot = false;

			int res = f_mkdir("/screens");

			if (FR_EXIST != res && FR_OK != res) {
				printf("f_mkdir /scrshots failed\n");
				goto fs_error;
			}

			char name[32];

			for (int i = 0; i <= 99; i++) {
				sprintf(name, "/screens/screen%02i.bmp", i);
				int res = f_open(&file, name, FA_CREATE_NEW | FA_WRITE);

				if (FR_EXIST == res)
					continue;

				if (FR_OK != res) {
					printf("f_open %s failed\n", name);
					goto fs_error;
				}

				goto fs_write;
			}

			printf("too many screenshots\n");
			goto fs_error;

fs_write:
			sdk_bmp_write_header(&file, TFT_WIDTH, TFT_HEIGHT);
			sdk_bmp_write_frame(&file, tft_input);
			f_close(&file);

			printf("wrote %s\n", name);
		}

fs_error:

		if (sdk_config.show_fps) {
			tft_draw_string_right(TFT_WIDTH - 1, 0, sdk_config.fps_color, "%.0f",
					      floorf(active_fps));
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
	bool on = false;

	while (true) {
		sem_acquire_blocking(&sync_sema);
		tft_swap_buffers();
		sem_release(&paint_sema);

		tft_sync();

		if (!on) {
			/* Light up the screen. */
			tft_display_on();
			on = true;
		}

		task_yield();
	}
}

void tft_dma_channel_wait_for_finish_blocking(int dma_ch)
{
	while (dma_channel_is_busy(dma_ch)) {
		task_wait_for_dma(dma_ch);
	}
}
