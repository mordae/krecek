#include <pico/stdlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <sdk.h>
#include <tft.h>

typedef enum {
	EMPTY = 0,
	SUGAR = 1,
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

} TileType;

TileType map[15][20] = {
	{ 13, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 12 },
	{ 5, 2, 1, 1, 1, 5, 1, 1, 1, 1, 1, 1, 1, 1, 5, 1, 1, 1, 2, 5 },
	{ 5, 1, 13, 17, 1, 16, 1, 15, 4, 4, 4, 4, 17, 1, 16, 1, 15, 12, 1, 5 },
	{ 5, 1, 5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 1, 5 },
	{ 5, 1, 16, 1, 15, 17, 1, 13, 17, 0, 0, 15, 12, 1, 15, 17, 1, 16, 1, 5 },
	{ 5, 1, 1, 1, 1, 1, 1, 5, 0, 0, 0, 0, 5, 1, 1, 1, 1, 1, 1, 5 },
	{ 9, 4, 4, 17, 1, 3, 1, 5, 0, 0, 0, 0, 5, 1, 3, 1, 15, 4, 4, 7 },
	{ 5, 1, 1, 1, 1, 1, 1, 5, 0, 0, 0, 0, 5, 1, 1, 1, 1, 1, 1, 5 },
	{ 5, 1, 14, 1, 15, 17, 1, 11, 4, 4, 4, 4, 10, 1, 15, 17, 1, 14, 1, 5 },
	{ 5, 1, 5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 1, 5 },
	{ 5, 1, 11, 17, 1, 14, 1, 15, 4, 4, 4, 4, 17, 1, 14, 1, 15, 10, 1, 5 },
	{ 5, 2, 1, 1, 1, 5, 1, 1, 1, 1, 1, 1, 1, 1, 5, 1, 1, 1, 2, 5 },
	{ 11, 4, 4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 4, 6, 4, 4, 4, 4, 10 },
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
	sdk_set_output_gain_db(6);
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
}

void game_reset(void)
{
	new_round();
}

void game_input(unsigned dt_usec)
{
}

static void draw_tile(TileType type, int x, int y)
{
	switch (type) {
	case SUGAR:
		tft_draw_rect(x + 3, y + 3, x + 4, y + 4, WHITE);
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
