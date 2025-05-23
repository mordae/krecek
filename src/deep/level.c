#include "level.h"
#include <stdlib.h>
#include <string.h>
#include <tileset.png.h>

static uint32_t xorshift_seed;

static inline uint32_t xorshift(void)
{
	uint32_t x = xorshift_seed;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	xorshift_seed = x;
	return x;
}

static inline uint32_t xorshift_bits(unsigned bits)
{
	return xorshift() >> (32 - bits);
}

static inline bool is_vacant(Level *level, int x0, int y0)
{
	if (x0 <= 0 || x0 >= MAP_MAX || y0 <= 0 || y0 >= MAP_MAX)
		return false;

	for (int y = y0 - 1; y <= y0 + 1; y++)
		for (int x = x0 - 1; x <= x0 + 1; x++)
			if (level->map[y][x].tile_id)
				return false;

	return true;
}

static void flood(Level *level, uint8_t x, uint8_t y, uint8_t tile_id)
{
	static uint8_t sx[1024];
	static uint8_t sy[1024];

	if (x <= 0 || x >= MAP_MAX || y <= 0 || y >= MAP_MAX)
		return;

	int stack = 0;

	sx[stack] = x;
	sy[stack++] = y;

	do {
		x = sx[--stack];
		y = sy[stack];

		level->map[y][x].tile_id = tile_id;

		/* Make sure we don't overflow. */
		if (stack >= 1020)
			continue;

		if (y > 0 && 1 == level->map[y - 1][x].tile_id) {
			sx[stack] = x;
			sy[stack++] = y - 1;
		}

		if (y < MAP_MAX && 1 == level->map[y + 1][x].tile_id) {
			sx[stack] = x;
			sy[stack++] = MIN(MAP_MAX, y + 1);
		}

		if (x > 0 && 1 == level->map[y][x - 1].tile_id) {
			sx[stack] = x - 1;
			sy[stack++] = y;
		}

		if (x < MAP_MAX && 1 == level->map[y][x + 1].tile_id) {
			sx[stack] = x + 1;
			sy[stack++] = y;
		}
	} while (stack > 0);
}

