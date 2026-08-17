#include "minevox.h"

#include <stdlib.h>
#include <string.h>

/* clang-format off */
const struct block_info block_info[NUM_BLOCKS] = {
	/*                  name          solid opaque liquid shades light hardness */
	[BLOCK_AIR]       = { "Air",       0, 0, 0, 0,  0,   0 },
	[BLOCK_GRASS]     = { "Grass",     1, 1, 0, 1,  0,   6 },
	[BLOCK_DIRT]      = { "Dirt",      1, 1, 0, 1,  0,   5 },
	[BLOCK_STONE]     = { "Stone",     1, 1, 0, 1,  0,  12 },
	[BLOCK_COBBLE]    = { "Cobble",    1, 1, 0, 1,  0,  14 },
	[BLOCK_SAND]      = { "Sand",      1, 1, 0, 1,  0,   5 },
	[BLOCK_LOG]       = { "Log",       1, 1, 0, 1,  0,  10 },
	[BLOCK_LEAVES]    = { "Leaves",    1, 1, 0, 0,  0,   2 },
	[BLOCK_PLANKS]    = { "Planks",    1, 1, 0, 1,  0,   9 },
	[BLOCK_GLASS]     = { "Glass",     1, 0, 0, 0,  0,   3 },
	[BLOCK_WATER]     = { "Water",     0, 0, 1, 0,  0,   0 },
	[BLOCK_BRICK]     = { "Bricks",    1, 1, 0, 1,  0,  14 },
	[BLOCK_COAL_ORE]  = { "Coal Ore",  1, 1, 0, 1,  0,  15 },
	[BLOCK_IRON_ORE]  = { "Iron Ore",  1, 1, 0, 1,  0,  18 },
	[BLOCK_GLOWSTONE] = { "Glowstone", 1, 1, 0, 1, 15,   4 },
	[BLOCK_BEDROCK]   = { "Bedrock",   1, 1, 0, 1,  0, 255 },
};
/* clang-format on */

color_t block_tex[NUM_BLOCKS][NUM_FACES][TEX_SIZE * TEX_SIZE];
color_t block_avg[NUM_BLOCKS];

/* Deterministic per-texel randomness. */
static uint32_t tex_hash(uint32_t a)
{
	a ^= a >> 16;
	a *= 0x7feb352dU;
	a ^= a >> 15;
	a *= 0x846ca68bU;
	a ^= a >> 16;
	return a;
}

/* Signed noise in range -amp .. +amp, stable for a given texel. */
static int tex_noise(int block, int face, int x, int y, int amp)
{
	uint32_t h = tex_hash((block * 977u + face) * 8191u + y * 137u + x);
	return (int)(h % (2u * amp + 1u)) - amp;
}

static color_t shade_rgb(int r, int g, int b)
{
	r = clampi(r, 0, 255);
	g = clampi(g, 0, 255);
	b = clampi(b, 0, 255);
	return rgb_to_rgb565(r, g, b);
}

/* Fill a whole face with a noisy base color. */
static void face_fill(int block, int face, int r, int g, int b, int amp)
{
	for (int y = 0; y < TEX_SIZE; y++) {
		for (int x = 0; x < TEX_SIZE; x++) {
			int n = tex_noise(block, face, x, y, amp);
			block_tex[block][face][y * TEX_SIZE + x] = shade_rgb(r + n, g + n, b + n);
		}
	}
}

static void texel(int block, int face, int x, int y, int r, int g, int b)
{
	block_tex[block][face][(y & TEX_MASK) * TEX_SIZE + (x & TEX_MASK)] = shade_rgb(r, g, b);
}

