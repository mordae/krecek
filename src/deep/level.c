#include "level.h"
#include <stdlib.h>
#include <string.h>
#include <tileset.png.h>

static uint8_t scratch[MAP_SIZE][MAP_SIZE];

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

static inline bool is_vacant(int x0, int y0)
{
	if (x0 <= 0 || x0 >= MAP_MAX || y0 <= 0 || y0 >= MAP_MAX)
		return false;

	for (int y = y0 - 1; y <= y0 + 1; y++)
		for (int x = x0 - 1; x <= x0 + 1; x++)
			if (scratch[y][x])
				return false;

	return true;
}

static void flood(uint8_t x, uint8_t y, uint8_t tile_id)
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

		scratch[y][x] = tile_id;

		/* Make sure we don't overflow. */
		if (stack >= 1020)
			continue;

		if (y > 0 && 1 == scratch[y - 1][x]) {
			sx[stack] = x;
			sy[stack++] = y - 1;
		}

		if (y < MAP_MAX && 1 == scratch[y + 1][x]) {
			sx[stack] = x;
			sy[stack++] = MIN(MAP_MAX, y + 1);
		}

		if (x > 0 && 1 == scratch[y][x - 1]) {
			sx[stack] = x - 1;
			sy[stack++] = y;
		}

		if (x < MAP_MAX && 1 == scratch[y][x + 1]) {
			sx[stack] = x + 1;
			sy[stack++] = y;
		}
	} while (stack > 0);
}

void level_generate(Level *level)
{
	xorshift_seed = level->seed;
	memset(scratch, 0, sizeof(scratch));

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
			scratch[y][x] = 1;

	for (int y = level->ey - 2; y <= level->ey + 2; y++)
		for (int x = level->ex - 2; x <= level->ex + 2; x++)
			scratch[y][x] = 1;

	for (int ry = 1; ry < MAP_MAX; ry++) {
		for (int rx = 1; rx < MAP_MAX; rx++) {
			int wmax = xorshift_bits(ROOM_BITS);
			int hmax = xorshift_bits(ROOM_BITS);

			if (!is_vacant(rx, ry))
				continue;

			int w = 1;
			int h = 1;

			while (w < wmax && h < hmax) {
				if (w < wmax) {
					bool vacant = true;

					for (int y = ry; y < ry + h; y++)
						vacant &= is_vacant(rx + w, y);

					if (vacant) {
						w += 1;
					} else {
						wmax = w;
					}
				}

				if (h < hmax) {
					bool vacant = true;

					for (int x = rx; x < rx + w; x++)
						vacant &= is_vacant(x, ry + h);

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
					scratch[y][x] = 1;
		}
	}

	for (int y = 1; y < MAP_MAX; y++) {
		for (int x = 1; x < MAP_MAX; x++) {
			int t7 = scratch[y - 1][x - 1];
			int t8 = scratch[y - 1][x + 0];
			int t9 = scratch[y - 1][x + 1];
			int t4 = scratch[y + 0][x - 1];
			int t5 = scratch[y + 0][x + 0];
			int t6 = scratch[y + 0][x + 1];
			int t1 = scratch[y + 1][x - 1];
			int t2 = scratch[y + 1][x + 0];
			int t3 = scratch[y + 1][x + 1];

			int mask = (!!t7 << 8) | (!!t8 << 7) | (!!t9 << 6) | (!!t4 << 5) |
				   (!!t5 << 4) | (!!t6 << 3) | (!!t1 << 2) | (!!t2 << 1) |
				   (!!t3 << 0);

			int masks[] = { 0b001111001, 0b111010010, 0b111110111, 0b111110111,
					0b010010111 };
			int valid[] = { 0b000100000, 0b000000010, 0b111100100, 0b111100000,
					0b010000000 };
			int len = sizeof(valid) / sizeof(*valid);

			for (int i = 0; i < len; i++) {
				if (valid[i] == (mask & masks[i])) {
					scratch[y][x] = 1;
					break;
				}
			}
		}
	}

	scratch[level->sy][level->sx] = 1;
	flood(level->sx, level->sy, 2);

	scratch[level->ey][level->ex] = 1;
	flood(level->ex, level->ey, 3);

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

		if (scratch[y][x])
			continue;

		int t8 = scratch[y - 1][x];
		int t4 = scratch[y][x - 1];
		int t6 = scratch[y][x + 1];
		int t2 = scratch[y + 1][x];

		if (t2 && t8 && t2 != t8) {
			if (t2 + t8 == 5) {
				if (attempts < 1024)
					continue;

				scratch[y][x] = 2;
				break;
			}

			scratch[y][x] = 1;
			flood(x, y, MAX(t2, t8));
		}

		if (t4 && t6 && t4 != t6) {
			if (t4 + t6 == 5) {
				if (attempts < 1024)
					continue;

				scratch[y][x] = 2;
				break;
			}

			scratch[y][x] = 1;
			flood(x, y, MAX(t4, t6));
		}
	}

	/*
	 * Table converting 4b wall present/missing state (8462)
	 * into central tile id to actually use.
	 */
	const uint8_t tile_lut[16] = {
		0x00, // 0000  ----
		0x00, // 0001  ---2
		0x07, // 0010  --6-
		0x01, // 0011  --62
		0x09, // 0100  -4--
		0x04, // 0101  -4-2
		0x08, // 0110  -46-
		0x0a, // 0111  -462
		0x0b, // 1000  8---
		0x00, // 1001  8--2
		0x03, // 1010  8-6-
		0x02, // 1011  8-62
		0x06, // 1100  84--
		0x05, // 1101  84-2
		0x08, // 1110  846-
		0x0a, // 1111  8462
	};

	for (int y = 0; y <= MAP_MAX; y++) {
		for (int x = 0; x <= MAP_MAX; x++) {
			if (scratch[y][x]) {
				level->map[y][x].tile_id = 12 + xorshift_bits(2);
				level->map[y][x].solid = 0;
			} else {
				int t8 = y > 0 ? !scratch[y - 1][x] : 0;
				int t2 = y < MAP_MAX ? !scratch[y + 1][x] : 0;
				int t4 = x > 0 ? !scratch[y][x - 1] : 0;
				int t6 = x < MAP_MAX ? !scratch[y][x + 1] : 0;

				uint8_t key = (t8 << 3) | (t4 << 2) | (t6 << 1) | (t2 << 0);

				level->map[y][x].tile_id = tile_lut[key];
				level->map[y][x].solid = 1;
			}
		}
	}

	level->map[level->sy][level->sx].tile_id = 16;
	level->map[level->ey][level->ex].tile_id = 17;
}
