#include <sdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <inputs.png.h>

#define MAX_FILES 128
static char file_names[MAX_FILES][13];
static int nfiles = 0;
static int selected = 0;

static bool show_overlay = true;
static int deleting = -1;

static FIL file;

static color_t cached[TFT_WIDTH][TFT_HEIGHT];
static int cached_image = -1;

static __always_inline color_t blend(color_t a, color_t b)
{
	// Fast, non-linear 50:50 color blending.
	// https://www.reddit.com/r/algorithms/comments/ami4r9/fast_color_operations/
	return ((a ^ b) >> 1 & 0b0111101111101111) + (a & b);
}

static void screens_paint(float dt, int depth)
{
	(void)dt;
	(void)depth;

	if (0 == nfiles) {
		tft_draw_string_center(TFT_WIDTH / 2, TFT_HEIGHT / 2 - 8,
				       rgb_to_rgb565(255, 255, 255), "No files.");
		return;
	}

	int err;
	char path[32];
	sprintf(path, "/screens/%s", file_names[selected]);

	if (cached_image != selected) {
		if ((err = f_open(&file, path, FA_OPEN_ALWAYS | FA_READ)))
			goto fail;

		if ((err = sdk_bmp_read_frame(&file, cached)))
			goto fail;

		if ((err = f_close(&file)))
			goto fail;

		if (err) {
fail:
			tft_draw_string_center(TFT_WIDTH / 2, TFT_HEIGHT / 2 - 8,
					       rgb_to_rgb565(255, 255, 255), "Error %i", err);
			tft_draw_string_center(TFT_WIDTH / 2, TFT_HEIGHT / 2 + 8,
					       rgb_to_rgb565(255, 255, 255), "%s", f_strerror(err));
			cached_image = -1;
		}

		cached_image = selected;
		memcpy(tft_input, cached, sizeof(cached));
	} else {
		memcpy(tft_input, cached, sizeof(cached));
	}

	if (show_overlay) {
		// 50% brightness when overlay is active.
		for (int x = 0; x < TFT_WIDTH; x++)
			for (int y = 0; y < TFT_HEIGHT; y++)
				tft_input[x][y] = blend(tft_input[x][y], 0);
	}

	if (deleting >= 0) {
		// Fun little effect to indicate deletion.
		for (int x = 0; x < TFT_WIDTH; x++) {
			for (int y = 0; y < TFT_HEIGHT; y++) {
				uint32_t r = rand();
				r = (r >> 16) ^ (r & 0xffff);

				if (r <= (uint32_t)deleting)
					tft_input[x][y] = 0;
			}
		}

		deleting += dt * (1 << 16);
	}

	if (show_overlay) {
		tft_draw_string(4, 4, rgb_to_rgb565(255, 255, 255), "%s", file_names[selected]);

		sdk_draw_tile(TFT_WIDTH - 18, TFT_HEIGHT - 18, &ts_inputs_png, 0);
		tft_draw_string_right(TFT_WIDTH - 24, TFT_HEIGHT - 17, rgb_to_rgb565(255, 255, 255),
				      "View");

		sdk_draw_tile(2, TFT_HEIGHT - 18, &ts_inputs_png, 10);
		tft_draw_string(24, TFT_HEIGHT - 17, rgb_to_rgb565(255, 255, 255), "Delete");
	}

	if (deleting >= (1 << 16)) {
		if ((err = f_unlink(path)))
			printf("sdk: f_unlink %s failed: %s\n", path, f_strerror(err));

		deleting = -1;
		cached_image = -1;

		extern sdk_scene_t scene_screens;
		sdk_scene_swap(&scene_screens);
	}
}

static bool screens_handle(sdk_event_t event)
{
	switch (event) {
	case SDK_PRESSED_B:
	case SDK_TICK_EAST:
		if (++selected >= nfiles)
			selected = 0;
		return true;

	case SDK_PRESSED_X:
	case SDK_TICK_WEST:
		if (--selected < 0)
			selected = nfiles - 1;
		return true;

	case SDK_PRESSED_A:
		show_overlay = !show_overlay;
		return true;

	case SDK_PRESSED_START:
		deleting = 0;
		return true;

	case SDK_RELEASED_START:
		deleting = -1;
		return true;

	default:
		return false;
	}
}

static void screens_pushed(void)
{
	static DIR dir;

	if (FR_OK != f_opendir(&dir, "/screens"))
		return;

	for (int i = 0; i < MAX_FILES; i++) {
		FILINFO info;

		if (FR_OK != f_readdir(&dir, &info))
			goto fail;

		if (!info.fname[0])
			break;

		strlcpy(file_names[nfiles++], info.fname, 13);
	}

	selected = clamp(selected, 0, nfiles - 1);

fail:
	f_closedir(&dir);
}

static void screens_popped(void)
{
	nfiles = 0;
}

sdk_scene_t scene_screens = {
	.paint = screens_paint,
	.handle = screens_handle,
	.pushed = screens_pushed,
	.popped = screens_popped,
};
