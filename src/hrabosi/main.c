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

#define WORLD_WIDTH 640
#define WORLD_HEIGHT 480

#define WORLD_RIGHT (WORLD_WIDTH - 1)
#define WORLD_BOTTOM (WORLD_HEIGHT - 1)

static uint32_t grid[WORLD_HEIGHT][WORLD_WIDTH / 32];
static uint8_t sqrt_table[512];

static int camera_left = 0;
static int camera_top = 0;

inline static int dirt_color(int x, int y);
static void poke_hole(int wx, int wy, int radius);

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

	for (int i = 0; i < 64; i++)
		poke_hole(rand() % (WORLD_WIDTH + 64) - 32, rand() % (WORLD_HEIGHT + 64) - 32,
			  rand() % 14 + 3);
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

			int dirt = (grid[wy][wx / 32] >> (wx & 31)) & 1;
			tft_draw_pixel(x, y, dirt ? dirt_color(wx, wy) : BLACK);
		}
	}
}

inline static int dirt_color(int x, int y)
{
	int xy = y * WORLD_WIDTH + x;

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

static void poke_hole(int wx, int wy, int radius)
{
	/* Maximum allowable radius that fits within the sqrt_table. */
	radius = clamp(radius, -16, 16);

	for (int y = wy - radius; y <= wy + radius; y++) {
		if (y < 0)
			continue;

		if (y > WORLD_BOTTOM)
			break;

		for (int x = wx - radius; x <= wx + radius; x++) {
			if (x < 0)
				continue;

			if (x > WORLD_RIGHT)
				break;

			int r = sqrt_table[(y - wy) * (y - wy) + (x - wx) * (x - wx)];

			if (r > radius)
				continue;

			grid[y][x / 32] &= ~(1u << (x & 31));
		}
	}
}