/* Sprinkle blobs of a second color over a face, used for ores and cobble. */
static void face_blobs(int block, int face, int count, int r, int g, int b, int amp)
{
	uint32_t rnd = tex_hash(block * 31337u + face);

	for (int i = 0; i < count; i++) {
		rnd = tex_hash(rnd);
		int cx = rnd % TEX_SIZE;
		rnd = tex_hash(rnd);
		int cy = rnd % TEX_SIZE;
		rnd = tex_hash(rnd);
		int size = 1 + (rnd & 1);

		for (int dy = 0; dy <= size; dy++) {
			for (int dx = 0; dx <= size; dx++) {
				if (dx == size && dy == size)
					continue;

				int n = tex_noise(block, face + 7, cx + dx, cy + dy, amp);
				texel(block, face, cx + dx, cy + dy, r + n, g + n, b + n);
			}
		}
	}
}

/* Copy one face over another. */
static void face_copy(int block, int dst, int src)
{
	memcpy(block_tex[block][dst], block_tex[block][src], sizeof(block_tex[block][dst]));
}

static void make_grass(void)
{
	face_fill(BLOCK_GRASS, FACE_TOP, 96, 156, 68, 18);
	face_fill(BLOCK_GRASS, FACE_BOTTOM, 134, 96, 67, 14);
	face_fill(BLOCK_GRASS, FACE_SIDE, 134, 96, 67, 14);

	/* Ragged green fringe on the sides. */
	for (int x = 0; x < TEX_SIZE; x++) {
		int depth = 2 + (tex_hash(x * 71u + 3u) & 1);

		for (int y = 0; y < depth; y++) {
			int n = tex_noise(BLOCK_GRASS, FACE_SIDE, x, y, 16);
			texel(BLOCK_GRASS, FACE_SIDE, x, y, 92 + n, 152 + n, 64 + n);
		}
	}
}

static void make_log(void)
{
	/* Bark. */
	face_fill(BLOCK_LOG, FACE_SIDE, 96, 74, 44, 10);

	for (int x = 0; x < TEX_SIZE; x++) {
		if (tex_hash(x * 313u) & 1)
			continue;

		for (int y = 0; y < TEX_SIZE; y++) {
			int n = tex_noise(BLOCK_LOG, FACE_SIDE, x, y, 8);
			texel(BLOCK_LOG, FACE_SIDE, x, y, 74 + n, 55 + n, 32 + n);
		}
	}

	/* Growth rings on the cut. */
	for (int y = 0; y < TEX_SIZE; y++) {
		for (int x = 0; x < TEX_SIZE; x++) {
			int dx = x * 2 - (TEX_SIZE - 1);
			int dy = y * 2 - (TEX_SIZE - 1);
			int d = (dx * dx + dy * dy) >> 4;
			int ring = (d & 1) ? 18 : 0;
			int n = tex_noise(BLOCK_LOG, FACE_TOP, x, y, 8);
			texel(BLOCK_LOG, FACE_TOP, x, y, 168 - ring + n, 136 - ring + n,
			      86 - ring + n);
		}
	}

	face_copy(BLOCK_LOG, FACE_BOTTOM, FACE_TOP);
}

static void make_planks(void)
{
	face_fill(BLOCK_PLANKS, FACE_SIDE, 160, 126, 76, 10);

	for (int x = 0; x < TEX_SIZE; x++) {
		texel(BLOCK_PLANKS, FACE_SIDE, x, 0, 118, 90, 52);
		texel(BLOCK_PLANKS, FACE_SIDE, x, 4, 118, 90, 52);
	}

	/* Short joints between the boards. */
	texel(BLOCK_PLANKS, FACE_SIDE, 5, 1, 128, 100, 60);
	texel(BLOCK_PLANKS, FACE_SIDE, 5, 2, 128, 100, 60);
	texel(BLOCK_PLANKS, FACE_SIDE, 5, 3, 128, 100, 60);
	texel(BLOCK_PLANKS, FACE_SIDE, 1, 5, 128, 100, 60);
	texel(BLOCK_PLANKS, FACE_SIDE, 1, 6, 128, 100, 60);
	texel(BLOCK_PLANKS, FACE_SIDE, 1, 7, 128, 100, 60);

	face_copy(BLOCK_PLANKS, FACE_TOP, FACE_SIDE);
	face_copy(BLOCK_PLANKS, FACE_BOTTOM, FACE_SIDE);
}

