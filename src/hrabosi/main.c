#include <pico/stdlib.h>

#include <sdk.h>
#include <tft.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define multiply332(x, f) \
	rgb_to_rgb332(rgb332_red((x)) * f, rgb332_green((x)) * f, rgb332_blue((x)) * f)

#define RED rgb332(7, 0, 0)
#define GREEN rgb332(0, 7, 0)
#define BLUE rgb332(0, 0, 3)

#define YELLOW rgb332(7, 7, 0)
#define ORANGE rgb332(7, 5, 0)
#define BROWN rgb332(7, 3, 0)

#define WHITE rgb332(7, 7, 3)
#define LGRAY rgb332(5, 5, 2)
#define DGRAY rgb332(2, 2, 1)
#define BLACK rgb332(0, 0, 0)

#define GRID_WIDTH (640 >> 2)
#define GRID_HEIGHT (480 >> 2)
#define GRID_RIGHT (GRID_WIDTH - 1)
#define GRID_BOTTOM (GRID_HEIGHT - 1)

#define CELL_SCALE 2
#define WORLD_WIDTH (GRID_WIDTH * CELL_SCALE)
#define WORLD_HEIGHT (GRID_HEIGHT * CELL_SCALE)

#define WORLD_RIGHT (WORLD_WIDTH - 1)
#define WORLD_BOTTOM (WORLD_HEIGHT - 1)

static uint32_t grid[WORLD_HEIGHT][WORLD_WIDTH / 32];

#define SQRT_MAX_1 22
#define SQRT_MAX_2 16
static uint8_t sqrt_table[2 * SQRT_MAX_2 * SQRT_MAX_2];

#define NUM_INITIAL_HOLES 128
#define HOLE_MAX_RADIUS SQRT_MAX_2

embed_tileset(ts_green_tank, 8, 16, 16, BLACK, "green_tank.data");
embed_tileset(ts_blue_tank, 8, 16, 16, BLACK, "blue_tank.data");
embed_tileset(ts_bullet, 8, 5, 5, BLACK, "bullet.data");

#define NUM_BULLETS 6
#define BULLET_SPEED (TANK_SPEED * 2.0f)

struct bullet {
	sdk_sprite_t s;
	float dx, dy;
	bool spawned;
};

struct tank {
	sdk_sprite_t s;
	float speed;
	int cooldown;
	struct bullet bullets[NUM_BULLETS];
};

inline static int dirt_color(int wx, int wy);
inline static bool has_dirt(int wx, int wy);
static int poke_hole(int gx, int gy, int radius);
static int poke_sprite(const sdk_sprite_t *s);
inline static int8_t get_angle(float rx, float ry);
static void move_and_slide(float wx, float wy, struct tank *t1, const struct tank *t2);
static float angle_to_x(int angle);
static float angle_to_y(int angle);

static void spawn_bullet(struct tank *t);
static void paint_bullets(void);
static void paint_dirt(int wx, int wy, int w, int h);
static void explode(int gx, int gy, int size);

#define TANK_SIZE 16
#define TANK_SPEED 66.0f
#define TANK_COOLDOWN (100 * 1000)

static struct tank tank1 = {
	.s = {
		.ox = 8.0f,
		.oy = 8.0f,
		.ts = &ts_green_tank,
	},
	.speed = TANK_SPEED,
	.bullets = {{ .s = { .ts = &ts_bullet, .ox = 2.5f, .oy = 2.5f, } }},
};
static struct tank tank2 = {
	.s = {
		.ox = 8.0f,
		.oy = 8.0f,
		.ts = &ts_blue_tank,
	},
	.speed = TANK_SPEED,
	.bullets = {{ .s = { .ts = &ts_bullet, .ox = 2.5f, .oy = 2.5f, } }},
};

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = false,
		.fps_color = DGRAY,
	};

	sdk_main(&config);
}

