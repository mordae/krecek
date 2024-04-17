#pragma once
#include <pico/stdlib.h>

#include <tft.h>

struct sdk_file {
	const void *data;
	size_t len;
};

struct sdk_sprite {
	const uint8_t *data;
	int w;
	int h;
	int transparency;
};

typedef const struct sdk_file *sdk_file_t;
typedef const struct sdk_sprite *sdk_sprite_t;

asm(".section \".flashdata.files\", \"a\"");
asm(".section \".flashdata.sprites\", \"a\"");

#define SDK_TO_STRING_(x) #x
#define SDK_TO_STRING(x) SDK_TO_STRING_((x))

#define embed_file(name, path)                  \
	extern const struct sdk_file name;      \
	asm(".align 4\n");                      \
	asm(".section \".flashdata.files\"\n"); \
	asm("_" #name "_start:\n");             \
	asm(".incbin \"" path "\"\n");          \
	asm("_" #name "_end:\n");               \
	asm(".align 4\n");                      \
	asm(#name ":\n");                       \
	asm(".int _" #name "_start\n");         \
	asm(".int _" #name "_end - _" #name "_start\n")

#define embed_sprite(name, w, h, t, path)         \
	extern const struct sdk_sprite name;      \
	asm(".align 4\n");                        \
	asm(".section \".flashdata.sprites\"\n"); \
	asm("_" #name "_start:\n");               \
	asm(".incbin \"" path "\"\n");            \
	asm(".align 4\n");                        \
	asm(#name ":\n");                         \
	asm(".int _" #name "_start\n");           \
	asm(".int " SDK_TO_STRING((w)) "\n");     \
	asm(".int " SDK_TO_STRING((h)) "\n");     \
	asm(".int " SDK_TO_STRING((t)) "\n")

inline static void sdk_draw_sprite_flipped(int x, int y, sdk_sprite_t sprite, bool flip_x,
					   bool flip_y, bool swap_xy)
{
	/* Make sure not to read the data through cache. */
	const uint8_t *data = sprite->data - XIP_BASE + XIP_NOALLOC_BASE;
	tft_draw_sprite_flipped(x, y, sprite->w, sprite->h, data, sprite->transparency, flip_x,
				flip_y, swap_xy);
}

inline static void __unused sdk_draw_sprite(int x, int y, sdk_sprite_t sprite)
{
	sdk_draw_sprite_flipped(x, y, sprite, false, false, false);
}

/* Draw rotated sprite, where angle is times 90° clockwise. */
inline static void __unused sdk_draw_sprite_rotated(int x, int y, sdk_sprite_t sprite, int angle)
{
	bool a = angle & 1;
	bool b = (angle >> 1) & 1;
	sdk_draw_sprite_flipped(x, y, sprite, b, b ^ a, a);
}
