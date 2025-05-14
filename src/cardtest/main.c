#include <sdk.h>

void game_start(void)
{
}

void game_input(unsigned dt_usec)
{
	(void)dt_usec;

	if (sdk_inputs_delta.a > 0) {
		f_mkdir("/testdir");
	}

	if (sdk_inputs_delta.b > 0) {
		f_unlink("/testdir");
	}

	if (sdk_inputs_delta.x > 0) {
		FIL file;
		UINT bw;
		if (FR_OK == f_open(&file, "/hello.txt", FA_WRITE | FA_CREATE_ALWAYS)) {
			f_write(&file, "Hello!", 6, &bw);
			f_close(&file);
		}
	}

	if (sdk_inputs_delta.y > 0) {
		f_unlink("/hello.txt");
	}

	if (sdk_inputs_delta.start > 0) {
		sdk_reboot_into_slot(15);
	}
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;

	tft_fill(0);

	tft_draw_rect(0, 0, TFT_WIDTH - 1, 18, rgb_to_rgb565(63, 63, 63));

	static const char *status_name[] = { "ready", "idle", "missing" };

	static uint16_t status_color[] = {
		rgb_to_rgb565(127, 255, 127),
		rgb_to_rgb565(255, 255, 127),
		rgb_to_rgb565(255, 127, 127),
	};

	if (SDCARD_READY == sdk_sdcard_status) {
		tft_draw_string(2, 2, rgb_to_rgb565(127, 255, 127), "%.3f GiB",
				(double)sdk_sdcard_csd.size / (1024 * 1024 * 1024));
	}

	tft_draw_string_right(TFT_WIDTH - 1, 2, status_color[sdk_sdcard_status], "%s",
			      status_name[sdk_sdcard_status]);

	static DIR dir;
	if (FR_OK == f_opendir(&dir, "/")) {
		FILINFO file;
		int y = 24;

		while (FR_OK == f_readdir(&dir, &file)) {
			if (!file.fname[0])
				break;

			tft_draw_string(0, y, rgb_to_rgb565(255, 255, 255), "%-12s%s %u",
					file.fname, (file.fattrib & AM_DIR) ? "/" : " ",
					(unsigned)file.fsize);
			y += 16;
		}

		f_closedir(&dir);
	}
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = false,
		.off_on_select = true,
		.fps_color = rgb_to_rgb565(31, 31, 31),
	};

	sdk_main(&config);
}
