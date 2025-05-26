#include <pico/stdlib.h>
#include <math.h>

#include <sdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <tft.h>

#include "level.h"

#include <tileset-16x16.png.h>
#include <walls-32x64.png.h>
#include <floors-32x32.png.h>
#include <player-32x32.png.h>
#include <enemies-32x32.png.h>

#include <cover.png.h>

sdk_game_info("deep", &image_cover_png);

#define GRAY rgb_to_rgb565(127, 127, 127)

typedef enum Orientation {
	NORTH,
	SOUTH,
	EAST,
	WEST,
} Orientation;

typedef struct Frame {
	uint32_t next : 8;  /* Frame after this one */
	uint32_t tile : 8;  /* Tile to show */
	uint32_t flip : 1;  /* Mirror tile along X */
	uint32_t msec : 10; /* Milliseconds to dwell */
	uint32_t tag : 5;   /* Arbitrary value */
} Frame;

static_assert(sizeof(Frame) == 4);

typedef struct Animation {
	const Frame *frames;
	const Frame *frame;
	float timeout;
} Animation;

#define TAG_ATTACKING 1
#define TAG_WALKING 2

static const Frame player_frames[] = {
	/* Idle */
	[0x00] = { 0x00, 15, 0, 100, 0 },

	/* Idle Up */
	[0x01] = { 0x01, 0, 0, 100, 0 },

	/* Idle Down */
	[0x02] = { 0x02, 2, 0, 100, 0 },

	/* Idle Right */
	[0x03] = { 0x03, 5, 0, 100, 0 },

	/* Idle Left */
	[0x04] = { 0x04, 5, 1, 100, 0 },

	/* Walk Up Loop */
	[0x10] = { 0x11, 0, 0, 150, TAG_WALKING },
	[0x11] = { 0x10, 1, 0, 150, TAG_WALKING },

	/* Walk Down Loop */
	[0x18] = { 0x19, 2, 0, 150, TAG_WALKING },
	[0x19] = { 0x18, 3, 0, 150, TAG_WALKING },

	/* Walk Right Loop */
	[0x20] = { 0x21, 4, 0, 125, TAG_WALKING },
	[0x21] = { 0x22, 5, 0, 125, TAG_WALKING },
	[0x22] = { 0x23, 6, 0, 125, TAG_WALKING },
	[0x23] = { 0x20, 7, 0, 125, TAG_WALKING },

	/* Walk Left Loop */
	[0x28] = { 0x29, 4, 1, 125, TAG_WALKING },
	[0x29] = { 0x2a, 5, 1, 125, TAG_WALKING },
	[0x2a] = { 0x2b, 6, 1, 125, TAG_WALKING },
	[0x2b] = { 0x28, 7, 1, 125, TAG_WALKING },

	/* Attack Up */
	[0x30] = { 0x31, 8, 0, 50, TAG_ATTACKING },
	[0x31] = { 0x32, 9, 0, 50, TAG_ATTACKING },
	[0x32] = { 0x33, 10, 0, 100, TAG_ATTACKING },
	[0x33] = { 0x33, 11, 0, 100, 0 },

	/* Attack Down */
	[0x38] = { 0x39, 12, 0, 50, TAG_ATTACKING },
	[0x39] = { 0x3a, 13, 0, 50, TAG_ATTACKING },
	[0x3a] = { 0x3b, 14, 0, 100, TAG_ATTACKING },
	[0x3b] = { 0x3b, 15, 0, 100, 0 },

	/* Attack Right */
	[0x40] = { 0x41, 16, 0, 50, TAG_ATTACKING },
	[0x41] = { 0x42, 17, 0, 50, TAG_ATTACKING },
	[0x42] = { 0x43, 18, 0, 100, TAG_ATTACKING },
	[0x43] = { 0x43, 19, 0, 100, 0 },

	/* Attack Left */
	[0x48] = { 0x49, 20, 0, 50, TAG_ATTACKING },
	[0x49] = { 0x4a, 21, 0, 50, TAG_ATTACKING },
	[0x4a] = { 0x4b, 22, 0, 100, TAG_ATTACKING },
	[0x4b] = { 0x4b, 23, 0, 100, 0 },
};

static int player_walk[] = {
	[NORTH] = 0x10,
	[SOUTH] = 0x18,
	[EAST] = 0x20,
	[WEST] = 0x28,
};

