// round.c
#include <sdk.h>
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define DEFAULT_CHAR_WIDTH 6
static int measure_string_width_local(const char *str)
{
	if (str == NULL) {
		return 0;
	}
	return strlen(str) * DEFAULT_CHAR_WIDTH;
}

#define PADDLE_WIDTH 4
#define PADDLE_HEIGHT (int)(TFT_HEIGHT / 3)
#define BALL_WIDTH 5
#define BALL_HEIGHT 5
#define PADDLE_SPEED 100

#define BALL_SPEEDUP 1.05f
#define BALL_SPEED 100
#define BALL_SPEED_DIAG 70

#ifndef GREEN
#define GREEN rgb_to_rgb565(0, 191, 0)
#endif
#ifndef GRAY
#define GRAY rgb_to_rgb565(63, 63, 63)
#endif

struct paddle {
	float y;
	int score;
	uint16_t color;
};

struct ball {
	float x, y;
	float dx, dy;
	uint16_t color;
	float speed;
};

static struct paddle paddle1, paddle2;
static struct ball ball;

static uint32_t last_tx_ball_state_time = 0;
static uint32_t last_tx_round_countdown_time = 0;
static uint32_t last_tx_paddle_state_time = 0;
static uint32_t last_tx_client_paddle_state_time = 0;

static sdk_melody_t *game_melody = NULL;
static const char game_music_str[] = "/v:50 /i:square /bpm:120 /pl"
				     "{"
				     "C D E F G A B C5 /d:50"
				     "}";

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

static void new_round(void)
{
	paddle1.y = PADDLE_HEIGHT * 1;
	paddle2.y = PADDLE_HEIGHT * 1;

	paddle1.color = BLUE;
	paddle2.color = GREEN;

	ball.x = TFT_WIDTH / 2.0f;
	ball.y = TFT_HEIGHT / 2.0f;

	ball.speed = BALL_SPEED;

	bool is_p1_local = (game_our_id == players[0].id);
	if (is_p1_local) {
		ball.dx = (rand() % 2 == 0) ? BALL_SPEED_DIAG : -BALL_SPEED_DIAG;
		ball.dy = (rand() % 2 == 0) ? BALL_SPEED_DIAG : -BALL_SPEED_DIAG;
		printf("DEBUG: Host new_round: ball.dx=%.1f, ball.dy=%.1f.\n", ball.dx, ball.dy);
	} else {
		ball.dx = 0;
		ball.dy = 0;
		printf("DEBUG: Client new_round: ball position reset. dx/dy initialized to (0,0).\n");
	}

	ball.color = WHITE;
	printf("DEBUG: Device %04x: Ball position reset for new round countdown. Ball at (%.1f, %.1f).\n",
	       game_our_id, ball.x, ball.y);
}

void game_reset(void)
{
	paddle1.score = 0;
	paddle2.score = 0;
	new_round();
	printf("DEBUG: Device %04x: Game reset. Scores to 0.\n", game_our_id);
}

static void tx_ball_state(bool force_send)
{
	uint32_t now = time_us_32();

	if (!force_send && (now - last_tx_ball_state_time < BALL_STATE_TX_INTERVAL_US)) {
		return;
	}
	last_tx_ball_state_time = now;

	int16_t x_scaled = (int16_t)(ball.x * 100);
	int16_t y_scaled = (int16_t)(ball.y * 100);
	int16_t dx_scaled = (int16_t)(ball.dx * 100);
	int16_t dy_scaled = (int16_t)(ball.dy * 100);

	uint8_t msg[9];
	msg[0] = MSG_BALL_STATE;
	msg[1] = (x_scaled >> 8) & 0xFF;
	msg[2] = x_scaled & 0xFF;
	msg[3] = (y_scaled >> 8) & 0xFF;
	msg[4] = y_scaled & 0xFF;
	msg[5] = (dx_scaled >> 8) & 0xFF;
	msg[6] = dx_scaled & 0xFF;
	msg[7] = (dy_scaled >> 8) & 0xFF;
	msg[8] = dy_scaled & 0xFF;

	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
	printf("DEBUG: Device %04x: Sent BALL_STATE (x:%.1f, y:%.1f, dx:%.1f, dy:%.1f)\n",
	       game_our_id, ball.x, ball.y, ball.dx, ball.dy);
}

