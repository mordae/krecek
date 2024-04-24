#include <pico/stdlib.h>

#include <stdlib.h>

#include <sdk.h>
#include <tft.h>

#define PADDLE_HEIGHT 4
#define PADDLE_WIDTH (int)(TFT_WIDTH / 4)

#define RED 240
#define YELLOW 242
#define GREEN 244
#define BLUE 250
#define GRAY 8
#define WHITE 15

struct paddle {
	float x;
	int score;
};

static struct paddle paddle;

void game_reset(void)
{
	paddle.x = TFT_WIDTH / 2;
	paddle.score = 0;
}

void game_input(unsigned __unused dt_usec)
{
	//float dt = dt_usec / 1000000.0f;

	/* Joys have value from -2048 to +2047. */
	if (sdk_inputs.joy_x > 500)
		paddle.x += 1;
	else if (sdk_inputs.joy_x < -500)
		paddle.x -= 1;

	if ((paddle.x - PADDLE_WIDTH / 2) < 0) {
		paddle.x = 0 + PADDLE_WIDTH / 2;
	}

	if ((paddle.x + PADDLE_WIDTH / 2) > TFT_RIGHT) {
		paddle.x = TFT_RIGHT - PADDLE_WIDTH / 2;
	}
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	tft_draw_rect(paddle.x - PADDLE_WIDTH / 2,
		      TFT_BOTTOM - PADDLE_HEIGHT,
		      paddle.x + PADDLE_WIDTH / 2,
		      TFT_BOTTOM,
		      WHITE);
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
