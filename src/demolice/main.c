#include "sdk/game.h"
#include "sdk/input.h"
#include <math.h>
#include <pico/stdlib.h>

#include <sdk.h>
#include <stdlib.h>
#include <tft.h>

#define PADDLE_HEIGHT 4
#define PADDLE_WIDTH 30.0
#define BLOCK_COUNT_X 12
#define BLOCK_COUNT_Y 12
#define BLOCK_HEIGHT 5
#define BLOCK_WIDTH 10

#define BALL_MOVEMENT_SUBSTEPS 4

#define RED rgb_to_rgb565(255, 0, 0)
#define YELLOW rgb_to_rgb565(255, 255, 0)
#define GREEN rgb_to_rgb565(0, 255, 0)
#define BLUE rgb_to_rgb565(0, 0, 255)
#define GRAY rgb_to_rgb565(127, 127, 127)
#define WHITE rgb_to_rgb565(255, 255, 255)

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

static int blocks[BLOCK_COUNT_Y][BLOCK_COUNT_X];
static int block_colors[4] = { RED, YELLOW, GREEN, BLUE };

static struct vector_2 block_position = { .x = (float)(TFT_WIDTH - BLOCK_WIDTH * BLOCK_COUNT_X) / 2,
					  .y = (float)(TFT_WIDTH - BLOCK_WIDTH * BLOCK_COUNT_X) /
					       2 };

float get_vector_magnitude(struct vector_2 v)
{
	return sqrtf(v.x * v.x + v.y * v.y);
}

struct vector_2 normalise(struct vector_2 v)
{
	float m = get_vector_magnitude(v);
	if (!m) {
		return v;
	}
	struct vector_2 ov;
	ov.x = v.x / m;
	ov.y = v.y / m;
	return ov;
}

bool is_position_full(struct vector_2 p)
{
	if (p.x < 0 || p.x >= TFT_WIDTH) {
		return true;
	}
	if (p.y < 0) {
		return true;
	}
	if (p.y >= TFT_HEIGHT) {
		game_reset();
	}
	if (abs((int)(p.x - paddle.position)) < PADDLE_WIDTH / 2 &&
	    p.y > TFT_BOTTOM - PADDLE_HEIGHT) {
		ball.direction.y = -1 + abs((int)(p.x - paddle.position)) / PADDLE_WIDTH;
		ball.direction.x = (p.x - paddle.position) / PADDLE_WIDTH;
		return false;
	}
	if (p.x - block_position.x < 0 || p.y - block_position.y < 0) {
		return false;
	}
	p.x = (int)((p.x - block_position.x) / BLOCK_WIDTH);
	p.y = (int)((p.y - block_position.y) / BLOCK_HEIGHT);
	if (p.x > BLOCK_COUNT_X - 1 || p.y > BLOCK_COUNT_Y - 1) {
		return false;
	}
	if (blocks[(int)p.y][(int)p.x]) {
		blocks[(int)p.y][(int)p.x] = 0;
		ball.speed += 1;
		return true;
	}
	return false;
}

void handle_ball_collision()
{
	struct vector_2 pu = { ball.position.x, ball.position.y - 1 };
	struct vector_2 pd = { ball.position.x, ball.position.y + 1 };
	struct vector_2 pl = { ball.position.x - 1, ball.position.y };
	struct vector_2 pr = { ball.position.x + 1, ball.position.y };
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
	paddle.position = (float)TFT_WIDTH / 2;
	paddle.speed = 100;

	ball = (struct ball){ .position = (struct vector_2){ 5, 5 },
			      .direction = (struct vector_2){ 0.2, 0.4 },
			      .speed = 60 };

	for (int y = 0; y < BLOCK_COUNT_Y; y++) {
		for (int x = 0; x < BLOCK_COUNT_X; x++) {
			blocks[y][x] = block_colors[rand() % 4];
		}
	}
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	/* Joys have value from -2048 to +2047. */
	if (sdk_inputs.joy_x > 500)
		paddle.position += paddle.speed * dt * (sdk_inputs.b ? 2 : 1);
	else if (sdk_inputs.joy_x < -500)
		paddle.position -= paddle.speed * dt * (sdk_inputs.b ? 2 : 1);

	if ((paddle.position - PADDLE_WIDTH / 2) < 0) {
		paddle.position = 0 + PADDLE_WIDTH / 2;
	}
	if ((paddle.position + PADDLE_WIDTH / 2) > TFT_RIGHT) {
		paddle.position = TFT_RIGHT - PADDLE_WIDTH / 2;
	}

	for (int i = 0; i < BALL_MOVEMENT_SUBSTEPS; i++) {
		ball.position.x += ball.direction.x * ball.speed * dt / BALL_MOVEMENT_SUBSTEPS;
		ball.position.y += ball.direction.y * ball.speed * dt / BALL_MOVEMENT_SUBSTEPS;
		handle_ball_collision();
		ball.direction = normalise(ball.direction);
	}

	// printf("x: %.2f, y: %.2f\n", ball.position.x, ball.position.y);
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	tft_draw_rect(paddle.position - PADDLE_WIDTH / 2, TFT_BOTTOM - PADDLE_HEIGHT,
		      paddle.position + PADDLE_WIDTH / 2, TFT_BOTTOM, WHITE);

	for (int y = 0; y < BLOCK_COUNT_Y; y++) {
		for (int x = 0; x < BLOCK_COUNT_X; x++) {
			tft_draw_rect(block_position.x + x * BLOCK_WIDTH,
				      block_position.y + y * BLOCK_HEIGHT,
				      block_position.x + (x + 1) * BLOCK_WIDTH - 1,
				      block_position.y + (y + 1) * BLOCK_HEIGHT - 1, blocks[y][x]);
		}
	}

	tft_draw_rect(ball.position.x - 1, ball.position.y - 1, ball.position.x + 1,
		      ball.position.y + 1, WHITE);
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
