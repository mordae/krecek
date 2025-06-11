#include <pico/stdlib.h>

#include <sdk.h>
#include <tft.h>

#include <balls.png.h>

#define GRAY rgb_to_rgb565(63, 63, 63)
#include <cover.png.h>
sdk_game_info("kal", &image_cover_png);

struct circle {
	int selected;
	int x;
	int y;
	int xmov;
	int ymov;
};
static struct circle circle;

void game_reset(void)
{
	circle.x = 60;
	circle.y = 50;
	circle.y = -10;
}

void game_input(unsigned dt_usec)
{
	(void)dt_usec;
}

void game_paint(unsigned __unused dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	if (circle.ymov >= -10 && circle.ymov <= 10) {
		circle.ymov += dt;
	}

	sdk_draw_tile(circle.x + 5, circle.y + circle.ymov, &ts_balls_png, 0);
	sdk_draw_tile(circle.x, circle.y, &ts_balls_png, 0);
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = true,
		.fps_color = GRAY,
	};

	sdk_main(&config);
}
