#include "minevox.h"

#include <stdlib.h>
#include <string.h>

/*
 * ===========================================================================
 * The world: storage, generation and ray casting
 * ===========================================================================
 *
 * The voxel volume itself is a plain three dimensional array flattened
 * into one dimension. Everything else in this file either fills it in at
 * the start of a game or answers questions about it afterwards.
 */

uint8_t world_data[WORLD_X * WORLD_Y * WORLD_Z];
uint8_t world_height[WORLD_X][WORLD_Y];
uint8_t world_top = WORLD_Z;

struct light world_lights[MAX_LIGHTS];
int world_num_lights = 0;

struct light active_lights[MAX_ACTIVE_LIGHTS];
int num_active_lights = 0;

uint32_t world_seed = 1;
float world_time = 90.0f;

/*
 * ---------------------------------------------------------------------------
 * Noise
 * ---------------------------------------------------------------------------
 *
 * Terrain comes from VALUE NOISE, which is the simplest way to make a
 * landscape that looks natural but is completely reproducible from a seed.
 *
 * The idea in three steps:
 *
 *   1. Imagine a grid of points, each holding a random height. We do not
 *      store that grid, we compute a point's value on demand by hashing
 *      its coordinates, which costs no memory at all.
 *   2. To find the height between grid points, blend the four corners
 *      around you.
 *   3. Blending linearly leaves visible creases along the grid lines, so
 *      first bend the blending factor with a smoothstep curve, which
 *      flattens the slope at each end and hides the seams.
 *
 * One layer of that gives gentle rolling ground. Adding several layers at
 * doubling frequency and halving strength, which is called FRACTIONAL
 * BROWNIAN MOTION, adds detail without losing the large shapes. That is
 * the fbm2 function below, and it is the whole of our terrain generator.
 */
static uint32_t hash_coords(int x, int y, int z, uint32_t seed)
{
	/*
	 * Everything is unsigned on purpose. Signed overflow is undefined
	 * behaviour in C, and the optimiser will happily exploit that; the
	 * first version of this file had exactly that bug and the compiler
	 * quietly rewrote the terrain generator.
	 */
	uint32_t h = seed;
	h ^= (uint32_t)x * 374761393u;
	h ^= (uint32_t)y * 668265263u;
	h ^= (uint32_t)z * 2246822519u;
	h ^= h >> 13;
	h *= 1274126177u;
	h ^= h >> 16;
	return h;
}

static float lattice(int x, int y, int z, uint32_t seed)
{
	return (hash_coords(x, y, z, seed) & 0xffff) * (1.0f / 65535.0f);
}

/* Eases from 0 to 1 with zero slope at both ends: 3t^2 - 2t^3. */
static float smoothstep(float t)
{
	return t * t * (3.0f - 2.0f * t);
}

static float noise2(float x, float y, uint32_t seed)
{
	int ix = (int)floorf(x);
	int iy = (int)floorf(y);
	float fx = smoothstep(x - ix);
	float fy = smoothstep(y - iy);

	float a = lattice(ix, iy, 0, seed);
	float b = lattice(ix + 1, iy, 0, seed);
	float c = lattice(ix, iy + 1, 0, seed);
	float d = lattice(ix + 1, iy + 1, 0, seed);

	float ab = a + (b - a) * fx;
	float cd = c + (d - c) * fx;
	return ab + (cd - ab) * fy;
}

/* The same idea in three dimensions, used to carve caves. */
static float noise3(float x, float y, float z, uint32_t seed)
{
	int ix = (int)floorf(x);
	int iy = (int)floorf(y);
	int iz = (int)floorf(z);
	float fx = smoothstep(x - ix);
	float fy = smoothstep(y - iy);
	float fz = smoothstep(z - iz);

	float v = 0.0f;

	for (int dz = 0; dz < 2; dz++) {
		float wz = dz ? fz : 1.0f - fz;

		for (int dy = 0; dy < 2; dy++) {
			float wy = dy ? fy : 1.0f - fy;

			for (int dx = 0; dx < 2; dx++) {
				float wx = dx ? fx : 1.0f - fx;
				v += wx * wy * wz * lattice(ix + dx, iy + dy, iz + dz, seed);
			}
		}
	}

	return v;
}