static void make_brick(void)
{
	face_fill(BLOCK_BRICK, FACE_SIDE, 150, 78, 62, 10);

	for (int y = 0; y < TEX_SIZE; y++) {
		for (int x = 0; x < TEX_SIZE; x++) {
			int row = y >> 2;
			int offset = row & 1 ? 2 : 6;
			bool mortar = (y & 3) == 3 || ((x + offset) & 7) == 0;

			if (mortar)
				texel(BLOCK_BRICK, FACE_SIDE, x, y, 176, 168, 160);
		}
	}

	face_copy(BLOCK_BRICK, FACE_TOP, FACE_SIDE);
	face_copy(BLOCK_BRICK, FACE_BOTTOM, FACE_SIDE);
}

static void make_glass(void)
{
	face_fill(BLOCK_GLASS, FACE_SIDE, 196, 226, 236, 6);

	for (int i = 0; i < TEX_SIZE; i++) {
		texel(BLOCK_GLASS, FACE_SIDE, i, 0, 236, 248, 252);
		texel(BLOCK_GLASS, FACE_SIDE, i, TEX_SIZE - 1, 236, 248, 252);
		texel(BLOCK_GLASS, FACE_SIDE, 0, i, 236, 248, 252);
		texel(BLOCK_GLASS, FACE_SIDE, TEX_SIZE - 1, i, 236, 248, 252);
	}

	/* A diagonal highlight so the pane reads as glass. */
	texel(BLOCK_GLASS, FACE_SIDE, 2, 5, 248, 252, 255);
	texel(BLOCK_GLASS, FACE_SIDE, 3, 4, 248, 252, 255);
	texel(BLOCK_GLASS, FACE_SIDE, 4, 3, 248, 252, 255);

	face_copy(BLOCK_GLASS, FACE_TOP, FACE_SIDE);
	face_copy(BLOCK_GLASS, FACE_BOTTOM, FACE_SIDE);
}

static void make_leaves(void)
{
	face_fill(BLOCK_LEAVES, FACE_SIDE, 62, 128, 48, 26);

	/* Punch a few gaps so the canopy looks airy. */
	for (int i = 0; i < 10; i++) {
		uint32_t h = tex_hash(i * 5701u + 17u);
		texel(BLOCK_LEAVES, FACE_SIDE, h % TEX_SIZE, (h >> 8) % TEX_SIZE, 34, 76, 30);
	}

	face_copy(BLOCK_LEAVES, FACE_TOP, FACE_SIDE);
	face_copy(BLOCK_LEAVES, FACE_BOTTOM, FACE_SIDE);
}

static void make_water(void)
{
	for (int y = 0; y < TEX_SIZE; y++) {
		for (int x = 0; x < TEX_SIZE; x++) {
			int wave = ((x + y) & 3) == 0 ? 22 : 0;
			int n = tex_noise(BLOCK_WATER, FACE_TOP, x, y, 8);
			texel(BLOCK_WATER, FACE_TOP, x, y, 42 + wave + n, 90 + wave + n,
			      196 + wave + n);
		}
	}

	face_copy(BLOCK_WATER, FACE_SIDE, FACE_TOP);
	face_copy(BLOCK_WATER, FACE_BOTTOM, FACE_TOP);
}

static void make_glowstone(void)
{
	face_fill(BLOCK_GLOWSTONE, FACE_SIDE, 206, 166, 88, 16);
	face_blobs(BLOCK_GLOWSTONE, FACE_SIDE, 8, 250, 232, 150, 6);
	face_copy(BLOCK_GLOWSTONE, FACE_TOP, FACE_SIDE);
	face_copy(BLOCK_GLOWSTONE, FACE_BOTTOM, FACE_SIDE);
}

static void make_simple(int block, int r, int g, int b, int amp)
{
	face_fill(block, FACE_SIDE, r, g, b, amp);
	face_copy(block, FACE_TOP, FACE_SIDE);
	face_copy(block, FACE_BOTTOM, FACE_SIDE);
}

