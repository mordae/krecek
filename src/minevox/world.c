#include "minevox.h"

#include <stdlib.h>
#include <string.h>

uint8_t world_data[WORLD_X * WORLD_Y * WORLD_Z / 2];
uint8_t world_height[WORLD_X][WORLD_Y];
uint8_t world_top = WORLD_Z;

struct light world_lights[MAX_LIGHTS];
int world_num_lights = 0;

struct light active_lights[MAX_ACTIVE_LIGHTS];
int num_active_lights = 0;

uint32_t world_seed = 1;
float world_time = 90.0f;

/*
 * Player edits, kept separately so that we can save a whole world
 * as a seed plus a short list of changes.
 */
#define MAX_EDITS 1300

static uint32_t edits[MAX_EDITS];
static int num_edits = 0;

static inline uint32_t edit_pack(int x, int y, int z, uint8_t block)
{
	return (uint32_t)x | ((uint32_t)y << 6) | ((uint32_t)z << 12) | ((uint32_t)block << 17);
}

/*
 * Value noise.
 */
static uint32_t hash_coords(int x, int y, int z, uint32_t seed)
{
	/* Unsigned throughout, signed overflow would be undefined. */
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
		amp *= 0.5f;
		x *= 2.0f;
		y *= 2.0f;
	}

	return sum / total;
}

/*
 * Terrain.
 */
