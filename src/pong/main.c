#include <pico/stdlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <sdk.h>
#include <tft.h>

#define PADDLE_WIDTH 4
#define PADDLE_HEIGHT (int)(TFT_HEIGHT / 3)
#define PADDLE_MID (PADDLE_HEIGHT / 2)
#define BALL_WIDTH 5
#define BALL_HEIGHT 5
#define PADDLE_SPEED 100

#define BALL_SPEEDUP 1.05f
#define BALL_SPEED 100
#define BALL_SPEED_DIAG 70

#define GREEN rgb_to_rgb565(0, 191, 0)
#define BLUE rgb_to_rgb565(63, 63, 255)
#define GRAY rgb_to_rgb565(63, 63, 63)
#define WHITE rgb_to_rgb565(255, 255, 255)

struct paddle {
	float y;
	int score;
};

struct ball {
	float x, y;
	float dx, dy;
	uint16_t color;
	float speed;
};

static struct paddle paddle1, paddle2;
static struct ball ball;

sdk_player_t player;

sdk_track_square_t track_left_paddle = {
	.track = {
		.fn = sdk_play_square,
		.base = 16000,
		.peak = 20000,
		.attack = 0.05 * SDK_AUDIO_RATE,
		.decay = 0.05 * SDK_AUDIO_RATE,
		.sustain = 0.10 * SDK_AUDIO_RATE,
		.release = 0.10 * SDK_AUDIO_RATE,
		.pan = -20000,
	},
	.frequency = 440,
};

sdk_track_square_t track_right_paddle = {
	.track = {
		.fn = sdk_play_square,
		.base = 16000,
		.peak = 20000,
		.attack = 0.05 * SDK_AUDIO_RATE,
		.decay = 0.05 * SDK_AUDIO_RATE,
		.sustain = 0.10 * SDK_AUDIO_RATE,
		.release = 0.10 * SDK_AUDIO_RATE,
		.pan = 20000,
	},
	.frequency = 440 * 1.25,
};

sdk_track_noise_t track_left_miss = {
	.track = {
		.fn = sdk_play_noise,
		.base = 8000,
		.peak = 16000,
		.attack = 0.05 * SDK_AUDIO_RATE,
		.decay = 0.10 * SDK_AUDIO_RATE,
		.release = 0.10 * SDK_AUDIO_RATE,
		.pan = -20000,
	},
};

sdk_track_noise_t track_right_miss = {
	.track = {
		.fn = sdk_play_noise,
		.base = 8000,
		.peak = 16000,
		.attack = 0.05 * SDK_AUDIO_RATE,
		.decay = 0.10 * SDK_AUDIO_RATE,
		.release = 0.10 * SDK_AUDIO_RATE,
		.pan = 20000,
	},
};

static bool rects_overlap(int x0, int y0, int x1, int y1, int a0, int b0, int a1, int b1)
{
	int tmp;

	if (x0 > x1)
		tmp = x1, x1 = x0, x0 = tmp;

	if (a0 > a1)
		tmp = a1, a1 = a0, a0 = tmp;

	if (y0 > y1)
		tmp = y1, y1 = y0, y0 = tmp;

	if (b0 > b1)
		tmp = b1, b1 = b0, b0 = tmp;

	if (x1 < a0)
		return false;

	if (x0 > a1)
		return false;

	if (y1 < b0)
		return false;

	if (y0 > b1)
		return false;

	return true;
}

void game_start(void)
{
}

void game_audio(int nsamples)
{
	for (int s = 0; s < nsamples; s++) {
		int16_t left, right;
		sdk_player_sample(&player, &left, &right);
		sdk_write_sample(left, right);
	}
}

static void new_round(void)
{
	paddle1.y = PADDLE_HEIGHT * 1;
	paddle2.y = PADDLE_HEIGHT * 1;

	ball.x = TFT_WIDTH / 2.0f;
	ball.y = TFT_HEIGHT / 2.0f;

	ball.speed = BALL_SPEED;
	ball.dx = (rand() & 1) ? BALL_SPEED_DIAG : -BALL_SPEED_DIAG;
	ball.dy = (rand() & 1) ? BALL_SPEED_DIAG : -BALL_SPEED_DIAG;

	ball.color = WHITE;
}