void game_start(void)
{
	/* Pre-calculate square roots. */
	for (unsigned i = 0; i < sizeof(sqrt_table); i++)
		sqrt_table[i] = roundf(sqrtf(i));

	/* Use RGB332 palette arrangement. */
	for (int i = 0; i < 256; i++)
		tft_palette[i] = rgb332_to_rgb565(i);

	/* Copy bullet template to other slots. */
	for (int i = 1; i < NUM_BULLETS; i++) {
		tank1.bullets[i] = tank1.bullets[0];
		tank2.bullets[i] = tank2.bullets[0];
	}
}

void game_reset(void)
{
	memset(grid, 0xff, sizeof grid);

	for (int i = 0; i < NUM_INITIAL_HOLES; i++) {
		poke_hole(rand() % (GRID_WIDTH + HOLE_MAX_RADIUS * 2) - HOLE_MAX_RADIUS,
			  rand() % (GRID_HEIGHT + HOLE_MAX_RADIUS * 2) - HOLE_MAX_RADIUS,
			  rand() % 10 + 2);
	}

	tank1.s.x = (int)((rand() % (WORLD_WIDTH * 6 / 8)) + WORLD_WIDTH / 8);
	tank1.s.y = (int)((rand() % (WORLD_HEIGHT * 6 / 8)) + WORLD_HEIGHT / 8);
	tank1.speed = TANK_SPEED;
	tank1.cooldown = 0;

	tank2.s.x = tank1.s.x;
	tank2.s.y = tank1.s.y;
	tank2.speed = TANK_SPEED;
	tank2.cooldown = 0;

	while ((fabsf(tank2.s.x - tank1.s.x) < WORLD_WIDTH / 4.0f) &&
	       (fabsf(tank2.s.y - tank1.s.y) < WORLD_HEIGHT / 4.0f)) {
		tank2.s.x = (int)((rand() % (WORLD_WIDTH * 6 / 8)) + WORLD_WIDTH / 8);
		tank2.s.y = (int)((rand() % (WORLD_HEIGHT * 6 / 8)) + WORLD_HEIGHT / 8);
	}

	poke_hole(tank1.s.x / CELL_SCALE, tank1.s.y / CELL_SCALE, 12);
	poke_hole(tank2.s.x / CELL_SCALE, tank2.s.y / CELL_SCALE, 12);
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0;

	{
		sdk_inputs.joy_x = clamp(sdk_inputs.joy_x, -2047, 2047);
		sdk_inputs.joy_y = clamp(sdk_inputs.joy_y, -2047, 2047);

		float joy_x = dt * sdk_inputs.joy_x / 2047.0f;
		float joy_y = dt * sdk_inputs.joy_y / 2047.0f;

		float wx = tank1.s.x;
		float wy = tank1.s.y;

		tank1.speed = clamp(tank1.speed + TANK_SPEED * dt, 0, TANK_SPEED);

		int cells = poke_sprite(&tank1.s);
		tank1.speed -= cells * TANK_SPEED / 10;
		tank1.speed = clamp(tank1.speed * (cells ? 1.0f : 1.2f), 0, TANK_SPEED);

		if (abs(sdk_inputs.joy_x) > 200)
			wx = clamp(wx + joy_x * tank1.speed, TANK_SIZE / 2.0f,
				   WORLD_RIGHT - TANK_SIZE / 2.0f);

		if (abs(sdk_inputs.joy_y) > 200)
			wy = clamp(wy + joy_y * tank1.speed, TANK_SIZE / 2.0f,
				   WORLD_BOTTOM - TANK_SIZE / 2.0f);

		int angle = get_angle(wx - tank1.s.x, wy - tank1.s.y);
		if (angle >= 0)
			tank1.s.tile = angle;

		move_and_slide(wx, wy, &tank1, &tank2);
	}

	{
		float joy_x = dt * (sdk_inputs.b - sdk_inputs.x);
		float joy_y = dt * (sdk_inputs.a - sdk_inputs.y);

		float wx = tank2.s.x;
		float wy = tank2.s.y;

		tank2.speed = clamp(tank2.speed + TANK_SPEED * dt, 0, TANK_SPEED);

		int cells = poke_sprite(&tank2.s);
		tank2.speed -= cells * TANK_SPEED / 10;
		tank2.speed = clamp(tank2.speed * (cells ? 1.0f : 1.2f), 0, TANK_SPEED);

		wx = clamp(wx + joy_x * tank2.speed, TANK_SIZE / 2.0f,
			   WORLD_RIGHT - TANK_SIZE / 2.0f);
		wy = clamp(wy + joy_y * tank2.speed, TANK_SIZE / 2.0f,
			   WORLD_BOTTOM - TANK_SIZE / 2.0f);

		int angle = get_angle(wx - tank2.s.x, wy - tank2.s.y);
		if (angle >= 0)
			tank2.s.tile = angle;

		move_and_slide(wx, wy, &tank2, &tank1);
	}

	for (int i = 0; i < NUM_BULLETS; i++) {
		struct bullet *b1 = &tank1.bullets[i];
		struct bullet *b2 = &tank2.bullets[i];

		if (b1->spawned) {
			b1->s.x += b1->dx * dt;
			b1->s.y += b1->dy * dt;

			float b1x = b1->s.x - b1->s.ts->w / 2.0f;
			float b1y = b1->s.y - b1->s.ts->h / 2.0f;

			if (has_dirt(b1x, b1y)) {
				explode(b1x / CELL_SCALE, b1y / CELL_SCALE, 4);
				//poke_sprite(&b1->s);
				b1->spawned = false;
			} else if (b1x < 0 || b1x > WORLD_RIGHT || b1y < 0 || b1y > WORLD_BOTTOM) {
				b1->spawned = false;
			}
		}

		if (b2->spawned) {
			b2->s.x += b2->dx * dt;
			b2->s.y += b2->dy * dt;

			float b2x = b2->s.x - b2->s.ts->w / 2.0f;
			float b2y = b2->s.y - b2->s.ts->h / 2.0f;

			if (has_dirt(b2->s.x, b2->s.y)) {
				explode(b2x / CELL_SCALE, b2y / CELL_SCALE, 4);
				//poke_sprite(&b2->s);
				b2->spawned = false;
			} else if (b2x < 0 || b2x > WORLD_RIGHT || b2y < 0 || b2y > WORLD_BOTTOM) {
				b2->spawned = false;
			}
		}
	}

	tank1.cooldown = clampi(tank1.cooldown - dt_usec, 0, TANK_COOLDOWN);
	tank2.cooldown = clampi(tank2.cooldown - dt_usec, 0, TANK_COOLDOWN);

	if (sdk_inputs.select)
		spawn_bullet(&tank1);

	if (sdk_inputs.vol_down)
		spawn_bullet(&tank2);
}