static void tx_ball_bounce(void)
{
	int16_t x_scaled = (int16_t)(ball.x * 100);
	int16_t y_scaled = (int16_t)(ball.y * 100);
	int16_t dx_scaled = (int16_t)(ball.dx * 100);
	int16_t dy_scaled = (int16_t)(ball.dy * 100);

	uint8_t msg[9];
	msg[0] = MSG_BALL_BOUNCE;
	msg[1] = (x_scaled >> 8) & 0xFF;
	msg[2] = x_scaled & 0xFF;
	msg[3] = (y_scaled >> 8) & 0xFF;
	msg[4] = y_scaled & 0xFF;
	msg[5] = (dx_scaled >> 8) & 0xFF;
	msg[6] = dx_scaled & 0xFF;
	msg[7] = (dy_scaled >> 8) & 0xFF;
	msg[8] = dy_scaled & 0xFF;

	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
	printf("DEBUG: Device %04x: Sent BALL_BOUNCE (x:%.1f, y:%.1f, dx:%.1f, dy:%.1f)\n",
	       game_our_id, ball.x, ball.y, ball.dx, ball.dy);
}

static void tx_paddle_state(void)
{
	uint32_t now = time_us_32();

	if (now - last_tx_paddle_state_time < PADDLE_STATE_TX_INTERVAL_US) {
		return;
	}
	last_tx_paddle_state_time = now;

	int16_t y_scaled = (int16_t)(paddle1.y * 100);

	uint8_t msg[5];
	msg[0] = MSG_PADDLE_STATE;
	msg[1] = (y_scaled >> 8) & 0xFF;
	msg[2] = y_scaled & 0xFF;
	msg[3] = (paddle1.color >> 8) & 0xFF;
	msg[4] = paddle1.color & 0xFF;

	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
	printf("DEBUG: Device %04x: Sent PADDLE_STATE (y:%.1f, color:%04x)\n", game_our_id,
	       paddle1.y, paddle1.color);
}

static void tx_client_paddle_state(void)
{
	uint32_t now = time_us_32();

	if (now - last_tx_client_paddle_state_time < CLIENT_PADDLE_STATE_TX_INTERVAL_US) {
		return;
	}
	last_tx_client_paddle_state_time = now;

	int16_t y_scaled = (int16_t)(paddle2.y * 100);

	uint8_t msg[3];
	msg[0] = MSG_CLIENT_PADDLE_STATE;
	msg[1] = (y_scaled >> 8) & 0xFF;
	msg[2] = y_scaled & 0xFF;

	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
	printf("DEBUG: Device %04x: Sent CLIENT_PADDLE_STATE (y:%.1f)\n", game_our_id, paddle2.y);
}

static void tx_round_start_countdown(void)
{
	uint32_t now = time_us_32();

	if (now - last_tx_round_countdown_time < ROUND_START_COUNTDOWN_TX_INTERVAL_US) {
		return;
	}
	last_tx_round_countdown_time = now;

	uint8_t msg[7];
	msg[0] = MSG_ROUND_START_COUNTDOWN;
	msg[1] = (uint8_t)paddle1.score;
	msg[2] = (uint8_t)paddle2.score;
	msg[3] = (game_state_timer >> 24) & 0xFF;
	msg[4] = (game_state_timer >> 16) & 0xFF;
	msg[5] = (game_state_timer >> 8) & 0xFF;
	msg[6] = game_state_timer & 0xFF;

	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
	printf("DEBUG: Device %04x: Sent MSG_ROUND_START_COUNTDOWN (P1:%d, P2:%d, Timer:%d)\n",
	       game_our_id, paddle1.score, paddle2.score, game_state_timer);
}

static void start_new_round_countdown(void)
{
	printf("DEBUG: Device %04x: Host initiating new round countdown.\n", game_our_id);
	game_state_change(STATE_ROUND_RESET);
	new_round();
}

static void round_paint(float dt, int depth)
{
	(void)dt;
	(void)depth;

	if (0 != depth) {
		return;
	}

	tft_fill(BLACK);

	for (int i = 0; i < TFT_HEIGHT; i += 20) {
		tft_draw_rect(TFT_WIDTH / 2 - 1, i + 5, TFT_WIDTH / 2 + 1, i + 10 + 5, GRAY);
	}

	tft_draw_rect(0, paddle1.y, PADDLE_WIDTH - 1, paddle1.y + PADDLE_HEIGHT - 1, paddle1.color);
	tft_draw_rect(TFT_WIDTH - PADDLE_WIDTH, paddle2.y, TFT_WIDTH - 1,
		      paddle2.y + PADDLE_HEIGHT - 1, paddle2.color);

	tft_draw_rect(ball.x, ball.y, ball.x + BALL_WIDTH - 1, ball.y + BALL_HEIGHT - 1,
		      ball.color);

	tft_draw_string(TFT_WIDTH / 4 - 5, 0, BLUE, "%i", paddle1.score);
	tft_draw_string_right(TFT_WIDTH - (TFT_WIDTH / 4) + 5, 0, GREEN, "%i", paddle2.score);

	if (game_state == STATE_ROUND_RESET) {
		char countdown_str[10];
		int seconds_left = 2 - (game_state_timer / 1000000);
		if (seconds_left < 0)
			seconds_left = 0;
		snprintf(countdown_str, sizeof(countdown_str), "%d", seconds_left);
		tft_draw_string(TFT_WIDTH / 2 - (measure_string_width_local(countdown_str) / 2),
				TFT_HEIGHT / 2 - 10, WHITE, countdown_str);
	}
}

