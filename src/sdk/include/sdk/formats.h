#pragma once
#include <stdint.h>
#include <sdk/fatfs.h>
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

inline static int sdk_bmp_write_header(FIL *file, uint32_t width, uint32_t height)
{
	sdk_bmp_t bmp = {
		.signature = 0x4d42,
		.file_size = sizeof(bmp) + 2 * width * height,
		.data_offset = sizeof(bmp),
		.size = 40,
		.width = width,
		.height = height,
		.planes = 1,
		.bits_per_pixel = 16,
		.x_pixels_per_m = 2667,
		.y_pixels_per_m = 2667,
	};

	unsigned bw = 0;
	if (FR_OK != f_write(file, &bmp, sizeof(bmp), &bw))
		return -1;

	return 0;
}

inline static int sdk_bmp_write_frame(FIL *file, color_t (*frame)[TFT_HEIGHT])
{
	static_assert(TFT_WIDTH % 2 == 0);

	for (int y = 0; y < TFT_HEIGHT; y++) {
		color_t stride[TFT_WIDTH];

		for (int x = 0; x < TFT_WIDTH; x++)
			stride[x] = frame[x][TFT_BOTTOM - y];

		unsigned bw;
		if (FR_OK != f_write(file, stride, sizeof(stride), &bw))
			return -1;
	}

	return 0;
}