void game_paint(unsigned __unused dt_usec)
{
	const int pane_height = TFT_HEIGHT;
	const int pane_width = TFT_WIDTH / 2 - 1;
	const int pane_gap = 2;

	int t1x = tank1.s.x;
	int t1y = tank1.s.y;

	int t2x = tank2.s.x;
	int t2y = tank2.s.y;

	tft_draw_rect(pane_width, 0, pane_width + pane_gap, pane_height, DGRAY);
	tft_draw_rect(TFT_RIGHT, 0, TFT_RIGHT - 50, 50, BLACK);

	{
		int wx = clamp(t1x - pane_width * 1 / 2, 0, WORLD_RIGHT - pane_width);
		int wy = clamp(t1y - pane_height * 2 / 5, 0, WORLD_BOTTOM - pane_height);

		tft_set_clip(0, 0, pane_width, pane_height);
		tft_set_origin(wx, wy);

		paint_dirt(wx, wy, pane_width, pane_height);

		paint_bullets();
		sdk_draw_sprite(&tank2.s);
		sdk_draw_sprite(&tank1.s);

		tft_set_origin(0, 0);
		tft_clear_clip();
	}

	{
		int wx = clamp(t2x - pane_width * 1 / 2, 0, WORLD_RIGHT - pane_width);
		int wy = clamp(t2y - pane_height * 2 / 5, 0, WORLD_BOTTOM - pane_height);

		tft_set_clip(pane_width + pane_gap, 0, TFT_WIDTH, pane_height);
		tft_set_origin(wx - pane_gap - pane_width, wy);

		paint_dirt(wx, wy, pane_width, pane_height);

		paint_bullets();
		sdk_draw_sprite(&tank1.s);
		sdk_draw_sprite(&tank2.s);

		tft_set_origin(0, 0);
		tft_clear_clip();
	}
}

