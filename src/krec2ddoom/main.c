#include <pico/stdlib.h>
#include <sdk.h>
#include <tft.h>
#include <arrow.png.h>

#define BLUE rgb_to_rgb565(0, 0, 255)
#define WHITE rgb_to_rgb565(255, 255, 255)

typedef enum { EDITOR_MAP, EDITOR_TILE, EDITOR_MENU } EDITOR_STATE;

typedef struct {
	sdk_sprite_t s;
} Arrow;
static Arrow arrow;

static EDITOR_STATE editor_state = EDITOR_MENU;

void game_start(void)
{
	arrow.s.x = 35;
	arrow.s.y = 42;

	editor_state = EDITOR_MENU;
}

void game_reset(void)
{
	game_start();
}

void game_input(unsigned dt_usec)
{
	(void)dt_usec;

	int mx = sdk_inputs.tx * TFT_WIDTH;
	int my = sdk_inputs.ty * TFT_HEIGHT;
	arrow.s.ts = &ts_arrow_png;

	switch (editor_state) {
	case EDITOR_MENU:
		if (my >= 22 && my <= 54) {
			arrow.s.x = 35;
			arrow.s.y = 42;
			if (sdk_inputs.tp > 0.5) {
			}
		}
		if (my >= 55 && my <= 69) {
			arrow.s.x = 35;
			arrow.s.y = 57;
			if (sdk_inputs.tp > 0.5) {
			}
		}
		if (my >= 70 && my <= 84) {
			arrow.s.x = 35;
			arrow.s.y = 72;
			if (sdk_inputs.tp > 0.5) {
			}
		}
		if (my >= 84 && my <= 100) {
			arrow.s.x = 35;
			arrow.s.y = 87;
			if (sdk_inputs.tp > 0.5) {
			}
		}
		break;

	case EDITOR_MAP:
	case EDITOR_TILE:
		break;
	}
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;
	tft_fill(0);
	switch (editor_state) {
	case EDITOR_MENU:
		tft_draw_string(40, 10, WHITE, "Krecdoom");
		tft_draw_string(60, 22, WHITE, "editor");
		tft_draw_string(40, 40, WHITE, "Map select");
		tft_draw_string(40, 55, WHITE, "Tile editor");
		tft_draw_string(40, 70, WHITE, "New file");
		tft_draw_string(40, 85, WHITE, "Del file");
		sdk_draw_sprite(&arrow.s);
		break;

	case EDITOR_MAP:
	case EDITOR_TILE:
		break;
	}
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = false,
		.fps_color = WHITE,
	};
	sdk_main(&config);
}