static float fbm2(float x, float y, uint32_t seed, int octaves)
{
	float sum = 0.0f;
	float amp = 1.0f;
	float total = 0.0f;

	for (int i = 0; i < octaves; i++) {
		sum += amp * noise2(x, y, seed + i * 7919u);
		total += amp;

		/* Each layer is twice as detailed and half as strong. */
		amp *= 0.5f;
		x *= 2.0f;
		y *= 2.0f;
	}

	return sum / total;
}

/*
 * ---------------------------------------------------------------------------
 * Terrain
 * ---------------------------------------------------------------------------
 */
static void place_tree(int x, int y, int base)
{
	int height = 4 + (int)(hash_coords(x, y, 3, world_seed) % 3);

	if (base + height + 2 >= WORLD_Z)
		return;

	/* Canopy first, so the trunk is still visible from underneath. */
	for (int dz = height - 2; dz <= height + 1; dz++) {
		int radius = (dz >= height) ? 1 : 2;

		for (int dy = -radius; dy <= radius; dy++) {
			for (int dx = -radius; dx <= radius; dx++) {
				/* Clip the corners so the crown looks round. */
				if (abs(dx) == radius && abs(dy) == radius && radius > 1)
					continue;

				int lx = x + dx, ly = y + dy, lz = base + dz;

				if (!world_inside(lx, ly, lz))
					continue;

				if (world_get_fast(lx, ly, lz) != BLOCK_AIR)
					continue;

				world_put_raw(lx, ly, lz, BLOCK_LEAVES);
			}
		}
	}

	for (int dz = 0; dz < height; dz++)
		world_put_raw(x, y, base + dz, BLOCK_LOG);
}