static void paint_bullets(void)
{
	for (int i = 0; i < NUM_BULLETS; i++) {
		const struct bullet *b1 = &tank1.bullets[i];
		const struct bullet *b2 = &tank2.bullets[i];

		if (b1->spawned)
			sdk_draw_sprite(&b1->s);

		if (b2->spawned)
			sdk_draw_sprite(&b2->s);
	}
}

static void spawn_bullet(struct tank *t)
{
	if (t->cooldown > 0)
		return;

	struct bullet *b = NULL;

	for (int i = 0; i < NUM_BULLETS; i++) {
		if (!t->bullets[i].spawned) {
			b = &t->bullets[i];
			break;
		}
	}

	if (NULL == b)
		return;

	t->cooldown = TANK_COOLDOWN;
	b->spawned = true;
	b->s.x = t->s.x;
	b->s.y = t->s.y;
	b->dx = angle_to_x(t->s.tile) * BULLET_SPEED;
	b->dy = angle_to_y(t->s.tile) * BULLET_SPEED;
	b->s.tile = t->s.tile;

	if (t->s.tile == 0 || t->s.tile == 4)
		b->s.x -= 0.5f;

	if (t->s.tile == 2 || t->s.tile == 6)
		b->s.y -= 0.5f;
}

static void paint_dirt(int wx, int wy, int w, int h)
{
	for (int y = wy; y < wy + h; y++) {
		sdk_yield_every_us(2000);

		for (int x = wx; x < wx + w; x++) {
			bool dirt = has_dirt(x, y);
			tft_draw_pixel(x, y, dirt ? dirt_color(x, y) : BLACK);
		}
	}
}

inline static int dirt_color(int wx, int wy)
{
	int xy = wy * WORLD_WIDTH + wx;

	xy *= 1367130551;
	xy ^= xy >> 16;
	xy ^= xy >> 8;
	xy ^= xy >> 4;
	xy ^= xy >> 2;
	xy ^= xy >> 1;

	return (xy & 1) ? ORANGE : BROWN;
}

inline static bool has_dirt(int wx, int wy)
{
	int gx = wx / CELL_SCALE;
	int gy = wy / CELL_SCALE;

	if (gx < 0 || gy < 0)
		return false;

	if (gx >= GRID_WIDTH || gy >= GRID_HEIGHT)
		return false;

	return (grid[gy][gx / 32] >> (gx & 31)) & 1;
}

static int poke_hole(int gx, int gy, int radius)
{
	/* Maximum allowable radius that fits within the sqrt_table. */
	radius = abs(clamp(radius, -HOLE_MAX_RADIUS, HOLE_MAX_RADIUS));

	int cells = 0;

	for (int y = gy - radius; y <= gy + radius; y++) {
		if (y < 0)
			continue;

		if (y > GRID_BOTTOM)
			break;

		for (int x = gx - radius; x <= gx + radius; x++) {
			if (x < 0)
				continue;

			if (x > GRID_RIGHT)
				break;

			int r = sqrt_table[(y - gy) * (y - gy) + (x - gx) * (x - gx)];

			if (r > radius)
				continue;

			cells += (grid[y][x / 32] >> (x & 31)) & 1;
			grid[y][x / 32] &= ~(1u << (x & 31));
		}
	}

	return cells;
}