static bool round_handle(sdk_event_t event, int depth)
{
	(void)event;
	(void)depth;

	if (game_state != STATE_PLAYING) {
		return false;
	}

	return false;
}

static void round_tick(int jiffies, int depth)
{
	(void)depth;
	float dt = jiffies / 1000000.0f;

	bool is_p1_local = (game_our_id == players[0].id);
	bool is_p2_local = (game_our_id == players[1].id);

	if (is_p1_local && game_state == STATE_PLAYING) {
		if (sdk_inputs.joy_y > 500) {
			paddle1.y += PADDLE_SPEED * dt;
		} else if (sdk_inputs.joy_y < -500) {
			paddle1.y -= PADDLE_SPEED * dt;
		}

		if (paddle1.y < 0)
			paddle1.y = 0;
		else if (paddle1.y > TFT_HEIGHT - PADDLE_HEIGHT)
			paddle1.y = TFT_HEIGHT - PADDLE_HEIGHT;

		tx_paddle_state();
		tx_ball_state(false);
	}

	if (is_p2_local && game_state == STATE_PLAYING) {
		if (sdk_inputs.joy_y > 500) {
			paddle2.y += PADDLE_SPEED * dt;
		} else if (sdk_inputs.joy_y < -500) {
			paddle2.y -= PADDLE_SPEED * dt;
		}

		if (paddle2.y < 0)
			paddle2.y = 0;
		else if (paddle2.y > TFT_HEIGHT - PADDLE_HEIGHT)
			paddle2.y = TFT_HEIGHT - PADDLE_HEIGHT;

		tx_client_paddle_state();
	}

	if (game_state == STATE_PLAYING) {
		ball.x += ball.dx * dt;
		ball.y += ball.dy * dt;
		printf("DEBUG: Device %04x: ball.x=%.1f, y=%.1f, dx=%.1f, dy=%.1f\n", game_our_id,
		       ball.x, ball.y, ball.dx, ball.dy);
	}

	if (is_p1_local && game_state == STATE_PLAYING) {
		if (ball.y < 0) {
			ball.dy *= -1;
			ball.y = 0;
			tx_ball_bounce();
		} else if (ball.y > TFT_BOTTOM - BALL_HEIGHT) {
			ball.dy *= -1;
			ball.y = TFT_BOTTOM - BALL_HEIGHT;
			tx_ball_bounce();
		}

		if (rects_overlap(ball.x, ball.y, ball.x + BALL_WIDTH, ball.y + BALL_HEIGHT, 0,
				  paddle1.y, PADDLE_WIDTH - 1, paddle1.y + PADDLE_HEIGHT)) {
			ball.color = BLUE;
			float mid = paddle1.y + PADDLE_HEIGHT / 2.0f;
			float ballmid = ball.y + BALL_HEIGHT / 2.0f;
			float impact =
				(mid - ballmid) / (PADDLE_HEIGHT / 2.0f + BALL_HEIGHT) / 1.5f;
			float refl = asinf(impact);

			ball.speed *= BALL_SPEEDUP;
			ball.dx = cosf(refl) * ball.speed;
			ball.dy = -sinf(refl) * ball.speed;
			ball.x = PADDLE_WIDTH + 1;
			printf("DEBUG: Device %04x: Ball hit P1 paddle. New speed: %.1f\n",
			       game_our_id, ball.speed);
			tx_ball_bounce();
		} else if (ball.x < PADDLE_WIDTH - 1) {
			paddle2.score++;
			printf("DEBUG: Device %04x: P1 missed. P2 score: %d\n", game_our_id,
			       paddle2.score);
			start_new_round_countdown();
		}

		if (rects_overlap(ball.x, ball.y, ball.x + BALL_WIDTH, ball.y + BALL_HEIGHT,
				  TFT_WIDTH - PADDLE_WIDTH, paddle2.y, TFT_WIDTH - 1,
				  paddle2.y + PADDLE_HEIGHT)) {
			ball.color = GREEN;
			float mid = paddle2.y + PADDLE_HEIGHT / 2.0f;
			float ballmid = ball.y + BALL_HEIGHT / 2.0f;
			float impact =
				(mid - ballmid) / (PADDLE_HEIGHT / 2.0f + BALL_HEIGHT) / 1.5f;
			float refl = asinf(impact) + M_PI;

			ball.speed *= BALL_SPEEDUP;
			ball.dx = cosf(refl) * ball.speed;
			ball.dy = sinf(refl) * ball.speed;
			ball.x = TFT_WIDTH - PADDLE_WIDTH - BALL_WIDTH - 1;
			printf("DEBUG: Device %04x: Ball hit P2 paddle. New speed: %.1f\n",
			       game_our_id, ball.speed);
			tx_ball_bounce();
		} else if (ball.x + BALL_WIDTH > TFT_WIDTH - PADDLE_WIDTH + 1) {
			paddle1.score++;
			printf("DEBUG: Device %04x: P2 missed. P1 score: %d\n", game_our_id,
			       paddle1.score);
			start_new_round_countdown();
		}
	}

	if (game_state == STATE_ROUND_RESET) {
		if (game_state_timer >= 2000000) {
			printf("DEBUG: Device %04x: Round reset countdown finished. Transitioning to PLAYING.\n",
			       game_our_id);
			game_state_change(STATE_PLAYING);
			if (is_p1_local) {
				tx_ball_state(true);
			}
		} else {
			bool is_p1_local = (game_our_id == players[0].id);
			if (is_p1_local) {
				tx_round_start_countdown();
			}
		}
	}
}

