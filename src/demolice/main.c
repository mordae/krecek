#include <math.h>
#include <pico/stdlib.h>

#include <sdk.h>
#include <tft.h>

#define PADDLE_HEIGHT 4
#define PADDLE_WIDTH 30

#define RED 240
#define YELLOW 242
#define GREEN 244
#define BLUE 250
#define GRAY 8
#define WHITE 15

struct vector_2 {
	float x;
	float y;
};

struct paddle {
	float position;
	float speed;
};
static struct paddle paddle;

struct ball {
	struct vector_2 position;
	struct vector_2 direction;
	float speed;
};
static struct ball ball;

float get_vector_magnitude(struct vector_2 v) {
	return sqrtf(v.x * v.x + v.y * v.y);
}

struct vector_2 normalise(struct vector_2 v) {
	float m = get_vector_magnitude(v);
	if (!m) {
		return v;
	}
	struct vector_2 ov;
	ov.x = v.x / m;
	ov.y = v.y / m;
	return ov;
}

bool is_position_full(struct vector_2 p) {
	if (p.x < 0 || p.x >= TFT_WIDTH) {
		return true;
	}
	if (p.y < 0 || p.y >= TFT_HEIGHT) {
		return true;
	}
	return false;
}

void handle_ball_collision() {
	struct vector_2 pu = {ball.position.x, ball.position.y - 1};
	struct vector_2 pd = {ball.position.x, ball.position.y + 1};
	struct vector_2 pl = {ball.position.x - 1, ball.position.y};
	struct vector_2 pr = {ball.position.x + 1, ball.position.y};
	if (is_position_full(pu) && ball.direction.y < 0) {
		ball.direction.y *= -1;
	}
	if (is_position_full(pd) && ball.direction.y > 0) {
		ball.direction.y *= -1;
	}
	if (is_position_full(pl) && ball.direction.x < 0) {
		ball.direction.x *= -1;
	}
	if (is_position_full(pr) && ball.direction.x > 0) {
		ball.direction.x *= -1;
	}
}

void game_reset(void)
{
	paddle.position = TFT_WIDTH / 2;
	paddle.speed = 100;

	ball.position.x = 40; ball.position.y = 10;
	ball.direction.x = 0.2; ball.direction.y = 0.4;
	ball.speed = 30;
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	/* Joys have value from -2048 to +2047. */
	if (sdk_inputs.joy_x > 500)
		paddle.position += paddle.speed * dt;
	else if (sdk_inputs.joy_x < -500)
		paddle.position -= paddle.speed * dt;

	if ((paddle.position - PADDLE_WIDTH / 2) < 0) {
		paddle.position = 0 + PADDLE_WIDTH / 2;
	}
	if ((paddle.position + PADDLE_WIDTH / 2) > TFT_RIGHT) {
		paddle.position = TFT_RIGHT - PADDLE_WIDTH / 2;
	}

	ball.position.x = ball.direction.x * ball.speed * dt;
	ball.position.y = ball.direction.y * ball.speed * dt;
	handle_ball_collision();
	ball.direction = normalise(ball.direction);

}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	tft_draw_rect(paddle.position - PADDLE_WIDTH / 2,
		      TFT_BOTTOM - PADDLE_HEIGHT,
		      paddle.position + PADDLE_WIDTH / 2,
		      TFT_BOTTOM,
		      WHITE);

	tft_draw_rect(ball.position.x - 1,
		      ball.position.y - 1,
		      ball.position.x + 1,
		      ball.position.y + 1,
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