static int poke_sprite(const sdk_sprite_t *s)
{
	int cells = 0;

	const sdk_tileset_t *ts = s->ts;
	const uint8_t *data = sdk_get_tile_data(ts, s->tile);

	int wx = s->x - s->ox;
	int wy = s->y - s->oy;

	for (int sy = 0; sy < ts->h; sy++) {
		int gy = (wy + sy) / CELL_SCALE;

		if (gy < 0)
			continue;

		if (gy > GRID_BOTTOM)
			continue;

		for (int sx = 0; sx < ts->w; sx++) {
			if (data[sy * ts->w + sx] == ts->trsp)
				continue;

			int gx = (wx + sx) / CELL_SCALE;

			if (gx < 0)
				continue;

			if (gx > GRID_RIGHT)
				break;

			cells += (grid[gy][gx / 32] >> (gx & 31)) & 1;
			grid[gy][gx / 32] &= ~(1u << (gx & 31));
		}
	}

	return cells;
}

inline static int8_t get_angle(float rx, float ry)
{
	int8_t angles[3][3] = {
		{ 7, 0, 1 },
		{ 6, -1, 2 },
		{ 5, 4, 3 },
	};

	int y_orient = ry < 0 ? 0 : (ry > 0 ? 2 : 1);
	int x_orient = rx < 0 ? 0 : (rx > 0 ? 2 : 1);

	return angles[y_orient][x_orient];
}

static void move_and_slide(float wx, float wy, struct tank *t1, const struct tank *t2)
{
	/*
	 * Calculate collission baseline.
	 * We might be already colliding due to a rotation.
	 */
	int baseline = sdk_sprites_collide(&t1->s, &t2->s);

	/*
	 * Try moving horizontally and if the collission gets worse,
	 * cancel that movement.
	 */
	float orig_x = t1->s.x;
	t1->s.x = wx;
	int h_score = sdk_sprites_collide(&t1->s, &t2->s);
	if (h_score && h_score > baseline)
		t1->s.x = orig_x;

	/*
	 * Try moving vertically and if the collission gets worse,
	 * cancel that movement.
	 */
	float orig_y = t1->s.y;
	t1->s.y = wy;
	int v_score = sdk_sprites_collide(&t1->s, &t2->s);
	if (v_score && v_score > baseline)
		t1->s.y = orig_y;
}

static float angle_to_x(int angle)
{
	switch (angle & 7) {
	case 2:
		return +1.0f;

	case 6:
		return -1.0f;

	case 1:
	case 3:
		return +M_SQRT1_2;

	case 5:
	case 7:
		return -M_SQRT1_2;
	}

	return 0.0f;
}

static float angle_to_y(int angle)
{
	switch (angle & 7) {
	case 4:
		return +1.0f;

	case 0:
		return -1.0f;

	case 3:
	case 5:
		return +M_SQRT1_2;

	case 1:
	case 7:
		return -M_SQRT1_2;
	}

	return 0.0f;
}

static void explode(int gx, int gy, int size)
{
	if (size < 1)
		return;

	if (gx < 0 || gx > GRID_RIGHT)
		return;

	if (gy < 0 || gy > GRID_BOTTOM)
		return;

	uint32_t *tile = &grid[gy][gx / 32];
	uint32_t bit = 1u << (gx & 31);

	if (!((*tile) & bit))
		return;

	*tile &= ~bit;

	for (int y = gy - 1; y <= gy + 1; y++)
		for (int x = gx - 1; x <= gx + 1; x++)
			if (rand() & 1)
				explode(x, y, size - 1);
}
