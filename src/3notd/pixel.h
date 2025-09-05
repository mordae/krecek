#pragma once
#include <tft.h>
#include <stdio.h>
void drawpixel(int x, int y, int c) //draw a pixel at x/y with rgb
{
	int rgb[3];

	if (c == 0) {
		rgb[0] = 255;
		rgb[1] = 255;
		rgb[2] = 0;
	} //Yellow
	if (c == 1) {
		rgb[0] = 160;
		rgb[1] = 160;
		rgb[2] = 0;
	} //Yellow darker
	if (c == 2) {
		rgb[0] = 0;
		rgb[1] = 255;
		rgb[2] = 0;
	} //Green
	if (c == 3) {
		rgb[0] = 0;
		rgb[1] = 160;
		rgb[2] = 0;
	} //Green darker
	if (c == 4) {
		rgb[0] = 0;
		rgb[1] = 255;
		rgb[2] = 255;
	} //Cyan
	if (c == 5) {
		rgb[0] = 0;
		rgb[1] = 160;
		rgb[2] = 160;
	} //Cyan darker
	if (c == 6) {
		rgb[0] = 160;
		rgb[1] = 100;
		rgb[2] = 0;
	} //brown
	if (c == 7) {
		rgb[0] = 110;
		rgb[1] = 50;
		rgb[2] = 0;
	} //brown darker
	if (c == 8) {
		rgb[0] = 0;
		rgb[1] = 60;
		rgb[2] = 130;
	} //background
	uint16_t color = rgb_to_rgb565(rgb[0], rgb[1], rgb[2]);
	tft_draw_pixel(x, y, color);
}
