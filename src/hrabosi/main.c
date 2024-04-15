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

static sdk_sprite_t ss_green_tank[8] = { &s_green_tank_0,   &s_green_tank_45,  &s_green_tank_90,
					 &s_green_tank_135, &s_green_tank_180, &s_green_tank_225,
					 &s_green_tank_270, &s_green_tank_315 };

inline static int dirt_color(int wx, int wy);
inline static bool has_dirt(int wx, int wy);
static int poke_hole(int gx, int gy, int radius);
static int poke_sprite(int wx, int wy, const struct sdk_sprite *sprite);
inline static int8_t get_angle(float rx, float ry);

struct tank {
	float wx, wy;
	float speed;
	uint8_t angle;
	uint8_t color;
};

static struct tank tank1;

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
	tank1.color = GREEN;
	tank1.speed = TANK_SPEED;

	poke_hole(tank1.wx / CELL_SCALE, tank1.wy / CELL_SCALE, 12);
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0;
	float joy_x = dt * (sdk_inputs.joy_x + 64) / 2048.0;
	float joy_y = dt * (sdk_inputs.joy_y + 64) / 2048.0;

	float tank1_wx = tank1.wx;
	float tank1_wy = tank1.wy;

	tank1.speed = clamp(tank1.speed + TANK_SPEED * dt, 0, TANK_SPEED);

	sdk_sprite_t t1s = ss_green_tank[tank1.angle];
	int cells = poke_sprite(tank1.wx - t1s->w / 2.0f, tank1.wy - t1s->w / 2.0f, t1s);
	tank1.speed /= (cells + 1);

	if (!cells)
		tank1.speed = clamp(tank1.speed * 1.2f, 0, TANK_SPEED);

	if (abs(sdk_inputs.joy_x) > 200)
		tank1.wx = clamp(tank1.wx + joy_x * tank1.speed, 5, WORLD_RIGHT - 5);

	if (abs(sdk_inputs.joy_y) > 200)
		tank1.wy = clamp(tank1.wy + joy_y * tank1.speed, 5, WORLD_BOTTOM - 5);

	int tank1_angle = get_angle(tank1.wx - tank1_wx, tank1.wy - tank1_wy);
	if (tank1_angle >= 0)
		tank1.angle = tank1_angle;
}

void game_paint(unsigned __unused dt_usec)
{
	int viewport_left = clamp(tank1.wx - TFT_WIDTH / 2.0f, 0, WORLD_RIGHT - TFT_WIDTH);
	int viewport_top = clamp(tank1.wy - TFT_HEIGHT / 2.0f, 0, WORLD_BOTTOM - TFT_HEIGHT);
	//int viewport_right = viewport_left + TFT_WIDTH - 1;
	//int viewport_bottom = viewport_top + TFT_HEIGHT - 1;

	for (int y = 0; y < TFT_HEIGHT; y++) {
		/*
		 * Painting the background takes a lot of time.
		 * Make sure the input task can run unimpeded.
		 */
		sdk_yield_every_us(2000);

		for (int x = 0; x < TFT_WIDTH; x++) {
			int wx = viewport_left + x;
			int wy = viewport_top + y;

			tft_draw_pixel(x, y, has_dirt(wx, wy) ? dirt_color(wx, wy) : BLACK);
		}
	}

	{
		int vx = tank1.wx - viewport_left;
		int vy = tank1.wy - viewport_top;

		sdk_draw_sprite(vx - 8, vy - 8, ss_green_tank[tank1.angle]);
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
