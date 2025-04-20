#include <pico/stdlib.h>

#include <stdlib.h>
#include <math.h>

#include <sdk.h>
#include <tft.h>
#include <stdio.h>

#include <pacman.png.h>
#include <tiles.png.h>
#include <ghost.png.h>
#include <ghost-red.png.h>
#include <ghost-orange.png.h>
#include <ghost-blue.png.h>
#include <ghost-pink.png.h>

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

#define BLUE rgb_to_rgb565(0, 0, 255)
#define GRAY rgb_to_rgb565(127, 127, 127)
#define WHITE rgb_to_rgb565(255, 255, 255)

#define PACMAN_SIZE 7

#define GHOST_BLUE_SIZE 7

struct pacman {
	float dx, dy;
	float fdx, fdy;
	uint8_t color;
	float speed;
	sdk_sprite_t s;
	int score;
};

struct ghost_blue {
	float gbdx, gbdy;
	float fgbdx, fgbdy;
	float speed;
	sdk_sprite_t s;
};

static struct pacman pacman;

static struct ghost_blue ghost_blue;

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
	pacman.speed = 32;
	pacman.s = (sdk_sprite_t){
		.ts = &ts_pacman_png,
		.ox = 3.5,
		.oy = 3.5,
		.tile = 0,
	};

	ghost_blue.speed = 32;
	ghost_blue.s = (sdk_sprite_t){
		.ts = &ts_ghost_png,
		.ox = 3.5,
		.oy = 3.5,
		.tile = 0,
	};
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

		sdk_write_sample(sample, sample);
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
		e->period = frequency ? SDK_AUDIO_RATE / frequency : 1;
		e->generator = gen;

		break;
	}
}

static void new_round(void)
{
	pacman.s.x = 15.5 * 8;
	pacman.s.y = 6.5 * 8;
	ghost_blue.s.x = 10.5 * 8;
	ghost_blue.s.y = 6.5 * 8;
}

