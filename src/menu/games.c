#include <sdk.h>

#include <string.h>
#include <text.png.h>
#include <select.png.h>

#define NUM_GAMES 32
const sdk_game_info_t *games[NUM_GAMES];
static int selected = 0;

static void games_paint(float dt, int depth)
{
	(void)dt;
	(void)depth;

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

static bool games_handle(sdk_event_t event, int depth)
{
	(void)depth;

	switch (event) {
	case SDK_PRESSED_B:
	case SDK_TICK_EAST:
		selected = (selected + 1) % NUM_GAMES;
		return true;

	case SDK_PRESSED_X:
	case SDK_TICK_WEST:
		selected = (selected + NUM_GAMES - 1) % NUM_GAMES;
		return true;

	case SDK_PRESSED_A:
		if (games[selected])
			sdk_reboot_into_slot(selected);
		return true;

	default:
		return false;
	}
}

static void games_pushed(void)
{
	for (int i = 0; i < NUM_GAMES; i++) {
		unsigned base = XIP_BASE + (i * 512 * 1024) + 256;

		for (int j = 0; j < 1024; j++) {
			if (strncmp("KRECEK1", (const char *)(base + j), 8))
				continue;

			games[i] = (const void *)(base + j);
			break;
		}
	}
}

sdk_scene_t scene_games = {
	.paint = games_paint,
	.handle = games_handle,
	.pushed = games_pushed,
};
