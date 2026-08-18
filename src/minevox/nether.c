#include "minevox.h"

#include <stdlib.h>
#include <string.h>

/*
 * ===========================================================================
 * The nether reactor
 * ===========================================================================
 *
 * The old pocket and bedrock versions of Minecraft had no nether
 * dimension. Instead they had this: you built a small structure of gold
 * and cobblestone around a reactor core, tapped it, and the world around
 * you turned hostile. A spire of netherrack grew out of the ground, the
 * sky went red, pigmen appeared and treasure rained down. After a while it
 * burned out and left a shell of obsidian behind.
 *
 * That design is a gift for a handheld. A real second dimension would need
 * another 75 KiB of voxels, which we do not have. An event that happens in
 * the world we already have costs nothing but the code in this file.
 *
 * THE PATTERN
 *
 * Layers, bottom to top, with the core sitting in the middle of the
 * second one. G is a gold block, C is cobblestone, R is the reactor core
 * and a dot is anything at all.
 *
 *      z - 1            z (core)          z + 1
 *      G C G            G . G             . C .
 *      C C C            . R .             C C C
 *      G C G            G . G             . C .
 *
 * Eight gold blocks and ten cobblestone. Gold means finding gold ore deep
 * underground and smelting it, which is the point: the reactor is the end
 * of the survival progression, not the start of it.
 */

enum nether_state nether_state = NETHER_IDLE;
float nether_timer = 0.0f;

/* How long the event lasts, and how long the sky takes to calm down. */
#define NETHER_DURATION 100.0f
#define NETHER_FADE 6.0f

/* The spire that grows around the core. */
#define SPIRE_RADIUS 3
#define SPIRE_HEIGHT 12

/* Where the active reactor is, so we can turn it to obsidian afterwards. */
static int core_x, core_y, core_z;

/*
 * The pattern as a table rather than as code.
 *
 * Each layer is three rows of three characters, read in the order
 * -1, 0, +1 on z. Writing it out like this means the drawing above and the
 * check below can never drift apart, which is the usual failure mode when
 * a shape is spelled out in nested ifs.
 */
static const char *const pattern[3] = {
	"GCG"
	"CCC"
	"GCG", /* z - 1 */
	"G.G"
	".R."
	"G.G", /* z     */
	".C."
	"CCC"
	".C.", /* z + 1 */
};

void nether_init(void)
{
	nether_state = NETHER_IDLE;
	nether_timer = 0.0f;
}

void nether_reset(void)
{
	nether_init();
}

/* Which block does a pattern character stand for? */
static uint8_t pattern_block(char c)
{
	switch (c) {
	case 'G':
		return BLOCK_GOLD_BLOCK;
	case 'C':
		return BLOCK_COBBLE;
	case 'R':
		return BLOCK_REACTOR;
	default:
		return BLOCK_AIR;
	}
}

bool nether_pattern_ok(int x, int y, int z)
{
	if (world_get(x, y, z) != BLOCK_REACTOR)
		return false;

	/* Leave headroom, otherwise the spire has nowhere to grow. */
	if (z + SPIRE_HEIGHT >= WORLD_Z)
		return false;

	for (int layer = 0; layer < 3; layer++) {
		for (int row = 0; row < 3; row++) {
			for (int col = 0; col < 3; col++) {
				char c = pattern[layer][row * 3 + col];

				if (c == '.')
					continue;

				int bx = x + col - 1;
				int by = y + row - 1;
				int bz = z + layer - 1;

				if (world_get(bx, by, bz) != pattern_block(c))
					return false;
			}
		}
	}

	return true;
}

/*
 * Grow the spire.
 *
 * A hollow tower of netherrack with a floor every four levels and glowing
 * obsidian at the corners for light. The blocks go in with
 * world_set_quiet, which changes the world without writing to the save
 * file: the spire is temporary, and thousands of entries would swamp the
 * edit journal described in save.c.
 */
