#include <pico/stdlib.h>

#include <math.h>
#include <string.h>

#include <sdk.h>
#include <tft.h>

#define RED rgb_to_rgb565(255, 0, 0)
#define YELLOW rgb_to_rgb565(255, 255, 0)
#define GREEN rgb_to_rgb565(0, 255, 0)
#define BLUE rgb_to_rgb565(0, 0, 255)
#define GRAY rgb_to_rgb565(63, 63, 63)
#define WHITE rgb_to_rgb565(255, 255, 255)

#define ANGLE_DELTA (5.0f / 180.0f * M_PI)

enum screen {
	GAME = 0,
	SCORE,
};

static enum screen current_screen = GAME;
static uint32_t start_time;
static bool play_music = true;

struct worm {
	float x, y;
	float angle;
	float speed;
	color_t color;
	bool alive;
	uint32_t tod;
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

static uint16_t tones[256];
static const char tune[] =
	" ggahC g C g C g CChag D g D g D ggahC g C g C g CCDEF C F C F C FFEDC g C g C g CChag D g D g D";

void game_start(void)
{
	current_screen = GAME;

	tones['c'] = 131;
	tones['d'] = 147;
	tones['e'] = 165;
	tones['f'] = 175;
	tones['g'] = 196;
	tones['a'] = 220;
	tones['h'] = 247;

	tones['C'] = 261;
	tones['D'] = 293;
	tones['E'] = 329;
	tones['F'] = 349;
	tones['G'] = 392;
	tones['A'] = 440; // <--
	tones['H'] = 494;
}

void game_reset(void)
{
	current_screen = GAME;
	start_time = time_us_32();

	for (int x = 0; x < TFT_WIDTH; x++) {
		for (int y = 0; y < TFT_HEIGHT; y++) {
			grid[y][x] = -1;
		}
	}

	memcpy(worms, worms_init, sizeof worms);
	play_music = true;
}

void game_audio(int nsamples)
{
	static int ellapsed = 0;
	static int tone_pos = 0;

	if (!play_music) {
		tone_pos = 0;
		ellapsed = 0;

		for (int s = 0; s < nsamples; s++)
			sdk_write_sample(0, 0);

		return;
	}

	for (int s = 0; s < nsamples; s++) {
		if (ellapsed > SDK_AUDIO_RATE / 4) {
			tone_pos++;
			ellapsed = 0;
		}

		if (!tune[tone_pos]) {
			tone_pos = 0;
		}

		int freq = tones[(unsigned)tune[tone_pos]];

		if (freq) {
			int period = SDK_AUDIO_RATE / freq;
			int half_period = 2 * ellapsed / period;
			int modulo = half_period & 1;
			int16_t sample = 4000 * (modulo ? 1 : -1);
			sdk_write_sample(sample, sample);
		} else {
			sdk_write_sample(0, 0);
		}

		ellapsed++;
	}
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

static void paint_game()
{
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

	int worms_alive = 0;

	for (int i = 0; i < NUM_WORMS; i++) {
		worms_alive += worms[i].alive;

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

		if (worms[i].alive)
			worms[i].tod = time_us_32() - start_time;
	}

	if (worms_alive <= 1) {
		current_screen = SCORE;
	}
}

static int pick_next_loser(uint32_t tod_better_than)
{
	uint32_t worst_tod = UINT32_MAX;
	int worst = 0;

	for (int i = 0; i < NUM_WORMS; i++) {
		if (worms[i].tod > tod_better_than && worms[i].tod < worst_tod) {
			worst_tod = worms[i].tod;
			worst = i;
		}
	}

	return worst;
}

static void paint_score()
{
	play_music = false;

	if (sdk_inputs.start) {
		game_reset();
	}

	int loser4 = pick_next_loser(0);
	int loser3 = pick_next_loser(worms[loser4].tod);
	int loser2 = pick_next_loser(worms[loser3].tod);
	int loser1 = pick_next_loser(worms[loser2].tod);

	tft_draw_rect(0, 0, TFT_RIGHT, TFT_BOTTOM, worms[loser4].color);
	tft_draw_rect(TFT_WIDTH / 8, TFT_HEIGHT / 8, TFT_RIGHT - TFT_WIDTH / 8,
		      TFT_BOTTOM - TFT_HEIGHT / 8, worms[loser3].color);
	tft_draw_rect(2 * TFT_WIDTH / 8, 2 * TFT_HEIGHT / 8, TFT_RIGHT - 2 * TFT_WIDTH / 8,
		      TFT_BOTTOM - 2 * TFT_HEIGHT / 8, worms[loser2].color);
	tft_draw_rect(3 * TFT_WIDTH / 8, 3 * TFT_HEIGHT / 8, TFT_RIGHT - 3 * TFT_WIDTH / 8,
		      TFT_BOTTOM - 3 * TFT_HEIGHT / 8, worms[loser1].color);
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	if (current_screen == GAME) {
		paint_game();
	} else if (current_screen == SCORE) {
		paint_score();
	}
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
