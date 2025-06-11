#include <math.h>
#include <string.h>

#include "common.h"

#define RED rgb_to_rgb565(255, 0, 0)
#define YELLOW rgb_to_rgb565(255, 255, 0)
#define GREEN rgb_to_rgb565(0, 255, 0)
#define BLUE rgb_to_rgb565(0, 0, 255)
#define GRAY rgb_to_rgb565(63, 63, 63)
#define WHITE rgb_to_rgb565(255, 255, 255)

#define ANGLE_DELTA (250.0f / 180.0f * M_PI)

struct worm {
	uint16_t id;
	float x, y;
	float angle;
	float speed;
	color_t color;
	bool alive;
};

static int8_t grid[TFT_HEIGHT][TFT_WIDTH];

static int turn_left, turn_right;

#define NUM_WORMS 4
static struct worm worms[NUM_WORMS];
static struct worm worms_init[NUM_WORMS] = {
	{
		.id = 0,
		.x = TFT_WIDTH / 2.0f - 5,
		.y = TFT_HEIGHT / 2.0f - 5,
		.angle = 225.0f / 180.0f * M_PI,
		.speed = 0.5f,
		.color = RED,
		.alive = true,
	},
	{
		.id = 0,
		.x = TFT_WIDTH / 2.0f + 5,
		.y = TFT_HEIGHT / 2.0f + 5,
		.angle = 45.0f / 180.0f * M_PI,
		.speed = 0.5f,
		.color = GREEN,
		.alive = true,
	},
	{
		.id = 0,
		.x = TFT_WIDTH / 2.0f + 5,
		.y = TFT_HEIGHT / 2.0f - 5,
		.angle = 315.0f / 180.0f * M_PI,
		.speed = 0.5f,
		.color = BLUE,
		.alive = true,
	},
	{
		.id = 0,
		.x = TFT_WIDTH / 2.0f - 5,
		.y = TFT_HEIGHT / 2.0f + 5,
		.angle = 135.0f / 180.0f * M_PI,
		.speed = 0.5f,
		.color = YELLOW,
		.alive = true,
	},

};

static bool round_handle(sdk_event_t event, int depth)
{
	if (depth)
		return false;

	switch (event) {
	case SDK_READ_JOYSTICK:
		turn_left = (turn_left & 0b01) | ((sdk_inputs.jx < -0.5f) << 1);
		turn_right = (turn_right & 0b01) | ((sdk_inputs.jx > 0.5f) << 1);
		return true;

	case SDK_PRESSED_B:
		turn_right |= 0b01;
		return true;

	case SDK_RELEASED_B:
		turn_right &= 0b10;
		return true;

	case SDK_PRESSED_X:
		turn_left |= 0b01;
		return true;

	case SDK_RELEASED_X:
		turn_left &= 0b10;
		return true;

	default:
		return false;
	}
}

static void tx_coords(float x, float y)
{
	uint8_t msg[] = { MSG_COORDS, game_our_id >> 8, game_our_id, x, y };
	sdk_send_rf(SDK_RF_ALL, msg, sizeof(msg));
}

static void round_paint(float dt, int depth)
{
	(void)depth;

	int our_worm = players[0].worm;

	if (our_worm >= 0) {
		if (turn_left && !turn_right)
			worms[our_worm].angle -= ANGLE_DELTA * dt;

		if (turn_right && !turn_left)
			worms[our_worm].angle += ANGLE_DELTA * dt;
	}

	tft_fill(0);

	tft_draw_rect(0, 0, 0, TFT_HEIGHT - 1, 7);
	tft_draw_rect(0, 0, TFT_WIDTH - 1, 0, 7);
	tft_draw_rect(TFT_WIDTH - 1, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1, 7);
	tft_draw_rect(0, TFT_HEIGHT - 1, TFT_WIDTH - 1, TFT_HEIGHT - 1, 7);

	for (int x = 0; x < TFT_WIDTH; x++) {
		for (int y = 0; y < TFT_HEIGHT; y++) {
			int8_t owner = grid[y][x];

			if (owner < 0)
				continue;

			int8_t age = owner & 0x0f;
			owner >>= 4;

			if (owner >= 0)
				tft_draw_pixel(x, y, worms[owner].color);

			if (age)
				grid[y][x] = (owner << 4) | (age - 1);
		}
	}

	int worms_alive = 0;

	for (int i = 0; i < NUM_WORMS; i++) {
		if (!worms[i].id)
			continue;

		if (!worms[i].alive)
			continue;

		worms_alive += 1;

		float hspd = 0, vspd = 0;

		if (worms[i].id == game_our_id) {
			hspd = worms[i].speed * cosf(worms[i].angle);
			vspd = worms[i].speed * sinf(worms[i].angle);
		}

		float x = worms[i].x += hspd;
		float y = worms[i].y += vspd;

		if (worms[i].id == game_our_id)
			tx_coords(worms[i].x, worms[i].y);

		int nx[4] = {
			clamp(x + 1, 0, TFT_WIDTH - 1),
			clamp(x + 1, 0, TFT_WIDTH - 1),
			clamp(x, 0, TFT_WIDTH - 1),
			clamp(x, 0, TFT_WIDTH - 1),
		};

		int ny[4] = {
			clamp(y + 1, 0, TFT_HEIGHT - 1),
			clamp(y, 0, TFT_HEIGHT - 1),
			clamp(y + 1, 0, TFT_HEIGHT - 1),
			clamp(y, 0, TFT_HEIGHT - 1),
		};

		for (int j = 0; j < 4; j++) {
			int8_t owner = grid[ny[j]][nx[j]];
			int8_t age = owner & 0x0f;
			owner >>= 4;

			if ((owner >= 0) && (owner == i) && !age) {
				worms[i].alive = false;
				continue;
			}

			if ((owner >= 0) && (owner != i)) {
				worms[i].alive = false;
				continue;
			}

			grid[ny[j]][nx[j]] = (i << 4) | 0x0f;
		}

		if (worms[i].x < 0 || worms[i].x > TFT_WIDTH)
			worms[i].alive = false;

		if (worms[i].y < 0 || worms[i].y > TFT_HEIGHT)
			worms[i].alive = false;
	}

	if (worms_alive <= 0)
		sdk_scene_pop();
}

static void round_pushed(void)
{
	for (int x = 0; x < TFT_WIDTH; x++)
		for (int y = 0; y < TFT_HEIGHT; y++)
			grid[y][x] = -1;

	memcpy(worms, worms_init, sizeof worms);

	for (int i = 0; i < NUM_PLAYERS; i++) {
		if (!players[i].id || players[i].worm < 0)
			continue;

		int worm = players[i].worm;
		worms[worm].id = players[i].id;
		worms[worm].color = players[i].color;
	}
}

sdk_scene_t scene_round = {
	.paint = round_paint,
	.handle = round_handle,
	.pushed = round_pushed,
};