static void grow_spire(int x, int y, int z)
{
	for (int h = 0; h < SPIRE_HEIGHT; h++) {
		int bz = z - 1 + h;

		if (bz >= WORLD_Z)
			break;

		for (int dy = -SPIRE_RADIUS; dy <= SPIRE_RADIUS; dy++) {
			for (int dx = -SPIRE_RADIUS; dx <= SPIRE_RADIUS; dx++) {
				int bx = x + dx;
				int by = y + dy;

				if (!world_inside(bx, by, bz))
					continue;

				bool wall = abs(dx) == SPIRE_RADIUS || abs(dy) == SPIRE_RADIUS;
				bool floor = (h % 4) == 0;
				bool corner = abs(dx) == SPIRE_RADIUS && abs(dy) == SPIRE_RADIUS;

				/* A gap in the wall, so you can get out again. */
				if (wall && dx == 0 && h % 4 != 0 && h % 4 != 3)
					continue;

				if (corner && (h % 4) == 2) {
					world_set_quiet(bx, by, bz, BLOCK_GLOWING_OBSIDIAN);
					continue;
				}

				if (wall) {
					world_set_quiet(bx, by, bz, BLOCK_NETHERRACK);
					continue;
				}

				if (floor && h > 0) {
					/* Leave a hole to climb through. */
					if (dx == 0 && dy == 0)
						continue;

					world_set_quiet(bx, by, bz, BLOCK_NETHERRACK);
					continue;
				}

				/* Clear the inside so there is room to stand. */
				if (h > 0 && world_get(bx, by, bz) != BLOCK_AIR)
					world_set_quiet(bx, by, bz, BLOCK_AIR);
			}
		}
	}
}

/* Scatter the reward the reactor is famous for. */
static void drop_treasure(int x, int y, int z)
{
	items_spawn(x + 0.5f, y + 0.5f, z + 1.5f, BLOCK_GLOWSTONE, 4);
	items_spawn(x - 1.5f, y + 0.5f, z + 1.5f, ITEM_GOLD_INGOT, 3);
	items_spawn(x + 1.5f, y + 0.5f, z + 1.5f, BLOCK_NETHERRACK, 8);
	items_spawn(x + 0.5f, y - 1.5f, z + 1.5f, ITEM_IRON_INGOT, 2);
	items_spawn(x + 0.5f, y + 1.5f, z + 5.5f, ITEM_PORKCHOP, 3);
}

bool nether_activate(int x, int y, int z)
{
	if (nether_state != NETHER_IDLE)
		return false;

	if (!nether_pattern_ok(x, y, z))
		return false;

	core_x = x;
	core_y = y;
	core_z = z;

	/*
	 * Consume the structure. These go through world_set, not the quiet
	 * version, because using up the gold has to survive a reload. It is
	 * only nineteen blocks, which the edit journal can easily afford.
	 */
	for (int layer = 0; layer < 3; layer++) {
		for (int row = 0; row < 3; row++) {
			for (int col = 0; col < 3; col++) {
				char c = pattern[layer][row * 3 + col];

				if (c == '.')
					continue;

				int bx = x + col - 1;
				int by = y + row - 1;
				int bz = z + layer - 1;

				world_set(bx, by, bz,
					  c == 'R' ? BLOCK_GLOWING_OBSIDIAN : BLOCK_NETHERRACK);
			}
		}
	}

	grow_spire(x, y, z);
	drop_treasure(x, y, z);

	nether_state = NETHER_ACTIVE;
	nether_timer = NETHER_DURATION;

	/* Fill the place immediately, do not make the player wait. */
	for (int i = 0; i < 3; i++) {
		float ax = x + (float)(sdk_random() % 5) - 2.0f;
		float ay = y + (float)(sdk_random() % 5) - 2.0f;
		mobs_spawn(MOB_PIGMAN, ax, ay, z + 1.0f);
	}

	sfx_play("/i:square (f) >c<c>e<e>g");
	menu_toast("The reactor ignites");
	return true;
}

void nether_update(float dt)
{
	if (nether_state == NETHER_IDLE)
		return;

	nether_timer -= dt;

	if (nether_timer > 0.0f)
		return;

	if (nether_state == NETHER_ACTIVE) {
		/*
		 * Burn out. The glowing obsidian goes dull, which is the
		 * visual marker of a reactor that has already been used.
		 */
		for (int dz = -1; dz <= SPIRE_HEIGHT; dz++) {
			for (int dy = -SPIRE_RADIUS; dy <= SPIRE_RADIUS; dy++) {
				for (int dx = -SPIRE_RADIUS; dx <= SPIRE_RADIUS; dx++) {
					int bx = core_x + dx;
					int by = core_y + dy;
					int bz = core_z + dz;

					if (world_get(bx, by, bz) == BLOCK_GLOWING_OBSIDIAN)
						world_set_quiet(bx, by, bz, BLOCK_OBSIDIAN);
				}
			}
		}

		nether_state = NETHER_FADING;
		nether_timer = NETHER_FADE;
		menu_toast("The reactor burns out");
		return;
	}

	nether_state = NETHER_IDLE;
	nether_timer = 0.0f;
}

float nether_intensity(void)
{
	switch (nether_state) {
	case NETHER_ACTIVE:
		/* Ramp up over the first couple of seconds. */
		return clampf((NETHER_DURATION - nether_timer) * 0.5f, 0.0f, 1.0f);

	case NETHER_FADING:
		return clampf(nether_timer / NETHER_FADE, 0.0f, 1.0f);

	default:
		return 0.0f;
	}
}