static int player_attacks[] = {
	[NORTH] = 0x30,
	[SOUTH] = 0x38,
	[EAST] = 0x40,
	[WEST] = 0x48,
};

static void set_frame(Animation *a, sdk_sprite_t *s, uint8_t frame_id)
{
	a->frame = &a->frames[frame_id];
	a->timeout = a->frame->msec * 1e-3;
	s->tile = a->frame->tile;
	s->flip_x = a->frame->flip;
}

static void animate(Animation *a, sdk_sprite_t *s, float dt)
{
	a->timeout -= dt;

	if (a->timeout > 0)
		return;

	set_frame(a, s, a->frame->next);
}

typedef struct Character {
	sdk_sprite_t s;
	Animation a;
	Orientation o;
	float speed;
	float x, y;
} Character;

static Character player = {
	.s = {
		.ts = &ts_player_32x32_png,
		.ox = 15.5f,
		.oy = 29.0f,
		.x = TFT_WIDTH / 2.0f,
		.y = TFT_HEIGHT / 2.0f,
	},
	.a = {
		.frames = player_frames,
		.frame = player_frames,
	},
	.o = SOUTH,
};

static Level level;

static void change_floor(void)
{
	level.seed = 0x1337 + level.floor - 1;

	uint32_t t0 = time_us_32();
	level_generate(&level);
	uint32_t t1 = time_us_32();

	printf("level generated in %u us\n", (unsigned)(t1 - t0));

	player.x = level.sx * TILE_SIZE + 0.5f * TILE_SIZE;
	player.y = level.sy * TILE_SIZE + 0.5f * TILE_SIZE;

	set_frame(&player.a, &player.s, 0x00);
}

void game_reset(void)
{
	level.floor = 1;
	change_floor();

	player.speed = 50.0f;
	player.s.tile = 0;
}

static void move_and_slide(Character *ch, float dx, float dy)
{
	float nx = ch->x + dx;
	float ny = ch->y + dy;

	int tx = ch->x / TILE_SIZE;
	int ty = ch->y / TILE_SIZE;

	int tnx = nx / TILE_SIZE;
	int tny = ny / TILE_SIZE;

	bool x_solid = level.map[ty][tnx].solid;
	bool y_solid = level.map[tny][tx].solid;
	bool xy_solid = level.map[tny][tnx].solid;

	if (!xy_solid && !x_solid && !y_solid) {
		ch->x = clamp(ch->x + dx, 0, MAP_SIZE * TILE_SIZE - 1e-9f);
		ch->y = clamp(ch->y + dy, 0, MAP_SIZE * TILE_SIZE - 1e-9f);
	} else if (!x_solid) {
		ch->x = clamp(ch->x + dx, 0, MAP_SIZE * TILE_SIZE - 1e-9f);
	} else if (!y_solid) {
		ch->y = clamp(ch->y + dy, 0, MAP_SIZE * TILE_SIZE - 1e-9f);
	}
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	if (sdk_inputs.start) {
		player.speed = 100.0f;
	} else {
		player.speed = 50.0f;
	}

	if (sdk_inputs_delta.a > 0 && !(player.a.frame->tag & TAG_ATTACKING)) {
		set_frame(&player.a, &player.s, player_attacks[player.o]);
	}

	float jx = sdk_inputs.jx + sdk_inputs.jy;
	float jy = sdk_inputs.jx - sdk_inputs.jy;

	float move_x = fabsf(jx) >= 0.25f ? player.speed * jx : 0;
	float move_y = fabsf(jy) >= 0.25f ? player.speed * jy : 0;

	if ((move_x || move_y)) {
		Orientation po = player.o;

		if (fabsf(sdk_inputs.jx) > fabsf(sdk_inputs.jy)) {
			player.o = sdk_inputs.jx < 0 ? WEST : EAST;
		} else {
			player.o = sdk_inputs.jy < 0 ? NORTH : SOUTH;
		}

		if (player.a.frame->tag & TAG_ATTACKING) {
			move_and_slide(&player, move_x * dt * 0.25f, move_y * dt * 0.25f);
		} else {
			move_and_slide(&player, move_x * dt, move_y * dt);
		}

		if (player.o != po) {
			if (player.a.frame->tag & TAG_ATTACKING) {
				set_frame(&player.a, &player.s, player_attacks[player.o]);
			} else if (!(player.a.frame->tag & TAG_WALKING)) {
				set_frame(&player.a, &player.s, player_walk[player.o]);
			}
		}
	}

	int pos_x = player.x / TILE_SIZE;
	int pos_y = player.y / TILE_SIZE;

	if (pos_x == level.ex && pos_y == level.ey) {
		level.floor++;
		change_floor();
	}

	animate(&player.a, &player.s, dt);
}

