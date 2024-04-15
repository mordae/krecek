#include <pico/stdlib.h>

#include <sdk.h>
#include <tft.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

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

#define GRID_WIDTH 640
#define GRID_HEIGHT 480
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

embed_sprite(s_green_tank_0, 16, 16, BLACK, "green_tank_0.data");
embed_sprite(s_green_tank_45, 16, 16, BLACK, "green_tank_45.data");
embed_sprite(s_green_tank_90, 16, 16, BLACK, "green_tank_90.data");
embed_sprite(s_green_tank_135, 16, 16, BLACK, "green_tank_135.data");
embed_sprite(s_green_tank_180, 16, 16, BLACK, "green_tank_180.data");
embed_sprite(s_green_tank_225, 16, 16, BLACK, "green_tank_225.data");
embed_sprite(s_green_tank_270, 16, 16, BLACK, "green_tank_270.data");
embed_sprite(s_green_tank_315, 16, 16, BLACK, "green_tank_315.data");

embed_sprite(s_blue_tank_0, 16, 16, BLACK, "blue_tank_0.data");
embed_sprite(s_blue_tank_45, 16, 16, BLACK, "blue_tank_45.data");
embed_sprite(s_blue_tank_90, 16, 16, BLACK, "blue_tank_90.data");
embed_sprite(s_blue_tank_135, 16, 16, BLACK, "blue_tank_135.data");
embed_sprite(s_blue_tank_180, 16, 16, BLACK, "blue_tank_180.data");
embed_sprite(s_blue_tank_225, 16, 16, BLACK, "blue_tank_225.data");
embed_sprite(s_blue_tank_270, 16, 16, BLACK, "blue_tank_270.data");
embed_sprite(s_blue_tank_315, 16, 16, BLACK, "blue_tank_315.data");

static sdk_sprite_t ss_green_tank[8] = { &s_green_tank_0,   &s_green_tank_45,  &s_green_tank_90,
					 &s_green_tank_135, &s_green_tank_180, &s_green_tank_225,
					 &s_green_tank_270, &s_green_tank_315 };

static sdk_sprite_t ss_blue_tank[8] = { &s_blue_tank_0,	  &s_blue_tank_45,  &s_blue_tank_90,
					&s_blue_tank_135, &s_blue_tank_180, &s_blue_tank_225,
					&s_blue_tank_270, &s_blue_tank_315 };

struct tank {
	float wx, wy;
	float speed;
	uint8_t angle;
};

inline static int dirt_color(int wx, int wy);
inline static bool has_dirt(int wx, int wy);
static int poke_hole(int gx, int gy, int radius);
static int poke_sprite(int wx, int wy, const struct sdk_sprite *sprite);
inline static int8_t get_angle(float rx, float ry);
static int sprites_collide(int s1x, int s1y, sdk_sprite_t s1, int s2x, int s2y, sdk_sprite_t s2);
static bool sprite_has_opaque_point(int sx, int sy, sdk_sprite_t s);
static void slide_on_collission(float wx, float wy, struct tank *t1, sdk_sprite_t s1,
				struct tank *t2, sdk_sprite_t s2);

static struct tank tank1;
static struct tank tank2;

#define TANK_SIZE 16
#define TANK_SPEED 66.0f

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = true,
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
}