void world_generate(uint32_t seed)
{
	world_seed = seed;
	memset(world_data, BLOCK_AIR, sizeof(world_data));

	const float cx = WORLD_X * 0.5f;
	const float cy = WORLD_Y * 0.5f;
	const float radius = WORLD_X * 0.46f;

	/*
	 * The surface height of every column, kept while we build so that
	 * later passes do not have to search for it. Static rather than a
	 * local because 2 KiB will not fit on the console's small stack.
	 */
	static uint8_t height_of[WORLD_X][WORLD_Y];

	for (int x = 0; x < WORLD_X; x++) {
		for (int y = 0; y < WORLD_Y; y++) {
			/* Two layers of noise: fine hills over broad ridges. */
			float h = fbm2(x * 0.070f, y * 0.070f, seed, 4);
			float ridge = fbm2(x * 0.028f, y * 0.028f, seed + 555u, 2);
			float land = (h - 0.42f) * 26.0f + (ridge - 0.5f) * 12.0f;

			/*
			 * Sink the rim of the map into the sea. Without this
			 * the world would end in a cliff wall, and an island
			 * explains the edge of the map far more gracefully.
			 */
			float dx = x - cx;
			float dy = y - cy;
			float dist = sqrtf(dx * dx + dy * dy) / radius;
			float falloff = 1.0f - smoothstep(clampf((dist - 0.55f) / 0.45f, 0, 1));

			int top =
				(int)(SEA_LEVEL + 1.0f + land * falloff - (1.0f - falloff) * 7.0f);
			top = clampi(top, 1, WORLD_Z - 12);
			height_of[x][y] = top;

			int stone_top = top - 3 - (int)(noise2(x * 0.3f, y * 0.3f, seed + 99u) * 2);

			for (int z = 0; z <= top; z++) {
				uint8_t block;

				if (z == 0)
					block = BLOCK_BEDROCK;
				else if (z <= stone_top)
					block = BLOCK_STONE;
				else if (z < top)
					block = BLOCK_DIRT;
				else if (top <= SEA_LEVEL + 1)
					block = BLOCK_SAND;
				else
					block = BLOCK_GRASS;

				world_put_raw(x, y, z, block);
			}

			/* Flood everything below sea level that is still air. */
			for (int z = top + 1; z <= SEA_LEVEL; z++)
				world_put_raw(x, y, z, BLOCK_WATER);
		}
	}

	/*
	 * Caves. Where the three dimensional noise dips below a threshold we
	 * remove the stone, which produces winding connected tunnels rather
	 * than isolated bubbles. We keep them away from the surface and the
	 * sea floor so that nothing floods.
	 */
	for (int x = 1; x < WORLD_X - 1; x++) {
		for (int y = 1; y < WORLD_Y - 1; y++) {
			int top = height_of[x][y];

			for (int z = 1; z < top - 1; z++) {
				float n = noise3(x * 0.16f, y * 0.16f, z * 0.24f, seed + 4242u);

				if (n < 0.36f && z < SEA_LEVEL - 1 && z < top - 3)
					world_put_raw(x, y, z, BLOCK_AIR);
			}
		}
	}

	/*
	 * Ores, and the occasional pool of lava to make the deep dangerous.
	 * Rarity is just a number out of a thousand, and the depth limits
	 * are what make it worth digging further down.
	 */
	for (int x = 0; x < WORLD_X; x++) {
		for (int y = 0; y < WORLD_Y; y++) {
			for (int z = 1; z < height_of[x][y] - 2; z++) {
				if (world_get_fast(x, y, z) != BLOCK_STONE)
					continue;

				uint32_t r = hash_coords(x, y, z, seed + 7777u) % 1000;

				if (r < 18)
					world_put_raw(x, y, z, BLOCK_COAL_ORE);
				else if (r < 28 && z < 11)
					world_put_raw(x, y, z, BLOCK_IRON_ORE);
				else if (r < 33 && z < 7)
					world_put_raw(x, y, z, BLOCK_GOLD_ORE);
				else if (r < 35 && z < 6)
					world_put_raw(x, y, z, BLOCK_GLOWSTONE);
			}

			/* Lava settles in the lowest hollows. */
			for (int z = 1; z < 5; z++) {
				if (world_get_fast(x, y, z) != BLOCK_AIR)
					continue;

				if (hash_coords(x, y, z, seed + 2024u) % 100 < 55)
					world_put_raw(x, y, z, BLOCK_LAVA);
			}
		}
	}

	/*
	 * Trees, at most one per six by six cell so their crowns have room,
	 * and only where a slow noise field says a forest belongs. That
	 * second test is what creates clearings instead of an even carpet.
	 */
	const int cell = 6;

	for (int cellx = 0; cellx < WORLD_X / cell; cellx++) {
		for (int celly = 0; celly < WORLD_Y / cell; celly++) {
			uint32_t h = hash_coords(cellx, celly, 5, seed + 31u);

			int x = cellx * cell + h % cell;
			int y = celly * cell + (h >> 8) % cell;

			if (x < 3 || y < 3 || x >= WORLD_X - 3 || y >= WORLD_Y - 3)
				continue;

			int top = height_of[x][y];

			if (top <= SEA_LEVEL + 1)
				continue;

			if (world_get_fast(x, y, top) != BLOCK_GRASS)
				continue;

			float density = fbm2(x * 0.05f, y * 0.05f, seed + 909u, 2);

			if (((h >> 16) & 255) > (uint32_t)(density * 200.0f))
				continue;

			place_tree(x, y, top + 1);
		}
	}

	world_top = 0;

	for (int x = 0; x < WORLD_X; x++)
		for (int y = 0; y < WORLD_Y; y++)
			world_update_height(x, y);

	world_rebuild_lights();
}

/*
 * ---------------------------------------------------------------------------
 * Derived data
 * ---------------------------------------------------------------------------
 *
 * Two things are cached because recomputing them per pixel would be far
 * too slow: how deep a block is buried, and where the light sources are.
 */
void world_update_height(int x, int y)
{
	int z = WORLD_Z - 1;

	/* Leaves and glass let daylight through, so they do not count. */
	while (z > 0 && !block_info[world_get_fast(x, y, z)].shades)
		z--;

	world_height[x][y] = z + 1;

	/*
	 * Separately, remember the highest block of any kind. Rays heading
	 * upwards past this can stop immediately, which saves a good deal
	 * of work on every pixel of sky.
	 */
	z = WORLD_Z - 1;

	while (z > 0 && !world_get_fast(x, y, z))
		z--;

	if (z + 1 > world_top)
		world_top = z + 1;
}

