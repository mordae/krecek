#include <pico/stdlib.h>

#include <sdk.h>
#include <tft.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define RED 255
#define ORANGE 240
#define BROWN 241
#define YELLOW 242
#define GREEN 244
#define BLUE 234
#define WHITE 15
#define GRAY 7
#define BLACK 0
#define DARK_RED (RED - 64)
#define DARK_ORANGE (ORANGE - 64)
#define DARK_BROWN (BROWN - 64)

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

static int camera_left = 0;
static int camera_top = 0;

inline static int dirt_color(int wx, int wy);
inline static bool has_dirt(int wx, int wy);
static void poke_hole(int gx, int gy, int radius);

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = true,
	};

	sdk_main(&config);
}

void game_start(void)
{
	/* Pre-calculate square roots. */
	for (unsigned i = 0; i < sizeof(sqrt_table); i++)
		sqrt_table[i] = roundf(sqrtf(i));
}

void game_reset(void)
{
	camera_left = TFT_WIDTH / 2;
	camera_top = TFT_HEIGHT / 2;

	memset(grid, 0xff, sizeof grid);

	for (int i = 0; i < NUM_INITIAL_HOLES; i++) {
		poke_hole(rand() % (GRID_WIDTH + HOLE_MAX_RADIUS * 2) - HOLE_MAX_RADIUS,
			  rand() % (GRID_HEIGHT + HOLE_MAX_RADIUS * 2) - HOLE_MAX_RADIUS,
			  rand() % 10 + 2);
	}
}

void game_input(void)
{
	camera_top = clamp(camera_top + sdk_inputs.joy_y / 256, TFT_HEIGHT / 2,
			   WORLD_BOTTOM - TFT_HEIGHT / 2);
	camera_left = clamp(camera_left + sdk_inputs.joy_x / 256, TFT_WIDTH / 2,
			    WORLD_RIGHT - TFT_WIDTH / 2);
}

void game_paint(unsigned __unused dt_usec)
{
	int viewport_left = clamp(camera_left - TFT_WIDTH / 2, 0, WORLD_RIGHT - TFT_WIDTH);
	int viewport_top = clamp(camera_top - TFT_HEIGHT / 2, 0, WORLD_BOTTOM - TFT_HEIGHT);
	//int viewport_right = viewport_left + TFT_WIDTH - 1;
	//int viewport_bottom = viewport_top + TFT_HEIGHT - 1;

	for (int y = 0; y < TFT_HEIGHT; y++) {
		for (int x = 0; x < TFT_WIDTH; x++) {
			int wx = viewport_left + x;
			int wy = viewport_top + y;

			tft_draw_pixel(x, y, has_dirt(wx, wy) ? dirt_color(wx, wy) : BLACK);
		}
	}
}

inline static int dirt_color(int wx, int wy)
{
	int xy = wy * WORLD_WIDTH + wx;

	xy ^= xy >> 7;
	xy *= 1367130551;
	xy ^= xy >> 9;
	xy &= 31;

	if (xy >= 31)
		return DARK_RED - 16;

	if (xy >= 1)
		return DARK_ORANGE;

	return DARK_BROWN;
}

inline static bool has_dirt(int wx, int wy)
{
	int gx = wx / CELL_SCALE;
	int gy = wy / CELL_SCALE;

	return (grid[gy][gx / 32] >> (gx & 31)) & 1;
}

static void poke_hole(int gx, int gy, int radius)
{
	/* Maximum allowable radius that fits within the sqrt_table. */
	radius = abs(clamp(radius, -HOLE_MAX_RADIUS, HOLE_MAX_RADIUS));

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

			grid[y][x / 32] &= ~(1u << (x & 31));
		}
	}
}
