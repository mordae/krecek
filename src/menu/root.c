#include <sdk.h>

extern sdk_scene_t scene_games;
extern sdk_scene_t scene_screens;

#define NUM_SCENES 2
static sdk_scene_t *scenes[NUM_SCENES] = { &scene_games, &scene_screens };
static int current_scene = 0;

static int bayer4x4[4][4] = {
	{ 0, 8, 2, 10 },
	{ 12, 4, 14, 6 },
	{ 3, 11, 1, 9 },
	{ 15, 7, 13, 5 },
};

static void root_paint(float dt, int depth)
{
	(void)dt;
	(void)depth;

	for (int y = 0; y < TFT_HEIGHT; y++) {
		int base_gray = TFT_HEIGHT + 64 - y;

		for (int x = 0; x < TFT_WIDTH; x++) {
			int gray = base_gray - bayer4x4[y & 3][x & 3];
			uint16_t color = rgb_to_rgb565(gray, gray >> 1, gray >> 2);
			tft_draw_pixel(x, y, color);
		}
	}
}

static bool root_handle(sdk_event_t event)
{
	switch (event) {
	case SDK_PRESSED_Y:
		current_scene = (current_scene + 1) % NUM_SCENES;
		sdk_scene_swap(scenes[current_scene]);
		return true;

	default:
		return false;
	}
}

static void root_pushed(void)
{
	sdk_scene_push(scenes[current_scene]);
}

sdk_scene_t scene_root = {
	.paint = root_paint,
	.handle = root_handle,
	.pushed = root_pushed,
};
