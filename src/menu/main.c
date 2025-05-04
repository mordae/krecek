#include <pico/stdlib.h>

#include <sdk.h>
#include <string.h>
#include <tft.h>

#include <text.png.h>
#include <select.png.h>
#include <cover.png.h>

sdk_game_info("menu", &image_cover_png);

#define GRAY rgb_to_rgb565(63, 63, 63)

#define NUM_GAMES 16
const sdk_game_info_t *games[NUM_GAMES];

int selected = 0;

void game_reset(void)
{
	for (int i = 0; i < NUM_GAMES; i++) {
		unsigned base = XIP_BASE + (i * 1024 * 1024) + 256;

		for (int j = 0; j < 1024; j++) {
			if (strncmp("KRECEK0", (const char *)(base + j), 8))
				continue;

			games[i] = (const void *)(base + j);
			break;
		}
	}
}

void game_input(unsigned dt_usec)
{
	(void)dt_usec;

	if (sdk_inputs_delta.b > 0 || sdk_inputs_delta.horizontal > 0) {
		selected = (selected + 1) % NUM_GAMES;
	}

	if (sdk_inputs_delta.x > 0 || sdk_inputs_delta.horizontal < 0) {
		selected = (NUM_GAMES + selected - 1) % NUM_GAMES;
	}

	if (sdk_inputs_delta.start > 0 && games[selected]) {
		sdk_reboot_into_slot(selected);
	}
}

static int bayer4x4[4][4] = {
	{ 0, 8, 2, 10 },
	{ 12, 4, 14, 6 },
	{ 3, 11, 1, 9 },
	{ 15, 7, 13, 5 },
};

void game_paint(unsigned __unused dt_usec)
{
	for (int y = 0; y < TFT_HEIGHT; y++) {
		int base_gray = TFT_HEIGHT + 64 - y;

		for (int x = 0; x < TFT_WIDTH; x++) {
			int gray = base_gray - bayer4x4[y & 3][x & 3];
			uint16_t color = rgb_to_rgb565(gray, gray >> 1, gray >> 2);
			tft_draw_pixel(x, y, color);
		}
	}

	if (selected) {
		tft_draw_sprite(1, 0, image_text_png.width, image_text_png.height,
				image_text_png.data, TRANSPARENT);
	}

	int prev = (NUM_GAMES + selected - 1) % NUM_GAMES;
	int next = (selected + 1) % NUM_GAMES;

	if (games[selected] && games[selected]->cover_image)
		tft_draw_sprite(35, 15, games[selected]->cover_image->width,
				games[selected]->cover_image->height,
				games[selected]->cover_image->data, TRANSPARENT);

	if (games[prev] && games[prev]->cover_image)
		tft_draw_sprite(-65, 20, games[prev]->cover_image->width,
				games[prev]->cover_image->height, games[prev]->cover_image->data,
				TRANSPARENT);

	if (games[next] && games[next]->cover_image)
		tft_draw_sprite(135, 20, games[next]->cover_image->width,
				games[next]->cover_image->height, games[next]->cover_image->data,
				TRANSPARENT);

	int dots_left = (TFT_WIDTH / 2) - (NUM_GAMES * 4 / 2);

	for (int i = 0; i < NUM_GAMES; i++)
		sdk_draw_tile(dots_left + i * 4, 120 - 5, &ts_select_png, selected == i);
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = false,
		.off_on_select = true,
		.fps_color = GRAY,
	};

	sdk_main(&config);
}
