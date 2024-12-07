#include <pico/stdlib.h>

#include <stdlib.h>
#include <math.h>

#include <sdk.h>
#include <tft.h>

typedef enum {
	EMPTY = 0,
	SUGAR = 1,
	POWERUP,
	CHERRY,
	WALL_SQUARE,
	WALL_H,
	WALL_V,
	WALL_T_N,
	WALL_T_W,
	WALL_T_S,
	WALL_T_E,
	CORNER_NW,
	CORNER_NE,
	CORNER_SW,
	CORNER_SE,
	EDGE_N,
	EDGE_W,
	EDGE_S,
	EDGE_E,
	INVISIBLE_WALL,

} TileType;

TileType map[15][20] = {
	{ 14, 5, 5, 5, 5, 9, 5, 5, 5, 5, 5, 5, 5, 5, 9, 5, 5, 5, 5, 13 },
	{ 6, 2, 1, 1, 1, 6, 1, 1, 1, 1, 1, 1, 1, 1, 6, 1, 1, 1, 2, 6 },
	{ 6, 1, 14, 18, 1, 17, 1, 16, 5, 5, 5, 5, 18, 1, 17, 1, 16, 13, 1, 6 },
	{ 6, 1, 6, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 1, 6 },
	{ 6, 1, 17, 1, 16, 18, 1, 14, 18, 19, 19, 16, 13, 1, 16, 18, 1, 17, 1, 6 },
	{ 6, 1, 1, 1, 1, 1, 1, 6, 0, 0, 0, 0, 6, 1, 1, 1, 1, 1, 1, 6 },
	{ 10, 5, 5, 18, 1, 4, 1, 6, 0, 0, 0, 0, 6, 1, 4, 1, 16, 5, 5, 8 },
	{ 6, 1, 1, 1, 1, 1, 1, 6, 0, 0, 0, 0, 6, 1, 1, 1, 1, 1, 1, 6 },
	{ 6, 1, 15, 1, 16, 18, 1, 12, 5, 5, 5, 5, 11, 1, 16, 18, 1, 15, 1, 6 },
	{ 6, 1, 6, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 1, 6 },
	{ 6, 1, 12, 18, 1, 15, 1, 16, 5, 5, 5, 5, 18, 1, 15, 1, 16, 11, 1, 6 },
	{ 6, 2, 1, 1, 1, 6, 1, 1, 1, 1, 1, 1, 1, 1, 6, 1, 1, 1, 2, 6 },
	{ 12, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 11 },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
	{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	  EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
};

#define RED 255
#define YELLOW 242
#define GREEN 244
#define BLUE 250
#define GRAY 8
#define WHITE 15

#define PACMAN_SIZE 7

struct pacman {
	float x, y;
	float dx, dy;
	float fdx, fdy;
	uint8_t color;
	float speed;
};

static struct pacman pacman;

struct effect;

typedef int16_t (*effect_gen_fn)(struct effect *eff);

struct effect {
	int offset;
	int length;
	int volume;
	int period;
	effect_gen_fn generator;
};

#define MAX_EFFECTS 8
struct effect effects[MAX_EFFECTS];

static int16_t __unused square_wave(struct effect *eff)
{
	if ((eff->offset % eff->period) < (eff->period / 2))
		return eff->volume;
	else
		return -eff->volume;
}

static int16_t __unused noise(struct effect *eff)
{
	return rand() % (2 * eff->volume) - eff->volume;
}

void game_start(void)
{
	sdk_set_output_gain_db(6);

	pacman.speed = 32;
}

void game_audio(int nsamples)
{
	for (int s = 0; s < nsamples; s++) {
		int sample = 0;

		for (int i = 0; i < MAX_EFFECTS; i++) {
			struct effect *e = effects + i;

			if (!e->volume)
				continue;

			sample += e->generator(e);

			if (e->offset++ >= e->length)
				e->volume = 0;
		}

		sdk_write_sample(sample);
	}
}

static void __unused play_effect(int volume, int frequency, int length, effect_gen_fn gen)
{
	for (int i = 0; i < MAX_EFFECTS; i++) {
		struct effect *e = effects + i;

		if (e->volume)
			continue;

		e->offset = 0;
		e->volume = volume;
		e->length = length;
		e->period = frequency ? 48000 / frequency : 1;
		e->generator = gen;

		break;
	}
}

static void new_round(void)
{
	pacman.x = 15.5 * 8;
	pacman.y = 6.5 * 8;
}

void game_reset(void)
{
	new_round();
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	if (sdk_inputs_delta.a > 0) {
		if (!pacman.dx) {
			pacman.dy = 1;
		} else {
			pacman.fdy = 1, pacman.fdx = 0;
		}
	} else if (sdk_inputs_delta.y > 0) {
		if (!pacman.dx) {
			pacman.dy = -1;
		} else {
			pacman.fdy = -1, pacman.fdx = 0;
		}
	} else if (sdk_inputs_delta.b > 0) {
		if (!pacman.dy) {
			pacman.dx = 1;
		} else {
			pacman.fdx = 1, pacman.fdy = 0;
		}
	} else if (sdk_inputs_delta.x > 0) {
		if (!pacman.dy) {
			pacman.dx = -1;
		} else {
			pacman.fdx = -1, pacman.fdy = 0;
		}
	}

	if (pacman.fdx || pacman.fdy) {
		float dx = fabsf(fmodf(pacman.x, 8) / 8 - 0.5f);
		float dy = fabsf(fmodf(pacman.y, 8) / 8 - 0.5f);
		float d = MAX(dx, dy);

		if (d < (2 * pacman.speed / (30 * 8))) {
			int hx = pacman.x / 8 + pacman.fdx;
			int hy = pacman.y / 8 + pacman.fdy;

			if (hx >= 0 && hx < 20 && hy >= 0 && hy < 15 && map[hy][hx] <= CHERRY) {
				pacman.dx = pacman.fdx;
				pacman.dy = pacman.fdy;
				pacman.fdx = 0;
				pacman.fdy = 0;
			}
		}
	}

	if (pacman.dy || pacman.dx) {
		float future_x = clamp(pacman.x + pacman.dx * dt * pacman.speed, 8 * 0.5,
				       TFT_RIGHT - 8 * 0.5);
		float future_y = clamp(pacman.y + pacman.dy * dt * pacman.speed, 8 * 0.5,
				       TFT_BOTTOM - 8 * 0.5);

		int future_tile_x = 0, future_tile_y = 0;

		if (pacman.dx > 0) {
			// Moving right
			float ideal_y = (roundf(future_y / 8 - 0.5f)) * 8 + 0.5 * 8;
			future_y = future_y * 0.8 + ideal_y * 0.2;
			future_tile_x = (future_x + 0.5 * PACMAN_SIZE) / 8;
			future_tile_y = future_y / 8;
		} else if (pacman.dx < 0) {
			// Moving left
			float ideal_y = (roundf(future_y / 8 - 0.5f)) * 8 + 0.5 * 8;
			future_y = future_y * 0.8 + ideal_y * 0.2;
			future_tile_x = (future_x - 0.5 * PACMAN_SIZE) / 8;
			future_tile_y = future_y / 8;
		} else if (pacman.dy > 0) {
			// Moving down
			float ideal_x = (roundf(future_x / 8 - 0.5f)) * 8 + 0.5 * 8;
			future_x = future_x * 0.8 + ideal_x * 0.2;
			future_tile_y = (future_y + 0.5 * PACMAN_SIZE) / 8;
			future_tile_x = future_x / 8;
		} else if (pacman.dy < 0) {
			// Moving up
			float ideal_x = (roundf(future_x / 8 - 0.5f)) * 8 + 0.5 * 8;
			future_x = future_x * 0.8 + ideal_x * 0.2;
			future_tile_y = (future_y - 0.5 * PACMAN_SIZE) / 8;
			future_tile_x = future_x / 8;
		}

		int future_tile = map[future_tile_y][future_tile_x];

		if (future_tile <= CHERRY) {
			pacman.x = future_x;
			pacman.y = future_y;
		}
	}
}

static void draw_tile(TileType type, int x, int y)
{
	switch (type) {
	case SUGAR:
		tft_draw_rect(x + 3, y + 3, x + 4, y + 4, WHITE);
		break;

	case POWERUP:
		tft_draw_rect(x + 2, y + 2, x + 5, y + 5, WHITE);
		break;

	case CHERRY:
		tft_draw_rect(x + 2, y + 2, x + 5, y + 5, RED);
		break;

	case WALL_H:
		tft_draw_rect(x, y + 1, x + 7, y + 1, BLUE);
		tft_draw_rect(x, y + 6, x + 7, y + 6, BLUE);
		break;

	case WALL_V:
		tft_draw_rect(x + 1, y, x + 1, y + 7, BLUE);
		tft_draw_rect(x + 6, y, x + 6, y + 7, BLUE);
		break;

	case WALL_SQUARE:
		tft_draw_rect(x + 1, y + 1, x + 6, y + 1, BLUE);
		tft_draw_rect(x + 1, y + 6, x + 6, y + 6, BLUE);
		tft_draw_rect(x + 1, y + 1, x + 1, y + 6, BLUE);
		tft_draw_rect(x + 6, y + 1, x + 6, y + 6, BLUE);
		break;

	case WALL_T_E:
		tft_draw_rect(x + 1, y, x + 1, y + 7, BLUE);
		tft_draw_rect(x + 6, y, x + 6, y + 1, BLUE);
		tft_draw_rect(x + 6, y + 1, x + 7, y + 1, BLUE);
		tft_draw_rect(x + 6, y + 6, x + 6, y + 7, BLUE);
		tft_draw_rect(x + 6, y + 6, x + 7, y + 6, BLUE);
		break;

	case WALL_T_N:
		tft_draw_rect(x, y + 6, x + 7, y + 6, BLUE);
		tft_draw_rect(x, y + 1, x + 1, y + 1, BLUE);
		tft_draw_rect(x + 1, y + 1, x + 1, y, BLUE);
		tft_draw_rect(x + 6, y, x + 6, y + 1, BLUE);
		tft_draw_rect(x + 6, y + 1, x + 7, y + 1, BLUE);
		break;

	case WALL_T_S:
		tft_draw_rect(x, y + 1, x + 7, y + 1, BLUE);
		tft_draw_rect(x, y + 6, x + 1, y + 6, BLUE);
		tft_draw_rect(x + 1, y + 6, x + 1, y + 7, BLUE);
		tft_draw_rect(x + 6, y + 6, x + 6, y + 7, BLUE);
		tft_draw_rect(x + 6, y + 6, x + 7, y + 6, BLUE);
		break;

	case WALL_T_W:
		tft_draw_rect(x + 6, y, x + 6, y + 7, BLUE);
		tft_draw_rect(x + 1, y, x + 1, y + 1, BLUE);
		tft_draw_rect(x, y + 1, x + 1, y + 1, BLUE);
		tft_draw_rect(x, y + 6, x + 1, y + 6, BLUE);
		tft_draw_rect(x + 1, y + 6, x + 1, y + 7, BLUE);
		break;

	case EDGE_E:
		tft_draw_rect(x + 6, y + 1, x, y + 1, BLUE);
		tft_draw_rect(x + 6, y + 1, x + 6, y + 6, BLUE);
		tft_draw_rect(x + 6, y + 6, x, y + 6, BLUE);
		break;

	case EDGE_W:
		tft_draw_rect(x + 7, y + 1, x + 1, y + 1, BLUE);
		tft_draw_rect(x + 1, y + 1, x + 1, y + 6, BLUE);
		tft_draw_rect(x + 7, y + 6, x + 1, y + 6, BLUE);
		break;

	case EDGE_N:
		tft_draw_rect(x + 1, y + 1, x + 6, y + 1, BLUE);
		tft_draw_rect(x + 1, y + 1, x + 1, y + 7, BLUE);
		tft_draw_rect(x + 6, y + 1, x + 6, y + 7, BLUE);
		break;

	case EDGE_S:
		tft_draw_rect(x + 1, y + 6, x + 6, y + 6, BLUE);
		tft_draw_rect(x + 1, y, x + 1, y + 6, BLUE);
		tft_draw_rect(x + 6, y, x + 6, y + 6, BLUE);
		break;

	case CORNER_NE:
		tft_draw_rect(x + 1, y, x + 1, y + 6, BLUE);
		tft_draw_rect(x + 1, y + 6, x + 7, y + 6, BLUE);
		tft_draw_rect(x + 6, y, x + 6, y + 1, BLUE);
		tft_draw_rect(x + 6, y + 1, x + 7, y + 1, BLUE);
		break;

	case CORNER_NW:
		tft_draw_rect(x, y + 6, x + 6, y + 6, BLUE);
		tft_draw_rect(x + 6, y + 6, x + 6, y, BLUE);
		tft_draw_rect(x, y + 1, x + 1, y + 1, BLUE);
		tft_draw_rect(x + 1, y + 1, x + 1, y, BLUE);
		break;

	case CORNER_SE:
		tft_draw_rect(x + 1, y + 1, x + 7, y + 1, BLUE);
		tft_draw_rect(x + 1, y + 1, x + 1, y + 7, BLUE);
		tft_draw_rect(x + 6, y + 6, x + 7, y + 6, BLUE);
		tft_draw_rect(x + 6, y + 6, x + 6, y + 7, BLUE);
		break;

	case CORNER_SW:
		tft_draw_rect(x, y + 1, x + 6, y + 1, BLUE);
		tft_draw_rect(x + 6, y + 1, x + 6, y + 7, BLUE);
		tft_draw_rect(x, y + 6, x + 1, y + 6, BLUE);
		tft_draw_rect(x + 1, y + 6, x + 1, y + 7, BLUE);
		break;

	default:
		// :shrug:
		break;
	}
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	for (int x = 0; x < TFT_WIDTH / 8; x++) {
		for (int y = 0; y < TFT_HEIGHT / 8; y++) {
			draw_tile(map[y][x], x * 8, y * 8);
		}
	}
	tft_draw_rect(pacman.x - PACMAN_SIZE * 0.5f, pacman.y - PACMAN_SIZE * 0.5f,
		      pacman.x + PACMAN_SIZE * 0.5f, pacman.y + PACMAN_SIZE * 0.5f, YELLOW);
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