static void place_tree(int x, int y, int base)
{
	int height = 4 + (int)(hash_coords(x, y, 3, world_seed) % 3);

	if (base + height + 2 >= WORLD_Z)
		return;

	/* Canopy first so that the trunk stays visible from below. */
	for (int dz = height - 2; dz <= height + 1; dz++) {
		int radius = (dz >= height) ? 1 : 2;

		for (int dy = -radius; dy <= radius; dy++) {
			for (int dx = -radius; dx <= radius; dx++) {
				if (abs(dx) == radius && abs(dy) == radius && radius > 1)
					continue;

				int lx = x + dx;
				int ly = y + dy;
				int lz = base + dz;

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
	memset(world_data, 0, sizeof(world_data));

	const float cx = WORLD_X * 0.5f;
	const float cy = WORLD_Y * 0.5f;
	const float radius = WORLD_X * 0.46f;

	/* Kept out of the stack, it would not fit there. */
	static uint8_t height_of[WORLD_X][WORLD_Y];

	for (int x = 0; x < WORLD_X; x++) {
		for (int y = 0; y < WORLD_Y; y++) {
			/* Rolling hills. */
			float h = fbm2(x * 0.055f, y * 0.055f, seed, 4);
			float ridge = fbm2(x * 0.021f, y * 0.021f, seed + 555u, 2);
			float land = (h - 0.42f) * 26.0f + (ridge - 0.5f) * 12.0f;

			/* Sink the rim of the map into the sea. */
			float dx = x - cx;
			float dy = y - cy;
			float dist = sqrtf(dx * dx + dy * dy) / radius;
			float falloff = 1.0f - smoothstep(clampf((dist - 0.55f) / 0.45f, 0, 1));

			int top =
				(int)(SEA_LEVEL + 1.0f + land * falloff - (1.0f - falloff) * 7.0f);
			top = clampi(top, 1, WORLD_Z - 6);
			height_of[x][y] = top;

			int stone_top = top - 3 - (int)(noise2(x * 0.3f, y * 0.3f, seed + 99u) * 2);

			for (int z = 0; z <= top; z++) {
				uint8_t block;

				if (z == 0) {
					block = BLOCK_BEDROCK;
				} else if (z <= stone_top) {
					block = BLOCK_STONE;
				} else if (z < top) {
					block = BLOCK_DIRT;
				} else if (top <= SEA_LEVEL + 1) {
					block = BLOCK_SAND;
				} else {
					block = BLOCK_GRASS;
				}

				world_put_raw(x, y, z, block);
			}

			for (int z = top + 1; z <= SEA_LEVEL; z++)
				world_put_raw(x, y, z, BLOCK_WATER);
		}
	}

	/* Carve caves out of the stone. */
	for (int x = 1; x < WORLD_X - 1; x++) {
		for (int y = 1; y < WORLD_Y - 1; y++) {
			int top = height_of[x][y];

			for (int z = 1; z < top - 1; z++) {
				float n = noise3(x * 0.14f, y * 0.14f, z * 0.22f, seed + 4242u);

				/* Keep the caves away from the surface and the sea. */
				if (n < 0.36f && z < SEA_LEVEL - 1 && z < top - 3)
					world_put_raw(x, y, z, BLOCK_AIR);
			}
		}
	}

	/* Sprinkle ores and a few glowing blocks into the dark. */
	for (int x = 0; x < WORLD_X; x++) {
		for (int y = 0; y < WORLD_Y; y++) {
			for (int z = 1; z < height_of[x][y] - 2; z++) {
				if (world_get_fast(x, y, z) != BLOCK_STONE)
					continue;

				uint32_t r = hash_coords(x, y, z, seed + 7777u) % 1000;

				if (r < 14)
					world_put_raw(x, y, z, BLOCK_COAL_ORE);
				else if (r < 22 && z < 10)
					world_put_raw(x, y, z, BLOCK_IRON_ORE);
				else if (r < 24 && z < 8)
					world_put_raw(x, y, z, BLOCK_GLOWSTONE);
			}
		}
	}

	/*
	 * Plant trees on dry grass. At most one per cell so that they have
	 * room to grow, and only where the forest noise says so, which
	 * leaves us with a mix of woods and meadows.
	 */
	const int cell = 6;

	for (int cx = 0; cx < WORLD_X / cell; cx++) {
		for (int cy = 0; cy < WORLD_Y / cell; cy++) {
			uint32_t h = hash_coords(cx, cy, 5, seed + 31u);

			int x = cx * cell + h % cell;
			int y = cy * cell + (h >> 8) % cell;

			if (x < 3 || y < 3 || x >= WORLD_X - 3 || y >= WORLD_Y - 3)
				continue;

			int top = height_of[x][y];

			if (top <= SEA_LEVEL + 1)
				continue;

			if (world_get_fast(x, y, top) != BLOCK_GRASS)
				continue;

			float density = fbm2(x * 0.045f, y * 0.045f, seed + 909u, 2);

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
	num_edits = 0;
}

void world_update_height(int x, int y)
{
	int z = WORLD_Z - 1;

	/* Leaves and glass let the daylight through. */
	while (z > 0 && !block_info[world_get_fast(x, y, z)].shades)
		z--;

	world_height[x][y] = z + 1;

	/* Anything at all, including leaves, raises the ceiling. */
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

				/* Buried lights are useless, skip them. */
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

void world_select_lights(float px, float py, float pz)
{
	float best[MAX_ACTIVE_LIGHTS];

	num_active_lights = 0;

	for (int i = 0; i < world_num_lights; i++) {
		float dx = world_lights[i].x + 0.5f - px;
		float dy = world_lights[i].y + 0.5f - py;
		float dz = world_lights[i].z + 0.5f - pz;
		float d2 = dx * dx + dy * dy + dz * dz;

		/* Anything further than this cannot brighten what we see. */
		if (d2 > 40.0f * 40.0f)
			continue;

		int at = num_active_lights;

		if (at < MAX_ACTIVE_LIGHTS) {
			num_active_lights++;
		} else {
			/* Replace the furthest one, if we are closer. */
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

static void light_forget(int x, int y, int z)
{
	for (int i = 0; i < world_num_lights; i++) {
		if (world_lights[i].x == x && world_lights[i].y == y && world_lights[i].z == z) {
			world_lights[i] = world_lights[--world_num_lights];
			return;
		}
	}
}

void world_set(int x, int y, int z, uint8_t block)
{
	if (!world_inside(x, y, z))
		return;

	uint8_t old = world_get_fast(x, y, z);

	if (old == block)
		return;

	world_put_raw(x, y, z, block);
	world_update_height(x, y);

	/* Keep the light list in sync without rescanning the world. */
	if (block_info[old].light)
		light_forget(x, y, z);

	if (block_info[block].light && world_num_lights < MAX_LIGHTS)
		world_lights[world_num_lights++] =
			(struct light){ x, y, z, block_info[block].light };

	uint32_t packed = edit_pack(x, y, z, block);
	uint32_t pos_mask = 0x1ffff;

	for (int i = 0; i < num_edits; i++) {
		if ((edits[i] & pos_mask) == (packed & pos_mask)) {
			edits[i] = packed;
			return;
		}
	}

	if (num_edits < MAX_EDITS)
		edits[num_edits++] = packed;
}

void world_forget_edits(void)
{
	num_edits = 0;
}

void world_find_spawn(float *out_x, float *out_y, float *out_z)
{
	/* Spiral out of the middle until we find dry land. */
	for (int r = 0; r < WORLD_X / 2; r++) {
		for (int dy = -r; dy <= r; dy++) {
			for (int dx = -r; dx <= r; dx++) {
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

bool world_raycast(float ox, float oy, float oz, float dx, float dy, float dz, float max_dist,
		   int *hx, int *hy, int *hz, int *px, int *py, int *pz)
{
	int mx = (int)floorf(ox);
	int my = (int)floorf(oy);
	int mz = (int)floorf(oz);

	int lx = mx, ly = my, lz = mz;

	float ddx = (dx == 0.0f) ? 1e30f : fabsf(1.0f / dx);
	float ddy = (dy == 0.0f) ? 1e30f : fabsf(1.0f / dy);
	float ddz = (dz == 0.0f) ? 1e30f : fabsf(1.0f / dz);

	int sx = dx < 0.0f ? -1 : 1;
	int sy = dy < 0.0f ? -1 : 1;
	int sz = dz < 0.0f ? -1 : 1;

	float tx = (dx < 0.0f ? (ox - mx) : (mx + 1.0f - ox)) * ddx;
	float ty = (dy < 0.0f ? (oy - my) : (my + 1.0f - oy)) * ddy;
	float tz = (dz < 0.0f ? (oz - mz) : (mz + 1.0f - oz)) * ddz;

	for (int i = 0; i < 64; i++) {
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

/*
 * Persistence.
 */
struct save_header {
	char magic[4];
	uint32_t seed;
	float x, y, z;
	float yaw, pitch;
	float time;
	uint8_t flying;
	uint8_t slot;
	uint8_t hotbar[9];
	uint16_t num_edits;
};

#define SAVE_MAGIC "MVX1"
#define SAVE_SLOT 0

/* Shared by both directions, one copy is plenty. */
static uint8_t save_buffer[SDK_SAVE_SLOT_SIZE];

void world_save(void)
{
	uint8_t *buffer = save_buffer;

	struct save_header hdr = {
		.seed = world_seed,
		.x = player.x,
		.y = player.y,
		.z = player.z,
		.yaw = player.yaw,
		.pitch = player.pitch,
		.time = world_time,
		.flying = player.flying,
		.slot = player.slot,
	};

	memcpy(hdr.magic, SAVE_MAGIC, 4);
	memcpy(hdr.hotbar, player.hotbar, sizeof(hdr.hotbar));

	int room = (SDK_SAVE_SLOT_SIZE - (int)sizeof(hdr)) / 3;
	int count = num_edits < room ? num_edits : room;
	hdr.num_edits = count;

	memset(buffer, 0, sizeof(save_buffer));
	memcpy(buffer, &hdr, sizeof(hdr));

	uint8_t *out = buffer + sizeof(hdr);

	for (int i = 0; i < count; i++) {
		out[i * 3 + 0] = edits[i] & 0xff;
		out[i * 3 + 1] = (edits[i] >> 8) & 0xff;
		out[i * 3 + 2] = (edits[i] >> 16) & 0xff;
	}

	sdk_save_write(SAVE_SLOT, buffer, sizeof(save_buffer));
}

bool world_load(void)
{
	uint8_t *buffer = save_buffer;

	sdk_save_read(SAVE_SLOT, buffer, sizeof(save_buffer));

	struct save_header hdr;
	memcpy(&hdr, buffer, sizeof(hdr));

	if (memcmp(hdr.magic, SAVE_MAGIC, 4))
		return false;

	int room = (SDK_SAVE_SLOT_SIZE - (int)sizeof(hdr)) / 3;

	if (hdr.num_edits > room)
		return false;

	world_generate(hdr.seed);

	const uint8_t *in = buffer + sizeof(hdr);

	for (int i = 0; i < hdr.num_edits; i++) {
		uint32_t packed = in[i * 3 + 0] | (in[i * 3 + 1] << 8) | (in[i * 3 + 2] << 16);

		int x = packed & 63;
		int y = (packed >> 6) & 63;
		int z = (packed >> 12) & 31;
		uint8_t block = (packed >> 17) & 15;

		world_put_raw(x, y, z, block);
		edits[i] = packed;
	}

	num_edits = hdr.num_edits;

	world_top = 0;

	for (int x = 0; x < WORLD_X; x++)
		for (int y = 0; y < WORLD_Y; y++)
			world_update_height(x, y);

	world_rebuild_lights();

	player.x = clampf(hdr.x, 1.0f, WORLD_X - 1.0f);
	player.y = clampf(hdr.y, 1.0f, WORLD_Y - 1.0f);
	player.z = clampf(hdr.z, 1.0f, WORLD_Z - 2.0f);
	player.yaw = hdr.yaw;
	player.pitch = clampf(hdr.pitch, -1.5f, 1.5f);
	player.vx = player.vy = player.vz = 0.0f;
	player.flying = hdr.flying;
	player.slot = clampi(hdr.slot, 0, 8);
	world_time = hdr.time;

	for (int i = 0; i < 9; i++)
		if (hdr.hotbar[i] < NUM_BLOCKS)
			player.hotbar[i] = hdr.hotbar[i];

	return true;
}