static uint32_t xorshift_seed = 0x1337;

static inline uint32_t xorshift(void)
{
	uint32_t x = xorshift_seed;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	xorshift_seed = x;
	return x;
}

static inline uint32_t xorshift_bits(unsigned bits)
{
	return xorshift() >> (32 - bits);
}

static inline int estimate_distance(int x0, int y0, int x1, int y1)
{
	int x = abs(x1 - x0);
	int y = abs(y1 - y0);

	int taxicab = x + y;
	int chebyshev = MAX(x, y);
	return (taxicab * 7 + chebyshev * 9) >> 4;
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(rgb_to_rgb565(0, 0, 0));

	int pos_x = player.x / TILE_SIZE;
	int pos_y = player.y / TILE_SIZE;

	tft_set_origin((player.x + player.y) - TFT_WIDTH / 2.0f,
		       (player.x - player.y) - TFT_HEIGHT / 2.0f);

	int y0 = clamp(pos_y - TFT_HEIGHT / 2 / TILE_SIZE - 2, 0, MAP_MAX);
	int y1 = clamp(pos_y + TFT_HEIGHT / 2 / TILE_SIZE + 2, 0, MAP_MAX);
	int x0 = clamp(pos_x - TFT_WIDTH / 2 / TILE_SIZE - 0, 0, MAP_MAX);
	int x1 = clamp(pos_x + TFT_WIDTH / 2 / TILE_SIZE + 0, 0, MAP_MAX);

	for (int y = y1; y >= y0; y--) {
		for (int x = x0; x <= x1; x++) {
			if (level.map[y][x].solid)
				continue;

			sdk_draw_tile((x + y) * TILE_SIZE, (x - y) * TILE_SIZE - TILE_SIZE,
				      &ts_floors_32x32_png, level.map[y][x].tile_id);
		}
	}

	uint32_t now = time_us_32();
	uint32_t enemy_frame = (now >> 18) & 3;

	for (int y = y1; y >= y0; y--) {
		for (int x = x0; x <= x1; x++) {
			if (pos_y == y && pos_x == x) {
				tft_set_origin(0, 0);
				sdk_draw_sprite(&player.s);
				tft_set_origin((player.x + player.y) - TFT_WIDTH / 2.0f,
					       (player.x - player.y) - TFT_HEIGHT / 2.0f);
			}

			if (level.map[y][x].enemy_id) {
				sdk_draw_tile((x + y) * TILE_SIZE, (x - y) * TILE_SIZE - TILE_SIZE,
					      &ts_enemies_32x32_png,
					      4 * (level.map[y][x].enemy_id - 1) + enemy_frame);
			}

			if (level.map[y][x].solid)
				sdk_draw_tile((x + y) * TILE_SIZE,
					      (x - y) * TILE_SIZE - 3 * TILE_SIZE,
					      &ts_walls_32x64_png, level.map[y][x].tile_id);
		}
	}

#if 1
	static int avg_range = 40 << 8;
	int range = (46 << 8) + xorshift_bits(4 + 8);
	avg_range += (range - avg_range) >> 3;
	range = avg_range >> 8;

	for (int x = 0; x < TFT_WIDTH; x++) {
		for (int y = 0; y < TFT_HEIGHT; y++) {
			int dist = estimate_distance(TFT_WIDTH / 2, TFT_HEIGHT / 2, x, y);
			dist += xorshift_bits(2);

			if (dist > range) {
				dist -= range;
				int mag = MAX(0, 32 - ((dist * dist) >> 6));
				tft_input[x][y] = color_mul(tft_input[x][y], mag);
			}
		}
	}
#endif

	tft_set_origin(0, 0);

#if 0
	tft_draw_string(0, 0, rgb_to_rgb565(127, 0, 0), "%i. %.0f,%.0f\n", level.floor, player.x,
			player.y);
#endif
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
