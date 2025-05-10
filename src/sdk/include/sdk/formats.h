#pragma once
#include <stdint.h>
#include <sdk/fatfs.h>
#include <sdk/image.h>
#include <tft.h>

#if !defined(__packed)
#define __packed __attribute__((__packed__))
#endif

typedef struct __packed sdk_bmp {
	// Header
	uint16_t signature;
	uint32_t file_size;
	uint32_t reserved;
	uint32_t data_offset;

	// InfoHeader
	uint32_t size;
	uint32_t width;
	int32_t height;
	uint16_t planes;
	uint16_t bits_per_pixel;
	uint32_t compression;
	uint32_t image_size;
	uint32_t x_pixels_per_m;
	uint32_t y_pixels_per_m;
	uint32_t colors_used;
	uint32_t important_colors;

	// BitFields
	uint32_t red_mask;
	uint32_t green_mask;
	uint32_t blue_mask;
	uint32_t alpha_mask;
} sdk_bmp_t;

/* Write BMP header. */
inline static int sdk_bmp_write_header(FIL *file, uint32_t width, uint32_t height)
{
	sdk_bmp_t bmp = {
		.signature = 0x4d42,
		.file_size = sizeof(bmp) + 2 * width * height,
		.data_offset = sizeof(bmp),
		.size = sizeof(bmp) - 14,
		.width = width,
		.height = -height,
		.planes = 1,
		.bits_per_pixel = 16,
		.compression = 3,
		.x_pixels_per_m = 2667,
		.y_pixels_per_m = 2667,
		.red_mask = 0xf800,
		.green_mask = 0x07e0,
		.blue_mask = 0x001f,
	};

	int err;
	unsigned bw = 0;
	if ((err = f_write(file, &bmp, sizeof(bmp), &bw)))
		return -err;

	return 0;
}

inline static color_t rgb565_to_argb1555(color_t rgb)
{
	color_t red = rgb >> 11;
	color_t green = (rgb << 5) >> 11;
	color_t blue = (rgb << 11) >> 11;
	return ((rgb == TRANSPARENT) << 15) | (red << 10) | (green << 5) | blue;
}

/* Write TFT frame buffer into BMP file. */
inline static int sdk_bmp_write_frame(FIL *file, const color_t (*frame)[TFT_HEIGHT])
{
	static_assert(TFT_WIDTH % 2 == 0);

	int err = sdk_bmp_write_header(file, TFT_WIDTH, TFT_HEIGHT);

	if (0 > err)
		return err;

	for (int y = 0; y < TFT_HEIGHT; y++) {
		color_t stride[TFT_WIDTH];

		for (int x = 0; x < TFT_WIDTH; x++)
			stride[x] = frame[x][y];

		unsigned bw;
		if ((err = f_write(file, stride, sizeof(stride), &bw)))
			return -err;
	}

	return 0;
}

/* Read TFT frame buffer from BMP file. */
inline static int sdk_bmp_read_frame(FIL *file, color_t (*frame)[TFT_HEIGHT])
{
	sdk_bmp_t bmp;

	int err;
	unsigned br;
	if ((err = f_read(file, &bmp, sizeof(bmp), &br)))
		return -err;

	if (bmp.signature != 0x4d42 || bmp.width != TFT_WIDTH || bmp.height != -TFT_HEIGHT ||
	    bmp.bits_per_pixel != 16 || bmp.colors_used != 0 || bmp.size != sizeof(bmp) - 14 ||
	    bmp.red_mask != 0xf800 || bmp.green_mask != 0x07e0 || bmp.blue_mask != 0x001f ||
	    bmp.alpha_mask != 0 || bmp.compression != 3)
		return -FR_INVALID_FORMAT;

	color_t stride[TFT_WIDTH];

	for (int y = 0; y < TFT_HEIGHT; y++) {
		if ((err = f_read(file, stride, sizeof(stride), &br)))
			return -err;

		for (int x = 0; x < TFT_WIDTH; x++)
			frame[x][y] = stride[x];
	}

	return 0;
}