void level_generate(Level *level)
{
	xorshift_seed = level->seed;

	memset(level->map, 0, sizeof(level->map));

	int half = 1 << (MAP_SIZE_BITS - 1);

	do {
		/* Start coordinates. Far from center. */
		level->sx = xorshift_bits(MAP_SIZE_BITS);
		level->sy = xorshift_bits(MAP_SIZE_BITS);
	} while ((abs(level->sx - half) + abs(level->sy - half) < half) ||
		 (level->sx < 3 || level->sx >= MAP_MAX - 3) ||
		 (level->sy < 3 || level->sy >= MAP_MAX - 3));

	do {
		/* End coordinates. Far from start. */
		level->ex = xorshift_bits(MAP_SIZE_BITS);
		level->ey = xorshift_bits(MAP_SIZE_BITS);
	} while ((abs(level->ex - level->sx) < 5 && abs(level->ey - level->sy) < 5) ||
		 (level->ex < 3 || level->ex >= MAP_MAX - 3) ||
		 (level->ey < 3 || level->ey >= MAP_MAX - 3));

	for (int y = level->sy - 2; y <= level->sy + 2; y++)
		for (int x = level->sx - 2; x <= level->sx + 2; x++)
			level->map[y][x].tile_id = 1;

	for (int y = level->ey - 2; y <= level->ey + 2; y++)
		for (int x = level->ex - 2; x <= level->ex + 2; x++)
			level->map[y][x].tile_id = 1;

	for (int ry = 1; ry < MAP_MAX; ry++) {
		for (int rx = 1; rx < MAP_MAX; rx++) {
			int wmax = xorshift_bits(ROOM_BITS);
			int hmax = xorshift_bits(ROOM_BITS);

			if (!is_vacant(level, rx, ry))
				continue;

			int w = 1;
			int h = 1;

			while (w < wmax && h < hmax) {
				if (w < wmax) {
					bool vacant = true;

					for (int y = ry; y < ry + h; y++)
						vacant &= is_vacant(level, rx + w, y);

					if (vacant) {
						w += 1;
					} else {
						wmax = w;
					}
				}

				if (h < hmax) {
					bool vacant = true;

					for (int x = rx; x < rx + w; x++)
						vacant &= is_vacant(level, x, ry + h);

					if (vacant) {
						h += 1;
					} else {
						hmax = h;
					}
				}
			}

			if (w <= 1 || h <= 1)
				continue;

			for (int y = ry; y < ry + h; y++)
				for (int x = rx; x < rx + w; x++)
					level->map[y][x].tile_id = 1;
		}
	}

	for (int y = 1; y < MAP_MAX; y++) {
		for (int x = 1; x < MAP_MAX; x++) {
			int t1 = level->map[y - 1][x - 1].tile_id;
			int t2 = level->map[y - 1][x + 0].tile_id;
			int t3 = level->map[y - 1][x + 1].tile_id;
			int t4 = level->map[y + 0][x - 1].tile_id;
			int t5 = level->map[y + 0][x + 0].tile_id;
			int t6 = level->map[y + 0][x + 1].tile_id;
			int t7 = level->map[y + 1][x - 1].tile_id;
			int t8 = level->map[y + 1][x + 0].tile_id;
			int t9 = level->map[y + 1][x + 1].tile_id;

			int mask = (!!t1 << 8) | (!!t2 << 7) | (!!t3 << 6) | (!!t4 << 5) |
				   (!!t5 << 4) | (!!t6 << 3) | (!!t7 << 2) | (!!t8 << 1) |
				   (!!t9 << 0);

			int masks[] = { 0b001111001, 0b111010010, 0b111110111, 0b111110111,
					0b010010111 };
			int valid[] = { 0b000100000, 0b000000010, 0b111100100, 0b111100000,
					0b010000000 };
			int len = sizeof(valid) / sizeof(*valid);

			for (int i = 0; i < len; i++) {
				if (valid[i] == (mask & masks[i])) {
					level->map[y][x].tile_id = 1;
					break;
				}
			}
		}
	}

	level->map[level->sy][level->sx].tile_id = 1;
	flood(level, level->sx, level->sy, 2);

	level->map[level->ey][level->ex].tile_id = 1;
	flood(level, level->ex, level->ey, 3);

	unsigned offset = 0;
	int attempts = 0;

	while (true) {
		attempts++;

		offset += 0x9e3779b9u;
		int tile = offset % (MAP_SIZE * MAP_SIZE);
		int y = tile >> MAP_SIZE_BITS;
		int x = tile & (MAP_SIZE - 1);

		if (x <= 1 || x >= MAP_MAX || y <= 1 || y >= MAP_MAX)
			continue;

		if (level->map[y][x].tile_id)
			continue;

		int t2 = level->map[y - 1][x].tile_id;
		int t4 = level->map[y][x - 1].tile_id;
		int t6 = level->map[y][x + 1].tile_id;
		int t8 = level->map[y + 1][x].tile_id;

		if (t2 && t8 && t2 != t8) {
			if (t2 + t8 == 5) {
				if (attempts < 1024)
					continue;

				level->map[y][x].tile_id = 2;
				break;
			}

			level->map[y][x].tile_id = 1;
			flood(level, x, y, MAX(t2, t8));
		}

		if (t4 && t6 && t4 != t6) {
			if (t4 + t6 == 5) {
				if (attempts < 1024)
					continue;

				level->map[y][x].tile_id = 2;
				break;
			}

			level->map[y][x].tile_id = 1;
			flood(level, x, y, MAX(t4, t6));
		}
	}

	for (int y = 0; y <= MAP_MAX; y++) {
		for (int x = 0; x <= MAP_MAX; x++) {
			if (1 == level->map[y][x].tile_id) {
				level->map[y][x].tile_id = 0;
			} else if (level->map[y][x].tile_id > 1) {
				level->map[y][x].tile_id = 1;
			} else {
				level->map[y][x].solid = 1;
			}
		}
	}

	level->map[level->sy][level->sx].tile_id = 2;
	level->map[level->ey][level->ex].tile_id = 3;
}
