#include <pico/stdlib.h>

#include <math.h>
#include <string.h>

#include <sdk.h>
#include <tft.h>

#define RED 255
#define YELLOW 241
#define GREEN 244
#define BLUE 234
#define GRAY 8
#define WHITE 15

#define ANGLE_DELTA (5.0f / 180.0f * M_PI)

struct worm {
	float x, y;
	float angle;
	float speed;
	uint8_t color;
	bool alive;
};

static int8_t grid[TFT_HEIGHT][TFT_WIDTH];

#define NUM_WORMS 4
static struct worm worms[NUM_WORMS];
static struct worm worms_init[NUM_WORMS] = {
	{
		.x = TFT_WIDTH / 2.0f - 5,
		.y = TFT_HEIGHT / 2.0f - 5,
		.angle = 225.0f / 180.0f * M_PI,
		.speed = 0.5f,
		.color = RED,
		.alive = true,
	},
	{
		.x = TFT_WIDTH / 2.0f + 5,
		.y = TFT_HEIGHT / 2.0f + 5,
		.angle = 45.0f / 180.0f * M_PI,
		.speed = 0.5f,
		.color = GREEN,
		.alive = true,
	},
	{
		.x = TFT_WIDTH / 2.0f + 5,
		.y = TFT_HEIGHT / 2.0f - 5,
		.angle = 315.0f / 180.0f * M_PI,
		.speed = 0.5f,
		.color = BLUE,
		.alive = true,
	},
	{
		.x = TFT_WIDTH / 2.0f - 5,
		.y = TFT_HEIGHT / 2.0f + 5,
		.angle = 135.0f / 180.0f * M_PI,
		.speed = 0.5f,
		.color = YELLOW,
		.alive = true,
	},

};

inline static int clamp(int x, int lo, int hi)
{
	if (x < lo)
		return lo;

	if (x > hi)
		return hi;

	return x;
}

void game_reset(void)
{
	for (int x = 0; x < TFT_WIDTH; x++) {
		for (int y = 0; y < TFT_HEIGHT; y++) {
			grid[y][x] = -1;
		}
	}

	memcpy(worms, worms_init, sizeof worms);
}

float angle_diff(float a, float b)
{
	float diff = fmodf(a - b, 2 * M_PI);

	if (diff < -M_PI)
		diff += 2 * M_PI;
	else if (diff > M_PI)
		diff -= 2 * M_PI;

	return diff;
}

void game_paint(unsigned __unused dt_usec)
{
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

	if (sdk_inputs.x)
		worms[0].angle -= ANGLE_DELTA;

	if (sdk_inputs.a)
		worms[0].angle += ANGLE_DELTA;

	if (sdk_inputs.b)
		worms[1].angle -= ANGLE_DELTA;

	if (sdk_inputs.y)
		worms[1].angle += ANGLE_DELTA;

	float p2_mag = sqrtf(powf(sdk_inputs.joy_x, 2) + powf(sdk_inputs.joy_y, 2));

	if (p2_mag > 200) {
		float new_angle = atan2f(sdk_inputs.joy_y, sdk_inputs.joy_x);
		float diff = angle_diff(worms[2].angle, new_angle);
		worms[2].angle -= diff * p2_mag / 2048 / 20;
	}

	if (sdk_inputs.vol_up)
		worms[3].angle -= ANGLE_DELTA;

	if (sdk_inputs.vol_down)
		worms[3].angle += ANGLE_DELTA;

	bool anybody_alive = false;

	for (int i = 0; i < NUM_WORMS; i++) {
		anybody_alive |= worms[i].alive;

		if (!worms[i].alive)
			continue;

		float hspd = worms[i].speed * cosf(worms[i].angle);
		float vspd = worms[i].speed * sinf(worms[i].angle);

		float x = worms[i].x += hspd;
		float y = worms[i].y += vspd;

		int nx[4] = {
			clamp(x + 0.5f, 0, TFT_WIDTH - 1),
			clamp(x + 0.5f, 0, TFT_WIDTH - 1),
			clamp(x - 0.5f, 0, TFT_WIDTH - 1),
			clamp(x - 0.5f, 0, TFT_WIDTH - 1),
		};

		int ny[4] = {
			clamp(y + 0.5f, 0, TFT_HEIGHT - 1),
			clamp(y - 0.5f, 0, TFT_HEIGHT - 1),
			clamp(y + 0.5f, 0, TFT_HEIGHT - 1),
			clamp(y - 0.5f, 0, TFT_HEIGHT - 1),
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

	if (!anybody_alive)
		game_reset();
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = true,
	};

	sdk_main(&config);
}
