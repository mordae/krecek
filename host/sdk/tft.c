#include <tft.h>

#include <stdint.h>
#include <string.h>

#if !defined(__weak)
#define __weak __attribute__((__weak__))
#endif

/*
 * We are using double buffering.
 *
 * One buffer is being written to by the client.
 * The other buffer is being transmitted.
 *
 * After every cycle the buffers are rotated.
 */
static uint8_t buffer[2][TFT_HEIGHT * TFT_WIDTH];

/* Currently inactive buffer that is to be sent to the display. */
uint8_t *tft_committed;

/* Current active buffer that is to be written into. */
uint8_t *tft_input;

/* Clipping rectangle. */
int tft_clip_x0 = 0;
int tft_clip_y0 = 0;
int tft_clip_x1 = TFT_WIDTH;
int tft_clip_y1 = TFT_HEIGHT;

/* Origin for relative drawing. */
int tft_origin_x = 0;
int tft_origin_y = 0;

/* Palette colors. */
uint16_t __attribute__((__aligned__(512))) tft_palette[256] = {
	0x0000, 0x1082, 0x2104, 0x31a6, 0x4228, 0x52aa, 0x632c, 0x73ae, 0x8c51, 0x9cd3, 0xad55,
	0xbdd7, 0xce59, 0xdefb, 0xef7d, 0xffff, 0x20a2, 0x20e2, 0x2102, 0x1902, 0x1102, 0x1102,
	0x1103, 0x1104, 0x10c4, 0x10a4, 0x1084, 0x1884, 0x2084, 0x2083, 0x2083, 0x2082, 0x2081,
	0x20c1, 0x2101, 0x1101, 0x0901, 0x0902, 0x0903, 0x0904, 0x08c4, 0x0884, 0x0844, 0x1044,
	0x1844, 0x2043, 0x2042, 0x2041, 0x2060, 0x20c0, 0x1900, 0x1100, 0x0900, 0x0101, 0x0102,
	0x0104, 0x00a4, 0x0044, 0x0004, 0x1004, 0x1804, 0x2003, 0x2002, 0x2000, 0x41a6, 0x41e6,
	0x4206, 0x3a06, 0x3206, 0x3206, 0x3207, 0x3208, 0x31c8, 0x31a8, 0x3188, 0x3988, 0x3988,
	0x4187, 0x4187, 0x4186, 0x4164, 0x41c4, 0x3a04, 0x3204, 0x2204, 0x2205, 0x2206, 0x2208,
	0x21a8, 0x2148, 0x2108, 0x3108, 0x3908, 0x4107, 0x4105, 0x4104, 0x4102, 0x41a2, 0x3a02,
	0x2a02, 0x1a02, 0x1203, 0x1205, 0x1208, 0x1168, 0x10e8, 0x1888, 0x2888, 0x3888, 0x4086,
	0x4084, 0x4082, 0x40c0, 0x4180, 0x3a00, 0x2200, 0x0a00, 0x0202, 0x0205, 0x0208, 0x0148,
	0x0088, 0x0808, 0x2008, 0x3808, 0x4006, 0x4003, 0x4000, 0x834c, 0x83ac, 0x7c0c, 0x740c,
	0x640c, 0x640d, 0x640e, 0x640f, 0x63b0, 0x6350, 0x6310, 0x7310, 0x7b10, 0x830f, 0x830d,
	0x830c, 0x82a8, 0x8368, 0x7c08, 0x6408, 0x4c08, 0x440a, 0x440c, 0x440f, 0x4350, 0x4290,
	0x4a10, 0x6210, 0x7210, 0x820e, 0x820b, 0x8208, 0x8204, 0x8324, 0x7404, 0x5404, 0x3404,
	0x2407, 0x240b, 0x240f, 0x22f0, 0x21d0, 0x2910, 0x5110, 0x7110, 0x810d, 0x8108, 0x8104,
	0x8160, 0x82e0, 0x7400, 0x4400, 0x1400, 0x0404, 0x0409, 0x040f, 0x0290, 0x0110, 0x1010,
	0x4010, 0x6810, 0x800c, 0x8006, 0x8000, 0xfeb7, 0xff77, 0xf7f7, 0xdff7, 0xc7f7, 0xbff9,
	0xbffc, 0xbfff, 0xbf3f, 0xbe7f, 0xc5ff, 0xddff, 0xf5ff, 0xfdfd, 0xfdfa, 0xfdf7, 0xfd70,
	0xfef0, 0xeff0, 0xbff0, 0x97f0, 0x87f3, 0x87f9, 0x87ff, 0x869f, 0x851f, 0x8c1f, 0xbc1f,
	0xec1f, 0xfc1b, 0xfc16, 0xfc10, 0xfc28, 0xfe48, 0xe7e8, 0xa7e8, 0x5fe8, 0x47ed, 0x47f6,
	0x47ff, 0x45df, 0x439f, 0x521f, 0x9a1f, 0xe21f, 0xfa1a, 0xfa11, 0xfa08, 0xfae0, 0xfdc0,
	0xe7e0, 0x87e0, 0x27e0, 0x07e7, 0x07f3, 0x07ff, 0x051f, 0x023f, 0x181f, 0x781f, 0xd81f,
	0xf818, 0xf80c, 0xf800,
};

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
	tft_input = buffer[0];
	tft_committed = buffer[1];
}

void tft_swap_buffers(void)
{
	uint8_t *tmp = tft_committed;
	tft_committed = tft_input;
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

void tft_draw_rect(int x0, int y0, int x1, int y1, int color)
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

void tft_fill(int color)
{
	memset(tft_input, color, TFT_WIDTH * TFT_HEIGHT);
}

void tft_draw_sprite(int x, int y, int w, int h, const uint8_t *data, int trsp)
{
	tft_draw_sprite_flipped(x, y, w, h, data, trsp, false, false, false);
}

void tft_draw_sprite_flipped(int x, int y, int w, int h, const uint8_t *data, int trsp, bool flip_x,
			     bool flip_y, bool swap_xy)
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

void tft_draw_glyph(int x, int y, int color, char c)
{
	uint8_t *glyph = tft_font + (size_t)c * 16;

	for (int gx = 0; gx < 8; gx++) {
		for (int gy = 0; gy < 16; gy++) {
			if ((glyph[gy] >> (7 - gx)) & 1) {
				tft_draw_pixel(x + gx, y + gy, color);
			}
		}
	}
}

void tft_draw_string(int x, int y, int color, const char *str)
{
	int len = strlen(str);

	for (int i = 0; i < len; i++)
		tft_draw_glyph(x + i * 8, y, color, str[i]);
}

void tft_draw_string_right(int x, int y, int color, const char *str)
{
	int len = strlen(str);

	x -= len * 8;

	for (int i = 0; i < len; i++)
		tft_draw_glyph(x + i * 8, y, color, str[i]);
}

void tft_draw_string_center(int x, int y, int color, const char *str)
{
	int len = strlen(str);

	x -= len * 4;

	for (int i = 0; i < len; i++)
		tft_draw_glyph(x + i * 8, y, color, str[i]);
}