static void round_pushed(void)
{
	printf("Round scene pushed! Stopping menu music and starting game music.\n");
	game_melody = sdk_melody_play_get(game_music_str);
}

static void round_revealed(void)
{
	printf("Round scene revealed!\n");
}

static bool round_inbox(sdk_message_t msg, int depth)
{
	(void)depth;

	if (msg.type != SDK_MSG_RF) {
		return false;
	}

	switch (msg.rf.data[0]) {
	case MSG_BALL_STATE:
	case MSG_BALL_BOUNCE:
		if (msg.rf.length == 9) {
			int16_t x_scaled = (msg.rf.data[1] << 8) | msg.rf.data[2];
			int16_t y_scaled = (msg.rf.data[3] << 8) | msg.rf.data[4];
			int16_t dx_scaled = (msg.rf.data[5] << 8) | msg.rf.data[6];
			int16_t dy_scaled = (msg.rf.data[7] << 8) | msg.rf.data[8];

			ball.x = (float)x_scaled / 100.0f;
			ball.y = (float)y_scaled / 100.0f;
			ball.dx = (float)dx_scaled / 100.0f;
			ball.dy = (float)dy_scaled / 100.0f;

			printf("DEBUG: Device %04x: Client received BALL_%s. New x:%.1f, y:%.1f, dx:%.1f, dy:%.1f\n",
			       game_our_id, (msg.rf.data[0] == MSG_BALL_STATE ? "STATE" : "BOUNCE"),
			       ball.x, ball.y, ball.dx, ball.dy);
			return true;
		}
		return false;
	case MSG_ROUND_START_COUNTDOWN:
		if (msg.rf.length == 7) {
			paddle1.score = (int)msg.rf.data[1];
			paddle2.score = (int)msg.rf.data[2];
			uint32_t received_timer = (msg.rf.data[3] << 24) | (msg.rf.data[4] << 16) |
						  (msg.rf.data[5] << 8) | msg.rf.data[6];

			printf("DEBUG: Device %04x: Received MSG_ROUND_START_COUNTDOWN (P1:%d, P2:%d, Host Timer:%d). Starting countdown.\n",
			       game_our_id, paddle1.score, paddle2.score, received_timer);

			game_state_change(STATE_ROUND_RESET);
			game_state_timer = received_timer;
			new_round();
			return true;
		}
		return false;
	case MSG_PADDLE_STATE:
		if (msg.rf.length == 5) {
			int16_t y_scaled = (msg.rf.data[1] << 8) | msg.rf.data[2];
			uint16_t received_color = (msg.rf.data[3] << 8) | msg.rf.data[4];

			paddle1.y = (float)y_scaled / 100.0f;
			paddle1.color = received_color;

			printf("DEBUG: Device %04x: Client received PADDLE_STATE (y:%.1f, color:%04x).\n",
			       game_our_id, paddle1.y, paddle1.color);
			return true;
		}
		return false;
	case MSG_CLIENT_PADDLE_STATE:
		if (msg.rf.length == 3) {
			int16_t y_scaled = (msg.rf.data[1] << 8) | msg.rf.data[2];

			paddle2.y = (float)y_scaled / 100.0f;

			printf("DEBUG: Device %04x: Host received CLIENT_PADDLE_STATE (y:%.1f).\n",
			       game_our_id, paddle2.y);
			return true;
		}
		return false;
	default:
		return false;
	}
	return false;
}

sdk_scene_t scene_round = {
	.paint = round_paint,
	.handle = round_handle,
	.tick = round_tick,
	.pushed = round_pushed,
	.revealed = round_revealed,
	.inbox = round_inbox,
};