void game_reset(void)
{
	new_round();
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	if (sdk_inputs_delta.a > 0 || sdk_inputs.joy_y > 300) {
		if (!pacman.dx) {
			pacman.dy = 1;
			ghost_blue.gbdy = 1;
		} else {
			pacman.fdy = 1, pacman.fdx = 0;
			ghost_blue.fgbdy = 1, ghost_blue.fgbdx = 0;
		}
	} else if (sdk_inputs_delta.y > 0 || sdk_inputs.joy_y < -300) {
		if (!pacman.dx) {
			pacman.dy = -1;
			ghost_blue.gbdy = -1;
		} else {
			pacman.fdy = -1, pacman.fdx = 0;
			ghost_blue.fgbdy = -1, ghost_blue.fgbdx = 0;
		}
	} else if (sdk_inputs_delta.b > 0 || sdk_inputs.joy_x > 300) {
		if (!pacman.dy) {
			pacman.dx = 1;
			ghost_blue.gbdx = 1;
		} else {
			pacman.fdx = 1, pacman.fdy = 0;
			ghost_blue.fgbdx = 1, ghost_blue.fgbdy = 0;
		}
	} else if (sdk_inputs_delta.x > 0 || sdk_inputs.joy_x < -300) {
		if (!pacman.dy) {
			pacman.dx = -1;
			ghost_blue.gbdx = -1;
		} else {
			pacman.fdx = -1, pacman.fdy = 0;
			ghost_blue.fgbdx = -1, ghost_blue.fgbdy = 0;
		}
	}

	float dx = fabsf(fmodf(pacman.s.x, 8) / 8 - 0.5f);
	float dy = fabsf(fmodf(pacman.s.y, 8) / 8 - 0.5f);
	float d = MAX(dx, dy);

	if (pacman.fdx || pacman.fdy) {
		if (d < (2 * pacman.speed / (30 * 8))) {
			int hx = pacman.s.x / 8 + pacman.fdx;
			int hy = pacman.s.y / 8 + pacman.fdy;

			if (hx >= 0 && hx < 20 && hy >= 0 && hy < 15 && map[hy][hx] <= CHERRY) {
				pacman.dx = pacman.fdx;
				pacman.dy = pacman.fdy;
				pacman.fdx = 0;
				pacman.fdy = 0;
			}
		}
	}

	bool moved = false;

	if (pacman.dy || pacman.dx) {
		float future_x = clamp(pacman.s.x + pacman.dx * dt * pacman.speed, 8 * 0.5,
				       TFT_RIGHT - 8 * 0.5);
		float future_y = clamp(pacman.s.y + pacman.dy * dt * pacman.speed, 8 * 0.5,
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
			moved = true;
			pacman.s.x = future_x;
			pacman.s.y = future_y;
		}

		if (ghost_blue.gbdy || ghost_blue.gbdx) {
			float future_gbx =
				clamp(ghost_blue.s.x + ghost_blue.gbdx * dt * ghost_blue.speed,
				      8 * 0.5, TFT_RIGHT - 8 * 0.5);
			float future_gby =
				clamp(ghost_blue.s.y + ghost_blue.gbdy * dt * ghost_blue.speed,
				      8 * 0.5, TFT_BOTTOM - 8 * 0.5);

			int future_tile_gbx = 0, future_tile_gby = 0;

			if (ghost_blue.gbdx > 0) {
				// Moving right
				float ideal_gby = (roundf(future_gby / 8 - 0.5f)) * 8 + 0.5 * 8;
				future_gby = future_gby * 0.8 + ideal_gby * 0.2;
				future_tile_gbx = (future_gbx + 0.5 * PACMAN_SIZE) / 8;
				future_tile_gby = future_gby / 8;
			} else if (ghost_blue.gbdx < 0) {
				// Moving left
				float ideal_gby = (roundf(future_gby / 8 - 0.5f)) * 8 + 0.5 * 8;
				future_gby = future_gby * 0.8 + ideal_gby * 0.2;
				future_tile_gbx = (future_gbx - 0.5 * PACMAN_SIZE) / 8;
				future_tile_gby = future_gby / 8;
			} else if (ghost_blue.gbdy > 0) {
				// Moving down
				float ideal_gbx = (roundf(future_gbx / 8 - 0.5f)) * 8 + 0.5 * 8;
				future_gbx = future_gbx * 0.8 + ideal_gbx * 0.2;
				future_tile_gby = (future_gby + 0.5 * PACMAN_SIZE) / 8;
				future_tile_gbx = future_gbx / 8;
			} else if (ghost_blue.gbdy < 0) {
				// Moving up
				float ideal_gbx = (roundf(future_gbx / 8 - 0.5f)) * 8 + 0.5 * 8;
				future_x = future_gbx * 0.8 + ideal_gbx * 0.2;
				future_tile_gby = (future_gby - 0.5 * PACMAN_SIZE) / 8;
				future_tile_gbx = future_gbx / 8;
			}

			int future_tile_gb = map[future_tile_gby][future_tile_gbx];

			if (future_tile_gb <= CHERRY || future_tile_gb == INVISIBLE_WALL) {
				moved = true;
				ghost_blue.s.x = future_gbx;
				ghost_blue.s.y = future_gby;
			}
		}

		if (pacman.dx > 0) {
			pacman.s.angle = 0;
		} else if (pacman.dx < 0) {
			pacman.s.angle = 2;
		} else if (pacman.dy > 0) {
			pacman.s.angle = 1;
		} else if (pacman.dy < 0) {
			pacman.s.angle = 3;
		}

		if (moved) {
			uint32_t now = time_us_32();
			pacman.s.tile = (now >> 16) & 1;
		} else {
			pacman.s.tile = 0;
		}

		if (d < 0.25) {
			int x = pacman.s.x / 8;
			int y = pacman.s.y / 8;

			int eaten = map[y][x];
			map[y][x] = 0;

			if (eaten == 1) {
				pacman.score += 10;
			} else if (eaten == 2) {
				pacman.score += 0;
			} else if (eaten == 3) {
				pacman.score += 50;
			} else if (eaten > 3) {
				pacman.score -= 10000;
			}
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
		sdk_draw_tile(x, y, &ts_tiles_png, 0);
		break;

	case WALL_H:
		sdk_draw_tile(x, y, &ts_tiles_png, 2);
		break;

	case WALL_V:
		sdk_draw_tile(x, y, &ts_tiles_png, 3);
		break;

	case WALL_SQUARE:
		sdk_draw_tile(x, y, &ts_tiles_png, 1);
		break;

	case WALL_T_E:
		sdk_draw_tile(x, y, &ts_tiles_png, 7);
		break;

	case WALL_T_N:
		sdk_draw_tile(x, y, &ts_tiles_png, 4);
		break;

	case WALL_T_S:
		sdk_draw_tile(x, y, &ts_tiles_png, 6);
		break;

	case WALL_T_W:
		sdk_draw_tile(x, y, &ts_tiles_png, 5);
		break;

	case EDGE_E:
		sdk_draw_tile(x, y, &ts_tiles_png, 15);
		break;

	case EDGE_W:
		sdk_draw_tile(x, y, &ts_tiles_png, 13);
		break;

	case EDGE_N:
		sdk_draw_tile(x, y, &ts_tiles_png, 12);
		break;

	case EDGE_S:
		sdk_draw_tile(x, y, &ts_tiles_png, 14);
		break;

	case CORNER_NE:
		sdk_draw_tile(x, y, &ts_tiles_png, 11);
		break;

	case CORNER_NW:
		sdk_draw_tile(x, y, &ts_tiles_png, 8);
		break;

	case CORNER_SE:
		sdk_draw_tile(x, y, &ts_tiles_png, 9);
		break;

	case CORNER_SW:
		sdk_draw_tile(x, y, &ts_tiles_png, 10);
		break;

	case INVISIBLE_WALL:
		sdk_draw_tile(x, y, &ts_tiles_png, 16);
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

	sdk_draw_sprite(&pacman.s);
	char buf[16];
	snprintf(buf, sizeof buf, "%i", pacman.score);
	tft_draw_string(0 + 10, TFT_BOTTOM - 16, BLUE, buf);

	sdk_draw_sprite(&ghost_blue.s);
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = false,
		.off_on_select = true,
		.fps_color = GRAY,
	};

	printf("%d", 8);
	sdk_main(&config);
}