void world_rebuild_lights(void)
{
	world_num_lights = 0;

	for (int x = 0; x < WORLD_X && world_num_lights < MAX_LIGHTS; x++) {
		for (int y = 0; y < WORLD_Y && world_num_lights < MAX_LIGHTS; y++) {
			for (int z = 0; z < WORLD_Z; z++) {
				uint8_t block = world_get_fast(x, y, z);

				if (!block_info[block].light)
					continue;

				/* A light with rock on all six sides is wasted. */
				if (block_is_opaque(world_get(x, y, z + 1)) &&
				    block_is_opaque(world_get(x, y, z - 1)) &&
				    block_is_opaque(world_get(x - 1, y, z)) &&
				    block_is_opaque(world_get(x + 1, y, z)) &&
				    block_is_opaque(world_get(x, y - 1, z)) &&
				    block_is_opaque(world_get(x, y + 1, z)))
					continue;

				if (world_num_lights >= MAX_LIGHTS)
					break;

				world_lights[world_num_lights++] =
					(struct light){ x, y, z, block_info[block].light };
			}
		}
	}
}

/*
 * Pick the handful of lights nearest the camera.
 *
 * The renderer loops over these for every lit pixel, so the count has to
 * stay tiny. Choosing them once per frame instead of per pixel is the
 * whole trick: distant lights cannot brighten anything we can see anyway.
 */
void world_select_lights(float px, float py, float pz)
{
	float best[MAX_ACTIVE_LIGHTS];

	num_active_lights = 0;

	for (int i = 0; i < world_num_lights; i++) {
		float dx = world_lights[i].x + 0.5f - px;
		float dy = world_lights[i].y + 0.5f - py;
		float dz = world_lights[i].z + 0.5f - pz;
		float d2 = dx * dx + dy * dy + dz * dz;

		if (d2 > 40.0f * 40.0f)
			continue;

		int at = num_active_lights;

		if (at < MAX_ACTIVE_LIGHTS) {
			num_active_lights++;
		} else {
			/* Full: throw out the furthest, if we beat it. */
			int worst = 0;

			for (int j = 1; j < MAX_ACTIVE_LIGHTS; j++)
				if (best[j] > best[worst])
					worst = j;

			if (best[worst] <= d2)
				continue;

			at = worst;
		}

		best[at] = d2;
		active_lights[at] = world_lights[i];
	}
}

/* Change a block and keep everything that depends on it up to date. */
static void world_set_common(int x, int y, int z, uint8_t block, bool record)
{
	if (!world_inside(x, y, z))
		return;

	uint8_t old = world_get_fast(x, y, z);

	if (old == block)
		return;

	world_put_raw(x, y, z, block);
	world_update_height(x, y);

	/*
	 * Keep the light list in step incrementally. Rescanning the whole
	 * volume takes about a hundred thousand reads, which is a visible
	 * stutter on the console every time you place a block.
	 */
	if (block_info[old].light) {
		for (int i = 0; i < world_num_lights; i++) {
			if (world_lights[i].x == x && world_lights[i].y == y &&
			    world_lights[i].z == z) {
				world_lights[i] = world_lights[--world_num_lights];
				break;
			}
		}
	}

	if (block_info[block].light && world_num_lights < MAX_LIGHTS)
		world_lights[world_num_lights++] =
			(struct light){ x, y, z, block_info[block].light };

	if (record)
		save_note_edit(x, y, z, block);
}

void world_set(int x, int y, int z, uint8_t block)
{
	world_set_common(x, y, z, block, true);
}

/*
 * Change a block without writing it to the save file.
 *
 * The nether spire uses this: it is thousands of blocks, it would swamp
 * the edit list, and it is meant to be temporary anyway.
 */
void world_set_quiet(int x, int y, int z, uint8_t block)
{
	world_set_common(x, y, z, block, false);
}

