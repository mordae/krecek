#include "minevox.h"

#include <stdlib.h>
#include <string.h>

/*
 * ===========================================================================
 * Blocks: what they are made of and what they look like
 * ===========================================================================
 *
 * Two things live here. First the table of block properties, which is the
 * closest thing this game has to a design document: change a number in it
 * and the game changes. Second the textures, which we generate at startup
 * rather than storing as images.
 *
 * WHY GENERATE TEXTURES INSTEAD OF DRAWING THEM?
 *
 * Twenty four blocks with three faces each at 8x8 pixels would be 9 KiB of
 * artwork in the firmware. Generating them costs about thirty lines of
 * code per block and no storage at all beyond the buffer we draw into,
 * and it means a texture can be tweaked by changing a colour rather than
 * by opening an image editor. The trade off is that everything ends up
 * looking a little mathematical, which suits a game made of cubes.
 */

/* clang-format off */
const struct block_info block_info[NUM_BLOCKS] = {
	/*                       name           solid opaque liquid shades light hard tool        drop */
	[BLOCK_AIR]            = { "Air",         0, 0, 0, 0,  0,   0, TOOL_NONE, 0 },
	[BLOCK_GRASS]          = { "Grass",       1, 1, 0, 1,  0,   6, TOOL_NONE, BLOCK_DIRT },
	[BLOCK_DIRT]           = { "Dirt",        1, 1, 0, 1,  0,   5, TOOL_NONE, BLOCK_DIRT },
	[BLOCK_STONE]          = { "Stone",       1, 1, 0, 1,  0,  14, TOOL_PICK, BLOCK_COBBLE },
	[BLOCK_COBBLE]         = { "Cobble",      1, 1, 0, 1,  0,  16, TOOL_PICK, BLOCK_COBBLE },
	[BLOCK_SAND]           = { "Sand",        1, 1, 0, 1,  0,   5, TOOL_NONE, BLOCK_SAND },
	[BLOCK_LOG]            = { "Log",         1, 1, 0, 1,  0,  10, TOOL_AXE,  BLOCK_LOG },
	[BLOCK_LEAVES]         = { "Leaves",      1, 1, 0, 0,  0,   2, TOOL_NONE, 0 },
	[BLOCK_PLANKS]         = { "Planks",      1, 1, 0, 1,  0,   9, TOOL_AXE,  BLOCK_PLANKS },
	[BLOCK_GLASS]          = { "Glass",       1, 0, 0, 0,  0,   3, TOOL_NONE, 0 },
	[BLOCK_WATER]          = { "Water",       0, 0, 1, 0,  0,   0, TOOL_NONE, 0 },
	[BLOCK_BRICK]          = { "Bricks",      1, 1, 0, 1,  0,  16, TOOL_PICK, BLOCK_BRICK },
	[BLOCK_COAL_ORE]       = { "Coal Ore",    1, 1, 0, 1,  0,  18, TOOL_PICK, ITEM_COAL },
	[BLOCK_IRON_ORE]       = { "Iron Ore",    1, 1, 0, 1,  0,  22, TOOL_PICK, BLOCK_IRON_ORE },
	[BLOCK_GOLD_ORE]       = { "Gold Ore",    1, 1, 0, 1,  0,  24, TOOL_PICK, BLOCK_GOLD_ORE },
	[BLOCK_GLOWSTONE]      = { "Glowstone",   1, 1, 0, 1, 15,   4, TOOL_NONE, BLOCK_GLOWSTONE },
	[BLOCK_BEDROCK]        = { "Bedrock",     1, 1, 0, 1,  0, 255, TOOL_NONE, 0 },
	[BLOCK_GOLD_BLOCK]     = { "Gold Block",  1, 1, 0, 1,  0,  20, TOOL_PICK, BLOCK_GOLD_BLOCK },
	[BLOCK_IRON_BLOCK]     = { "Iron Block",  1, 1, 0, 1,  0,  24, TOOL_PICK, BLOCK_IRON_BLOCK },
	[BLOCK_CRAFTING]       = { "Workbench",   1, 1, 0, 1,  0,  10, TOOL_AXE,  BLOCK_CRAFTING },
	[BLOCK_OBSIDIAN]       = { "Obsidian",    1, 1, 0, 1,  0,  90, TOOL_PICK, BLOCK_OBSIDIAN },
	[BLOCK_NETHERRACK]     = { "Netherrack",  1, 1, 0, 1,  2,   5, TOOL_PICK, BLOCK_NETHERRACK },
	[BLOCK_GLOWING_OBSIDIAN]={ "Glowing Obs", 1, 1, 0, 1, 12,  90, TOOL_PICK, 0 },
	[BLOCK_REACTOR]        = { "Reactor Core",1, 1, 0, 1,  4,  12, TOOL_PICK, BLOCK_REACTOR },
	[BLOCK_LAVA]           = { "Lava",        0, 1, 1, 1, 15,   0, TOOL_NONE, 0 },
};
/* clang-format on */

