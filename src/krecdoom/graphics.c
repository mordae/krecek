#include "graphics.h"
#include "common.h"
#include <stdlib.h>
#include <tft.h>

void draw_vertical_line(int x, int y1, int y2, uint16_t color)
{
	if (x < 0 || x >= SCREEN_WIDTH)
		return; // Clip X

	if (y1 > y2) {
		int temp = y1;
		y1 = y2;
		y2 = temp;
	}

	if (y1 < 0)
		y1 = 0;
	if (y2 >= SCREEN_HEIGHT)
		y2 = SCREEN_HEIGHT - 1;

	for (int y = y1; y <= y2; y++) {
		tft_draw_pixel(x, y, color);
	}
}

void draw_horizontal_line(int x1, int x2, int y, uint16_t color)
{
	if (y < 0 || y >= SCREEN_HEIGHT)
		return; // Clip Y

	if (x1 > x2) {
		int temp = x1;
		x1 = x2;
		x2 = temp;
	}

	if (x1 < 0)
		x1 = 0;
	if (x2 >= SCREEN_WIDTH)
		x2 = SCREEN_WIDTH - 1;

	for (int x = x1; x <= x2; x++) {
		tft_draw_pixel(x, y, color);
	}
}

void draw_line(int x1, int y1, int x2, int y2, uint16_t color)
{
	int dx = abs(x2 - x1);
	int dy = abs(y2 - y1);
	int sx = (x1 < x2) ? 1 : -1;
	int sy = (y1 < y2) ? 1 : -1;
	int err = dx - dy;

	while (1) {
		tft_draw_pixel(x1, y1, color);

		if (x1 == x2 && y1 == y2)
			break;
		int e2 = 2 * err;
		if (e2 > -dy) {
			err -= dy;
			x1 += sx;
		}
		if (e2 < dx) {
			err += dx;
			y1 += sy;
		}
	}
}

void clear_screen(uint16_t color)
{
	for (int y = 0; y < SCREEN_HEIGHT; y++) {
		for (int x = 0; x < SCREEN_WIDTH; x++) {
			tft_draw_pixel(x, y, color);
		}
	}
}