void world_find_spawn(float *out_x, float *out_y, float *out_z)
{
	/* Spiral outwards from the middle until we find dry land. */
	for (int r = 0; r < WORLD_X / 2; r++) {
		for (int dy = -r; dy <= r; dy++) {
			for (int dx = -r; dx <= r; dx++) {
				/* Only the rim of the square is new each round. */
				if (abs(dx) != r && abs(dy) != r)
					continue;

				int x = WORLD_X / 2 + dx;
				int y = WORLD_Y / 2 + dy;

				if (!world_inside(x, y, 0))
					continue;

				int top = world_height[x][y];

				if (top <= SEA_LEVEL + 1 || top + 3 >= WORLD_Z)
					continue;

				uint8_t under = world_get(x, y, top - 1);

				if (under != BLOCK_GRASS && under != BLOCK_SAND)
					continue;

				*out_x = x + 0.5f;
				*out_y = y + 0.5f;
				*out_z = top;
				return;
			}
		}
	}

	*out_x = WORLD_X / 2 + 0.5f;
	*out_y = WORLD_Y / 2 + 0.5f;
	*out_z = SEA_LEVEL + 2;
}

/*
 * ---------------------------------------------------------------------------
 * Ray casting
 * ---------------------------------------------------------------------------
 *
 * This is the grid traversal that the whole game is built on, so it is
 * worth understanding properly. It is usually called a DDA, and the
 * version for voxel grids works like this.
 *
 * A ray leaves a point and travels in a direction. It only ever crosses
 * from one cube to the next through a wall perpendicular to x, y or z. So
 * instead of stepping along the ray by small amounts and hoping not to
 * miss anything, we jump exactly from wall to wall:
 *
 *   - For each axis, work out how far along the ray we must travel to
 *     cross the next wall on that axis. Call those tx, ty and tz.
 *   - Whichever is smallest is the wall we hit first, so step to that
 *     cube and add the constant distance between walls on that axis.
 *   - Repeat.
 *
 * The distance between walls on the x axis is 1/|dx| for a unit direction,
 * which is where the divisions at the top come from. No cube is ever
 * skipped and none is visited twice, and the loop body is a couple of
 * comparisons and an add.
 */
bool world_raycast(float ox, float oy, float oz, float dx, float dy, float dz, float max_dist,
		   int *hx, int *hy, int *hz, int *px, int *py, int *pz)
{
	int mx = (int)floorf(ox);
	int my = (int)floorf(oy);
	int mz = (int)floorf(oz);

	int lx = mx, ly = my, lz = mz;

	/* Distance along the ray between two walls on each axis. */
	float ddx = (dx == 0.0f) ? 1e30f : fabsf(1.0f / dx);
	float ddy = (dy == 0.0f) ? 1e30f : fabsf(1.0f / dy);
	float ddz = (dz == 0.0f) ? 1e30f : fabsf(1.0f / dz);

	int sx = dx < 0.0f ? -1 : 1;
	int sy = dy < 0.0f ? -1 : 1;
	int sz = dz < 0.0f ? -1 : 1;

	/* Distance to the first wall, which depends where in the cube we start. */
	float tx = (dx < 0.0f ? (ox - mx) : (mx + 1.0f - ox)) * ddx;
	float ty = (dy < 0.0f ? (oy - my) : (my + 1.0f - oy)) * ddy;
	float tz = (dz < 0.0f ? (oz - mz) : (mz + 1.0f - oz)) * ddz;

	for (int i = 0; i < 96; i++) {
		/* Remember where we were: that is where a block would go. */
		lx = mx;
		ly = my;
		lz = mz;

		float dist;

		if (tx < ty && tx < tz) {
			mx += sx;
			dist = tx;
			tx += ddx;
		} else if (ty < tz) {
			my += sy;
			dist = ty;
			ty += ddy;
		} else {
			mz += sz;
			dist = tz;
			tz += ddz;
		}

		if (dist > max_dist)
			break;

		if (!world_inside(mx, my, mz))
			break;

		uint8_t block = world_get_fast(mx, my, mz);

		/* You cannot aim at air, and you cannot aim at a liquid. */
		if (block == BLOCK_AIR || block_info[block].liquid)
			continue;

		if (hx)
			*hx = mx;
		if (hy)
			*hy = my;
		if (hz)
			*hz = mz;
		if (px)
			*px = lx;
		if (py)
			*py = ly;
		if (pz)
			*pz = lz;

		return true;
	}

	return false;
}