color_t block_tex[NUM_BLOCKS][NUM_FACES][TEX_SIZE * TEX_SIZE];

/*
 * A hash, used as a source of repeatable randomness.
 *
 * Every texel asks this function what it should look like, so the answer
 * has to be the same every time the game starts. A hash gives us that:
 * unlike a random number generator it has no state, so texel (3, 5) of
 * block 7 always gets the same value no matter what order we draw in.
 */
static uint32_t tex_hash(uint32_t a)
{
	a ^= a >> 16;
	a *= 0x7feb352dU;
	a ^= a >> 15;
	a *= 0x846ca68bU;
	a ^= a >> 16;
	return a;
}

/* Noise in the range -amp to +amp, stable for a given texel. */
static int tex_noise(int block, int face, int x, int y, int amp)
{
	uint32_t h = tex_hash((block * 977u + face) * 8191u + y * 137u + x);
	return (int)(h % (2u * amp + 1u)) - amp;
}

static color_t shade_rgb(int r, int g, int b)
{
	return rgb_to_rgb565(clampi(r, 0, 255), clampi(g, 0, 255), clampi(b, 0, 255));
}

/* Fill a whole face with a noisy base colour. */
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

/* Sprinkle blobs of a second colour over a face, for ores and cobble. */
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

static void face_copy(int block, int dst, int src)
{
	memcpy(block_tex[block][dst], block_tex[block][src], sizeof(block_tex[block][dst]));
}

/* Most blocks look the same from every direction. */
static void make_simple(int block, int r, int g, int b, int amp)
{
	face_fill(block, FACE_SIDE, r, g, b, amp);
	face_copy(block, FACE_TOP, FACE_SIDE);
	face_copy(block, FACE_BOTTOM, FACE_SIDE);
}

/* Stone with something valuable embedded in it. */
static void make_ore(int block, int r, int g, int b)
{
	make_simple(block, 128, 128, 128, 12);
	face_blobs(block, FACE_SIDE, 5, r, g, b, 10);
	face_copy(block, FACE_TOP, FACE_SIDE);
	face_copy(block, FACE_BOTTOM, FACE_SIDE);
}

/* A block of solid metal, with a subtle seam so it reads as a block. */
static void make_metal(int block, int r, int g, int b)
{
	face_fill(block, FACE_SIDE, r, g, b, 6);

	for (int i = 0; i < TEX_SIZE; i++) {
		texel(block, FACE_SIDE, i, 0, r + 26, g + 26, b + 26);
		texel(block, FACE_SIDE, 0, i, r + 26, g + 26, b + 26);
		texel(block, FACE_SIDE, i, TEX_SIZE - 1, r - 24, g - 24, b - 24);
		texel(block, FACE_SIDE, TEX_SIZE - 1, i, r - 24, g - 24, b - 24);
	}

	face_copy(block, FACE_TOP, FACE_SIDE);
	face_copy(block, FACE_BOTTOM, FACE_SIDE);
}

