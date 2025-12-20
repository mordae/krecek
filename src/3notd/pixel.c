#include <tft.h>
void drawpixel(int x, int y, int r, int g, int b) //draw a pixel at x/y with rgb
{
	uint16_t color = rgb_to_rgb565(r, g, b);
	tft_draw_pixel(x, y, color);
}