void game_reset(void)
{
	memset(grid, 0xff, sizeof grid);

	for (int i = 0; i < NUM_INITIAL_HOLES; i++) {
		poke_hole(rand() % (GRID_WIDTH + HOLE_MAX_RADIUS * 2) - HOLE_MAX_RADIUS,
			  rand() % (GRID_HEIGHT + HOLE_MAX_RADIUS * 2) - HOLE_MAX_RADIUS,
			  rand() % 10 + 2);
	}

	tank1.wx = (int)((rand() % (WORLD_WIDTH * 6 / 8)) + WORLD_WIDTH / 8);
	tank1.wy = (int)((rand() % (WORLD_HEIGHT * 6 / 8)) + WORLD_HEIGHT / 8);
	tank1.speed = TANK_SPEED;

	tank2.wx = tank1.wx;
	tank2.wy = tank1.wy;
	tank2.speed = TANK_SPEED;

	while ((fabsf(tank2.wx - tank1.wx) < WORLD_WIDTH / 4.0f) &&
	       (fabsf(tank2.wy - tank1.wy) < WORLD_HEIGHT / 4.0f)) {
		tank2.wx = (int)((rand() % (WORLD_WIDTH * 6 / 8)) + WORLD_WIDTH / 8);
		tank2.wy = (int)((rand() % (WORLD_HEIGHT * 6 / 8)) + WORLD_HEIGHT / 8);
	}

	poke_hole(tank1.wx / CELL_SCALE, tank1.wy / CELL_SCALE, 12);
	poke_hole(tank2.wx / CELL_SCALE, tank2.wy / CELL_SCALE, 12);
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0;

	{
		sdk_inputs.joy_x = clamp((sdk_inputs.joy_x + 64) * 5 / 4, -2047, 2047);
		sdk_inputs.joy_y = clamp((sdk_inputs.joy_y + 64) * 5 / 4, -2047, 2047);

		float joy_x = dt * sdk_inputs.joy_x / 2047.0f;
		float joy_y = dt * sdk_inputs.joy_y / 2047.0f;

		float wx = tank1.wx;
		float wy = tank1.wy;

		tank1.speed = clamp(tank1.speed + TANK_SPEED * dt, 0, TANK_SPEED);

		sdk_sprite_t s = ss_green_tank[tank1.angle];
		int cells = poke_sprite(tank1.wx - s->w / 2.0f, tank1.wy - s->w / 2.0f, s);
		tank1.speed /= (cells + 1);

		if (!cells)
			tank1.speed = clamp(tank1.speed * 1.2f, 0, TANK_SPEED);

		if (abs(sdk_inputs.joy_x) > 200)
			tank1.wx = clamp(tank1.wx + joy_x * tank1.speed, TANK_SIZE / 2.0f,
					 WORLD_RIGHT - TANK_SIZE / 2.0f);

		if (abs(sdk_inputs.joy_y) > 200)
			tank1.wy = clamp(tank1.wy + joy_y * tank1.speed, TANK_SIZE / 2.0f,
					 WORLD_BOTTOM - TANK_SIZE / 2.0f);

		int angle = get_angle(tank1.wx - wx, tank1.wy - wy);
		if (angle >= 0)
			tank1.angle = angle;

		sdk_sprite_t sg = ss_green_tank[tank1.angle];
		sdk_sprite_t sb = ss_blue_tank[tank2.angle];
		slide_on_collission(wx, wy, &tank1, sg, &tank2, sb);
	}

	{
		float joy_x = dt * (sdk_inputs.b - sdk_inputs.x);
		float joy_y = dt * (sdk_inputs.a - sdk_inputs.y);

		float wx = tank2.wx;
		float wy = tank2.wy;

		tank2.speed = clamp(tank2.speed + TANK_SPEED * dt, 0, TANK_SPEED);

		sdk_sprite_t s = ss_blue_tank[tank2.angle];
		int cells = poke_sprite(tank2.wx - s->w / 2.0f, tank2.wy - s->w / 2.0f, s);
		tank2.speed /= (cells + 1);

		if (!cells)
			tank2.speed = clamp(tank2.speed * 1.2f, 0, TANK_SPEED);

		tank2.wx = clamp(tank2.wx + joy_x * tank2.speed, TANK_SIZE / 2.0f,
				 WORLD_RIGHT - TANK_SIZE / 2.0f);
		tank2.wy = clamp(tank2.wy + joy_y * tank2.speed, TANK_SIZE / 2.0f,
				 WORLD_BOTTOM - TANK_SIZE / 2.0f);

		int angle = get_angle(tank2.wx - wx, tank2.wy - wy);
		if (angle >= 0)
			tank2.angle = angle;

		sdk_sprite_t sb = ss_blue_tank[tank2.angle];
		sdk_sprite_t sg = ss_green_tank[tank1.angle];
		slide_on_collission(wx, wy, &tank2, sb, &tank1, sg);
	}
}

