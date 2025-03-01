#include <tft.h>

#include <stdint.h>
#include <string.h>

#if !defined(__weak)
#define __weak __attribute__((__weak__))
#endif

/* Buffers. */
color_t tft_buffers[2][TFT_RAW_HEIGHT][TFT_RAW_WIDTH];
color_t (*tft_input)[TFT_RAW_WIDTH];
color_t (*tft_active)[TFT_RAW_WIDTH];

/* Clipping rectangle. */
int tft_clip_x0 = 0;
int tft_clip_y0 = 0;
int tft_clip_x1 = TFT_WIDTH;
int tft_clip_y1 = TFT_HEIGHT;

/* Origin for relative drawing. */
int tft_origin_x = 0;
int tft_origin_y = 0;

__weak void tft_dma_channel_wait_for_finish_blocking(int dma_ch_spi)
{
	(void)dma_ch_spi;
}

void tft_control(uint8_t reg, const uint8_t *bstr, int len)
{
	(void)reg;
	(void)bstr;
	(void)len;
}

void tft_init(void)
{
	tft_input = tft_buffers[0];
	tft_active = tft_buffers[1];
}

void tft_swap_buffers(void)
{
	void *tmp = tft_input;
	tft_active = tft_input;
	tft_input = tmp;
}

void tft_sync(void)
{
}

void tft_swap_sync(void)
{
	tft_swap_buffers();
	tft_sync();
}

void tft_draw_rect(int x0, int y0, int x1, int y1, color_t color)
{
	if (x0 > x1) {
		int tmp = x0;
		x0 = x1;
		x1 = tmp;
	}

	if (y0 > y1) {
		int tmp = y0;
		y0 = y1;
		y1 = tmp;
	}

#if TFT_SWAP_XY
	for (int x = x0; x <= x1; x++)
		for (int y = y0; y <= y1; y++)
			tft_draw_pixel(x, y, color);
#else
	for (int y = y0; y <= y1; y++)
		for (int x = x0; x <= x1; x++)
			tft_draw_pixel(x, y, color);
#endif
}

void tft_fill(color_t color)
{
	for (int y = 0; y < TFT_RAW_HEIGHT; y++)
		for (int x = 0; x < TFT_RAW_WIDTH; x++)
			tft_input[y][x] = color;
}

void tft_draw_sprite(int x, int y, int w, int h, const color_t *data, uint16_t trsp)
{
	tft_draw_sprite_flipped(x, y, w, h, data, trsp, false, false, false);
}

void tft_draw_sprite_flipped(int x, int y, int w, int h, const color_t *data, uint16_t trsp,
			     bool flip_x, bool flip_y, bool swap_xy)
{
#define loop                                   \
	for (int sy = 0; sy < h; sy++)         \
		for (int sx = 0; sx < w; sx++) \
			if ((c = data[sy * w + sx]) != trsp)

	int c;
	int b = h - 1;
	int r = w - 1;

	if (flip_y && flip_x && swap_xy) {
		loop tft_draw_pixel(x + b - sy, y + r - sx, c);
	} else if (flip_y && flip_x && !swap_xy) {
		loop tft_draw_pixel(x + r - sx, y + b - sy, c);
	} else if (flip_y && !flip_x && swap_xy) {
		loop tft_draw_pixel(x + b - sy, y + sx, c);
	} else if (flip_y && !flip_x && !swap_xy) {
		loop tft_draw_pixel(x + sx, y + b - sy, c);
	} else if (!flip_y && flip_x && swap_xy) {
		loop tft_draw_pixel(x + sy, y + r - sx, c);
	} else if (!flip_y && flip_x && !swap_xy) {
		loop tft_draw_pixel(x + r - sx, y + sy, c);
	} else if (!flip_y && !flip_x && swap_xy) {
		loop tft_draw_pixel(x + sy, y + sx, c);
	} else {
		loop tft_draw_pixel(x + sx, y + sy, c);
	}

#undef loop
}

void tft_draw_glyph(int x, int y, color_t color, char c)
{
	const uint8_t *glyph = tft_font + (size_t)c * 16;

	for (int gx = 0; gx < 8; gx++) {
		for (int gy = 0; gy < 16; gy++) {
			if ((glyph[gy] >> (7 - gx)) & 1) {
				tft_draw_pixel(x + gx, y + gy, color);
			}
		}
	}
}

void tft_draw_string(int x, int y, color_t color, const char *str)
{
	int len = strlen(str);

	for (int i = 0; i < len; i++)
		tft_draw_glyph(x + i * 8, y, color, str[i]);
}

void tft_draw_string_right(int x, int y, color_t color, const char *str)
{
	int len = strlen(str);

	x -= len * 8;

	for (int i = 0; i < len; i++)
		tft_draw_glyph(x + i * 8, y, color, str[i]);
}

void tft_draw_string_center(int x, int y, color_t color, const char *str)
{
	int len = strlen(str);

	x -= len * 4;

	for (int i = 0; i < len; i++)
		tft_draw_glyph(x + i * 8, y, color, str[i]);
}