static void make_grass(void)
{
	face_fill(BLOCK_GRASS, FACE_TOP, 96, 156, 68, 18);
	face_fill(BLOCK_GRASS, FACE_BOTTOM, 134, 96, 67, 14);
	face_fill(BLOCK_GRASS, FACE_SIDE, 134, 96, 67, 14);

	/* Ragged green fringe along the top of the sides. */
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
	face_fill(BLOCK_LOG, FACE_SIDE, 96, 74, 44, 10);

	/* Vertical bark stripes. */
	for (int x = 0; x < TEX_SIZE; x++) {
		if (tex_hash(x * 313u) & 1)
			continue;

		for (int y = 0; y < TEX_SIZE; y++) {
			int n = tex_noise(BLOCK_LOG, FACE_SIDE, x, y, 8);
			texel(BLOCK_LOG, FACE_SIDE, x, y, 74 + n, 55 + n, 32 + n);
		}
	}

	/* Growth rings on the cut ends: concentric circles from a distance. */
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

static void make_planks(int block, int r, int g, int b)
{
	face_fill(block, FACE_SIDE, r, g, b, 10);

	for (int x = 0; x < TEX_SIZE; x++) {
		texel(block, FACE_SIDE, x, 0, r - 42, g - 36, b - 24);
		texel(block, FACE_SIDE, x, 4, r - 42, g - 36, b - 24);
	}

	/* Short joints, so the boards do not look infinitely long. */
	for (int y = 1; y <= 3; y++)
		texel(block, FACE_SIDE, 5, y, r - 32, g - 26, b - 16);

	for (int y = 5; y <= 7; y++)
		texel(block, FACE_SIDE, 1, y, r - 32, g - 26, b - 16);

	face_copy(block, FACE_TOP, FACE_SIDE);
	face_copy(block, FACE_BOTTOM, FACE_SIDE);
}

static void make_brick(void)
{
	face_fill(BLOCK_BRICK, FACE_SIDE, 150, 78, 62, 10);

	for (int y = 0; y < TEX_SIZE; y++) {
		for (int x = 0; x < TEX_SIZE; x++) {
			/* Every other course is offset, like real brickwork. */
			int offset = (y >> 2) & 1 ? 2 : 6;

			if ((y & 3) == 3 || ((x + offset) & 7) == 0)
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

	/* A diagonal highlight, so it reads as glass rather than as ice. */
	texel(BLOCK_GLASS, FACE_SIDE, 2, 5, 248, 252, 255);
	texel(BLOCK_GLASS, FACE_SIDE, 3, 4, 248, 252, 255);
	texel(BLOCK_GLASS, FACE_SIDE, 4, 3, 248, 252, 255);

	face_copy(BLOCK_GLASS, FACE_TOP, FACE_SIDE);
	face_copy(BLOCK_GLASS, FACE_BOTTOM, FACE_SIDE);
}

static void make_leaves(void)
{
	face_fill(BLOCK_LEAVES, FACE_SIDE, 62, 128, 48, 26);

	/* Punch a few dark gaps so the canopy looks airy. */
	for (int i = 0; i < 10; i++) {
		uint32_t h = tex_hash(i * 5701u + 17u);
		texel(BLOCK_LEAVES, FACE_SIDE, h % TEX_SIZE, (h >> 8) % TEX_SIZE, 34, 76, 30);
	}

	face_copy(BLOCK_LEAVES, FACE_TOP, FACE_SIDE);
	face_copy(BLOCK_LEAVES, FACE_BOTTOM, FACE_SIDE);
}

/* Water and lava share a shape, only the colours differ. */
static void make_fluid(int block, int r, int g, int b, int crest)
{
	for (int y = 0; y < TEX_SIZE; y++) {
		for (int x = 0; x < TEX_SIZE; x++) {
			int wave = ((x + y) & 3) == 0 ? crest : 0;
			int n = tex_noise(block, FACE_TOP, x, y, 8);
			texel(block, FACE_TOP, x, y, r + wave + n, g + wave + n, b + wave + n);
		}
	}

	face_copy(block, FACE_SIDE, FACE_TOP);
	face_copy(block, FACE_BOTTOM, FACE_TOP);
}

static void make_crafting(void)
{
	make_planks(BLOCK_CRAFTING, 160, 126, 76);

	/* A three by three grid on top, the universal sign for crafting. */
	face_fill(BLOCK_CRAFTING, FACE_TOP, 132, 100, 60, 8);

	for (int i = 0; i < TEX_SIZE; i++) {
		texel(BLOCK_CRAFTING, FACE_TOP, i, 2, 78, 56, 32);
		texel(BLOCK_CRAFTING, FACE_TOP, i, 5, 78, 56, 32);
		texel(BLOCK_CRAFTING, FACE_TOP, 2, i, 78, 56, 32);
		texel(BLOCK_CRAFTING, FACE_TOP, 5, i, 78, 56, 32);
	}

	/* Tools leaning against the side. */
	for (int y = 2; y < 7; y++)
		texel(BLOCK_CRAFTING, FACE_SIDE, y, y, 96, 70, 40);
}

static void make_obsidian(int block, bool glowing)
{
	face_fill(block, FACE_SIDE, 26, 20, 38, 10);
	face_blobs(block, FACE_SIDE, 5, 52, 40, 76, 8);

	if (glowing) {
		/* Veins of trapped light, the mark of a spent reactor. */
		for (int i = 0; i < 8; i++) {
			uint32_t h = tex_hash(i * 8191u + 41u);
			texel(block, FACE_SIDE, h % TEX_SIZE, (h >> 9) % TEX_SIZE, 236, 118, 46);
		}
	}

	face_copy(block, FACE_TOP, FACE_SIDE);
	face_copy(block, FACE_BOTTOM, FACE_SIDE);
}

static void make_reactor(void)
{
	face_fill(BLOCK_REACTOR, FACE_SIDE, 62, 62, 70, 8);

	/* A bright core caged behind a dark frame. */
	for (int y = 2; y < 6; y++)
		for (int x = 2; x < 6; x++)
			texel(BLOCK_REACTOR, FACE_SIDE, x, y, 226, 178, 60);

	for (int i = 2; i < 6; i++) {
		texel(BLOCK_REACTOR, FACE_SIDE, i, 3, 250, 232, 150);
		texel(BLOCK_REACTOR, FACE_SIDE, 3, i, 250, 232, 150);
	}

	face_copy(BLOCK_REACTOR, FACE_TOP, FACE_SIDE);
	face_copy(BLOCK_REACTOR, FACE_BOTTOM, FACE_SIDE);
}

void blocks_init(void)
{
	make_grass();
	make_simple(BLOCK_DIRT, 134, 96, 67, 16);
	make_simple(BLOCK_STONE, 128, 128, 128, 14);

	make_simple(BLOCK_COBBLE, 130, 130, 130, 10);
	face_blobs(BLOCK_COBBLE, FACE_SIDE, 7, 100, 100, 100, 12);
	face_copy(BLOCK_COBBLE, FACE_TOP, FACE_SIDE);
	face_copy(BLOCK_COBBLE, FACE_BOTTOM, FACE_SIDE);

	make_simple(BLOCK_SAND, 218, 206, 156, 12);
	make_log();
	make_leaves();
	make_planks(BLOCK_PLANKS, 160, 126, 76);
	make_glass();
	make_fluid(BLOCK_WATER, 42, 90, 196, 22);
	make_fluid(BLOCK_LAVA, 208, 82, 24, 40);
	make_brick();

	make_ore(BLOCK_COAL_ORE, 40, 40, 40);
	make_ore(BLOCK_IRON_ORE, 200, 158, 120);
	make_ore(BLOCK_GOLD_ORE, 226, 196, 82);

	make_simple(BLOCK_GLOWSTONE, 206, 166, 88, 16);
	face_blobs(BLOCK_GLOWSTONE, FACE_SIDE, 8, 250, 232, 150, 6);
	face_copy(BLOCK_GLOWSTONE, FACE_TOP, FACE_SIDE);
	face_copy(BLOCK_GLOWSTONE, FACE_BOTTOM, FACE_SIDE);

	make_simple(BLOCK_BEDROCK, 68, 68, 72, 26);
	make_metal(BLOCK_GOLD_BLOCK, 226, 190, 70);
	make_metal(BLOCK_IRON_BLOCK, 208, 208, 212);
	make_crafting();
	make_obsidian(BLOCK_OBSIDIAN, false);
	make_obsidian(BLOCK_GLOWING_OBSIDIAN, true);
	make_simple(BLOCK_NETHERRACK, 122, 46, 42, 22);
	make_reactor();
}

/*
 * Draw a block as a small isometric cube.
 *
 * HOW THE SHAPE IS BUILT
 *
 * Three faces are visible on a cube seen from above and to the side: the
 * top as a diamond, and two sides as parallelograms. For a cube `size`
 * pixels wide we call half the width `a` and a quarter of the height `q`:
 *
 *        (a,0)
 *       .     .          the diamond is the top face
 *   (0,q)     (size,q)
 *       .     .          below it the two sides split at x = a
 *        (a,2q)
 *
 * For each pixel we work out which of the three regions it is in and then
 * undo the projection to find the texture coordinate. That inverse is the
 * only mildly interesting maths here, and it is written out below.
 */
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
				/*
				 * Inside the top diamond. The projection maps
				 * texture coordinates (tu, tv) to
				 *   px = a + (tu - tv) * a
				 *   py = q + (tu + tv - 1) * q
				 * so solving the pair of equations gives us
				 * back tu and tv.
				 */
				float fa = (float)dx / a;
				float fb = (float)dy / q;
				face = FACE_TOP;
				u = (int)((fa + fb + 1.0f) * 0.5f * TEX_SIZE);
				v = (int)((fb - fa + 1.0f) * 0.5f * TEX_SIZE);
			} else if (px < a) {
				/* Left face: a parallelogram sloping down. */
				int top = q + (px * q) / a;

				if (py < top || py >= top + 2 * q)
					continue;

				face = FACE_SIDE;
				u = (px * TEX_SIZE) / a;
				v = ((py - top) * TEX_SIZE) / (2 * q);
			} else {
				/* Right face: the mirror of the left one. */
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

			/* Sides are darker, and the right one darker still. */
			int shade = face == FACE_SIDE ? (px < a ? 24 : 17) : 32;
			c = color_mul(c, (uint8_t)clampi((shade * bright) >> 5, 1, 32));

			tft_draw_pixel(x0 + px, y0 + py, c);
		}
	}
}