void game_paint(unsigned __unused dt_usec)
{
	const int pane_height = TFT_HEIGHT;
	const int pane_width = TFT_WIDTH / 2 - 1;
	const int pane_gap = 2;

	sdk_sprite_t green = ss_green_tank[tank1.angle];
	sdk_sprite_t blue = ss_blue_tank[tank2.angle];

	int t1x = tank1.wx;
	int t1y = tank1.wy;

	int t2x = tank2.wx;
	int t2y = tank2.wy;

	tft_draw_rect(pane_width, 0, pane_width + pane_gap, pane_height, DGRAY);

	{
		int wx = clamp(t1x - pane_width * 1 / 2, 0, WORLD_RIGHT - pane_width);
		int wy = clamp(t1y - pane_height * 2 / 5, 0, WORLD_BOTTOM - pane_height);

		tft_set_clip(0, 0, pane_width, pane_height);
		tft_set_origin(wx, wy);

		for (int y = wy; y < wy + pane_height; y++) {
			sdk_yield_every_us(2000);

			for (int x = wx; x < wx + pane_width; x++) {
				bool dirt = has_dirt(x, y);
				tft_draw_pixel(x, y, dirt ? dirt_color(x, y) : BLACK);
			}
		}

		sdk_draw_sprite(t2x - blue->w / 2, t2y - blue->h / 2, blue);
		sdk_draw_sprite(t1x - green->w / 2, t1y - green->h / 2, green);

		tft_set_origin(0, 0);
		tft_clear_clip();
	}

	{
		int wx = clamp(t2x - pane_width * 1 / 2, 0, WORLD_RIGHT - pane_width);
		int wy = clamp(t2y - pane_height * 2 / 5, 0, WORLD_BOTTOM - pane_height);

		tft_set_clip(pane_width + pane_gap, 0, TFT_WIDTH, pane_height);
		tft_set_origin(wx - pane_gap - pane_width, wy);

		for (int y = wy; y < wy + pane_height; y++) {
			sdk_yield_every_us(2000);

			for (int x = wx; x < wx + pane_width; x++) {
				bool dirt = has_dirt(x, y);
				tft_draw_pixel(x, y, dirt ? dirt_color(x, y) : BLACK);
			}
		}

		sdk_draw_sprite(t1x - green->w / 2, t1y - green->h / 2, green);
		sdk_draw_sprite(t2x - blue->w / 2, t2y - blue->h / 2, blue);

		tft_set_origin(0, 0);
		tft_clear_clip();
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

static int poke_sprite(int wx, int wy, sdk_sprite_t sprite)
{
	int cells = 0;

	for (int sy = 0; sy < sprite->h; sy++) {
		int gy = (wy + sy) / CELL_SCALE;

		if (gy < 0)
			continue;

		if (gy > GRID_BOTTOM)
			continue;

		for (int sx = 0; sx < sprite->w; sx++) {
			if (sprite->data[sy * sprite->w + sx] == sprite->transparency)
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

static int sprites_collide(int s1x, int s1y, sdk_sprite_t s1, int s2x, int s2y, sdk_sprite_t s2)
{
	int points = 0;

	if (s1x + s1->w < s2x || s2x + s2->w < s1x)
		return 0;

	if (s1y + s1->h < s2y || s2y + s2->h < s1y)
		return 0;

	for (int y = 0; y < s1->h; y++) {
		for (int x = 0; x < s1->w; x++) {
			int x2 = s2x - s1x + x;
			int y2 = s2y - s1y + y;

			if (sprite_has_opaque_point(x, y, s1) &&
			    sprite_has_opaque_point(x2, y2, s2))
				points++;
		}
	}

	return points;
}

static bool sprite_has_opaque_point(int sx, int sy, sdk_sprite_t s)
{
	if (sx < 0 || sx >= s->w)
		return false;

	if (sy < 0 || sy >= s->h)
		return false;

	return s->data[sy * s->w + sx] != s->transparency;
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

static void slide_on_collission(float wx, float wy, struct tank *t1, sdk_sprite_t s1,
				struct tank *t2, sdk_sprite_t s2)
{
	/*
	 * Calculate collission baseline.
	 * We might be already colliding due to a rotation.
	 */
	int baseline = sprites_collide((int)wx - s1->w / 2, (int)wy - s1->h / 2, s1,
				       (int)t2->wx - s2->w / 2, (int)t2->wy - s2->h / 2, s2);

	/*
	 * Try moving horizontally and if the collission gets worse,
	 * cancel that movement.
	 */
	int h_score = sprites_collide((int)t1->wx - s1->w / 2, (int)wy - s1->h / 2, s1,
				      (int)t2->wx - s2->w / 2, (int)t2->wy - s2->h / 2, s2);
	if (h_score && h_score > baseline)
		t1->wx = wx;

	/*
	 * Try moving vertically and if the collission gets worse,
	 * cancel that movement.
	 */
	int v_score = sprites_collide((int)wx - s1->w / 2, (int)t1->wy - s1->h / 2, s1,
				      (int)t2->wx - s2->w / 2, (int)t2->wy - s2->h / 2, s2);
	if (v_score && v_score > baseline)
		t1->wy = wy;
}