void game_reset(void)
{
	paddle1.score = 0;
	paddle2.score = 0;
	new_round();
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	/* Joys have value from -2048 to +2047. */
	if (sdk_inputs.joy_y > 500)
		paddle1.y += PADDLE_SPEED * dt;
	else if (sdk_inputs.joy_y < -500)
		paddle1.y -= PADDLE_SPEED * dt;

	if (paddle1.y < 0)
		paddle1.y = 0;
	else if (paddle1.y > TFT_BOTTOM - PADDLE_HEIGHT)
		paddle1.y = TFT_BOTTOM - PADDLE_HEIGHT;

	if (sdk_inputs.a)
		paddle2.y += PADDLE_SPEED * dt;
	else if (sdk_inputs.y)
		paddle2.y -= PADDLE_SPEED * dt;

	if (paddle2.y < 0)
		paddle2.y = 0;
	else if (paddle2.y > TFT_BOTTOM - PADDLE_HEIGHT)
		paddle2.y = TFT_BOTTOM - PADDLE_HEIGHT;

	/* ball is moving */
	ball.x += ball.dx * dt;
	ball.y += ball.dy * dt;

	/* ball can bounce from top and bottom */
	if (ball.y < 0) {
		ball.dy *= -1;
		ball.y = 0;
	} else if (ball.y > TFT_BOTTOM - BALL_HEIGHT) {
		ball.dy *= -1;
		ball.y = TFT_BOTTOM - BALL_HEIGHT;
	}

	if (rects_overlap(ball.x, ball.y, ball.x + BALL_WIDTH, ball.y + BALL_HEIGHT, 0, paddle1.y,
			  PADDLE_WIDTH - 1, paddle1.y + PADDLE_HEIGHT)) {
		// prekryv s levou palkou
		ball.color = BLUE;

		float mid = paddle1.y + PADDLE_HEIGHT / 2.0f;
		float ballmid = ball.y + BALL_HEIGHT / 2.0f;

		float impact = (mid - ballmid) / (PADDLE_HEIGHT / 2.0f + BALL_HEIGHT) / 1.5f;
		float refl = asinf(impact);

		ball.speed *= BALL_SPEEDUP;
		ball.dx = cosf(refl) * ball.speed;
		ball.dy = -sinf(refl) * ball.speed;

		ball.x = PADDLE_WIDTH + 1;

		sdk_add_track(&player, &track_left_paddle.track);
	} else if (ball.x < PADDLE_WIDTH - 1) {
		// srazka s levou zdi
		paddle2.score++;
		sdk_add_track(&player, &track_left_miss.track);
		new_round();
	}

	if (rects_overlap(ball.x, ball.y, ball.x + BALL_WIDTH, ball.y + BALL_HEIGHT, TFT_RIGHT,
			  paddle2.y, TFT_RIGHT - PADDLE_WIDTH + 1, paddle2.y + PADDLE_HEIGHT)) {
		// prekryv s pravou
		ball.color = GREEN;

		float mid = paddle2.y + PADDLE_HEIGHT / 2.0f;
		float ballmid = ball.y + BALL_HEIGHT / 2.0f;

		float impact = (mid - ballmid) / (PADDLE_HEIGHT / 2.0f + BALL_HEIGHT) / 1.5f;
		float refl = asinf(impact) + M_PI;

		ball.speed *= BALL_SPEEDUP;
		ball.dx = cosf(refl) * ball.speed;
		ball.dx = cosf(refl) * ball.speed;
		ball.dy = sinf(refl) * ball.speed;

		ball.x = TFT_RIGHT - PADDLE_WIDTH - BALL_WIDTH - 1;

		sdk_add_track(&player, &track_right_paddle.track);
	} else if (ball.x + BALL_WIDTH > TFT_RIGHT - PADDLE_WIDTH + 1) {
		// srazka s pravou zdi
		paddle1.score++;
		sdk_add_track(&player, &track_right_miss.track);
		new_round();
	}
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	for (int i = 0; i < TFT_HEIGHT; i += 20) {
		tft_draw_rect(TFT_WIDTH / 2, i + 5, TFT_WIDTH / 2, i + 10 + 5, ball.color);
	}

	tft_draw_rect(TFT_RIGHT - 1, 0, TFT_RIGHT - 2, TFT_BOTTOM, 3);
	tft_draw_rect(1, 0, 2, TFT_BOTTOM, 3);

	/* draw paddles */
	tft_draw_rect(0, paddle1.y, PADDLE_WIDTH - 1, paddle1.y + PADDLE_HEIGHT, BLUE);
	tft_draw_rect(TFT_RIGHT, paddle2.y, TFT_RIGHT - PADDLE_WIDTH + 1, paddle2.y + PADDLE_HEIGHT,
		      GREEN);

	/* draw ball */
	tft_draw_rect(ball.x, ball.y, ball.x + BALL_WIDTH, ball.y + BALL_HEIGHT, ball.color);

	char buf[16];
	snprintf(buf, sizeof buf, "%i", paddle1.score);
	tft_draw_string(0 + 10, 0, BLUE, buf);

	snprintf(buf, sizeof buf, "%i", paddle2.score);
	tft_draw_string_right(TFT_RIGHT - 10, 0, GREEN, buf);
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
