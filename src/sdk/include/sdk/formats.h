#pragma once
#include <stdint.h>
#include <string.h>

typedef struct sdk_xwd {
	uint32_t header_size;

	uint32_t file_version;
	uint32_t pixmap_format;
	uint32_t pixmap_depth;
	uint32_t pixmap_width;
	uint32_t pixmap_height;
	uint32_t xoffset;
	uint32_t byte_order;

	uint32_t bitmap_unit;
	uint32_t bitmap_bit_order;
	uint32_t bitmap_pad;

	uint32_t bits_per_pixel;

	uint32_t bytes_per_line;
	uint32_t visual_class;
	uint32_t red_mask;
	uint32_t green_mask;
	uint32_t blue_mask;
	uint32_t bits_per_rgb;
	uint32_t colormap_entries;
	uint32_t ncolors;
	uint32_t window_width;
	uint32_t window_height;
	uint32_t window_x;
	uint32_t window_y;
	uint32_t window_bdrwidth;

	char window_name[16];
} sdk_xwd_t;

inline static int sdk_xwd_init(sdk_xwd_t *xwd, const char *name, int width, int height)
{
	int len = strlen(name) + 1;

	if (len > 16)
		len = 16;

	xwd->header_size = __builtin_bswap32(sizeof(sdk_xwd_t) + len);

	xwd->file_version = __builtin_bswap32(7);
	xwd->pixmap_format = __builtin_bswap32(2);
	xwd->pixmap_depth = __builtin_bswap32(16);
	xwd->pixmap_width = __builtin_bswap32(width);
	xwd->pixmap_height = __builtin_bswap32(height);
	xwd->xoffset = 0;
	xwd->byte_order = 0;

	xwd->bitmap_unit = __builtin_bswap32(16);
	xwd->bitmap_bit_order = 0;
	xwd->bitmap_pad = __builtin_bswap32(16);

	xwd->bits_per_pixel = __builtin_bswap32(16);

	xwd->bytes_per_line = __builtin_bswap32(2 * width);
	xwd->visual_class = __builtin_bswap32(4);

	xwd->red_mask = __builtin_bswap32(0b1111100000000000);
	xwd->green_mask = __builtin_bswap32(0b0000011111100000);
	xwd->blue_mask = __builtin_bswap32(0b0000000000011111);
	xwd->bits_per_rgb = __builtin_bswap32(16);

	xwd->colormap_entries = 0;
	xwd->ncolors = 0;

	xwd->window_width = __builtin_bswap32(width);
	xwd->window_height = __builtin_bswap32(height);
	xwd->window_x = 0;
	xwd->window_y = 0;
	xwd->window_bdrwidth = 0;

	strlcpy(xwd->window_name, name, 16);

	return sizeof(sdk_xwd_t) + len;
}

typedef struct __attribute__((__packed__)) sdk_bmp {
	// Header
	uint16_t signature;
	uint32_t file_size;
	uint32_t reserved;
	uint32_t data_offset;

	// InfoHeader
	uint32_t size;
	uint32_t width;
	uint32_t height;
	uint16_t planes;
	uint16_t bits_per_pixel;
	uint32_t compression;
	uint32_t image_size;
	uint32_t x_pixels_per_m;
	uint32_t y_pixels_per_m;
	uint32_t colors_used;
	uint32_t important_colors;
} sdk_bmp_t;

inline static void sdk_bmp_init(sdk_bmp_t *bmp, uint32_t width, uint32_t height)
{
	bmp->signature = 0x4d42;
	bmp->file_size = sizeof(*bmp) + 2 * width * height;
	bmp->reserved = 0;
	bmp->data_offset = sizeof(*bmp);

	bmp->size = 40;
	bmp->width = width;
	bmp->height = height;
	bmp->planes = 1;
	bmp->bits_per_pixel = 16;
	bmp->compression = 0;
	bmp->image_size = 0;
	bmp->x_pixels_per_m = 2667;
	bmp->y_pixels_per_m = 2667;
	bmp->colors_used = 0;
	bmp->important_colors = 0;
}