static void make_ore(int block, int r, int g, int b)
{
	make_simple(block, 128, 128, 128, 12);
	face_blobs(block, FACE_SIDE, 5, r, g, b, 10);
	face_copy(block, FACE_TOP, FACE_SIDE);
	face_copy(block, FACE_BOTTOM, FACE_SIDE);
}

void blocks_init(void)
{
	make_grass();
	make_simple(BLOCK_DIRT, 134, 96, 67, 16);
	make_simple(BLOCK_STONE, 128, 128, 128, 14);

	make_simple(BLOCK_COBBLE, 122, 122, 122, 10);
	face_blobs(BLOCK_COBBLE, FACE_SIDE, 7, 96, 96, 96, 12);
	face_copy(BLOCK_COBBLE, FACE_TOP, FACE_SIDE);
	face_copy(BLOCK_COBBLE, FACE_BOTTOM, FACE_SIDE);

	make_simple(BLOCK_SAND, 218, 206, 156, 12);
	make_log();
	make_leaves();
	make_planks();
	make_glass();
	make_water();
	make_brick();
	make_ore(BLOCK_COAL_ORE, 40, 40, 40);
	make_ore(BLOCK_IRON_ORE, 200, 158, 120);
	make_glowstone();
	make_simple(BLOCK_BEDROCK, 68, 68, 72, 26);

	/* Average color of the top face, handy for flat HUD bits. */
	for (int b = 1; b < NUM_BLOCKS; b++) {
		int r = 0, g = 0, bl = 0;

		for (int i = 0; i < TEX_SIZE * TEX_SIZE; i++) {
			color_t c = block_tex[b][FACE_TOP][i];
			r += rgb565_red(c);
			g += rgb565_green(c);
			bl += rgb565_blue(c);
		}

		block_avg[b] = shade_rgb(r / (TEX_SIZE * TEX_SIZE), g / (TEX_SIZE * TEX_SIZE),
					 bl / (TEX_SIZE * TEX_SIZE));
	}
}

void blocks_draw_icon(int x0, int y0, int size, uint8_t block, uint8_t bright)
{
	if (!block || block >= NUM_BLOCKS)
		return;

	int a = size / 2; /* Half width. */
	int q = size / 4; /* Quarter height. */

	if (a < 2 || q < 1)
		return;

	for (int py = 0; py < size; py++) {
		for (int px = 0; px < size; px++) {
			int face;
			int u, v;

			int dx = px - a;
			int dy = py - q;

			if (abs(dx) * q + abs(dy) * a <= a * q) {
				/* Top diamond. */
				float fa = (float)dx / a;
				float fb = (float)dy / q;
				float tu = (fa + fb + 1.0f) * 0.5f;
				float tv = (fb - fa + 1.0f) * 0.5f;
				face = FACE_TOP;
				u = (int)(tu * TEX_SIZE);
				v = (int)(tv * TEX_SIZE);
			} else if (px < a) {
				/* Left side. */
				int top = q + (px * q) / a;

				if (py < top || py >= top + 2 * q)
					continue;

				face = FACE_SIDE;
				u = (px * TEX_SIZE) / a;
				v = ((py - top) * TEX_SIZE) / (2 * q);
			} else {
				/* Right side. */
				int top = 2 * q - ((px - a) * q) / a;

				if (py < top || py >= top + 2 * q)
					continue;

				face = FACE_SIDE;
				u = ((px - a) * TEX_SIZE) / a;
				v = ((py - top) * TEX_SIZE) / (2 * q);
			}

			u = clampi(u, 0, TEX_MASK);
			v = clampi(v, 0, TEX_MASK);

			color_t c = block_tex[block][face][v * TEX_SIZE + u];
			int shade = face == FACE_SIDE ? (px < a ? 24 : 17) : 32;

			c = color_mul(c, (uint8_t)clampi((shade * bright) >> 5, 1, 32));

			tft_draw_pixel(x0 + px, y0 + py, c);
		}
	}
}
