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
	asm(#name ":\n");                       \
	asm(".int _" #name "_start\n");         \
	asm(".int " #name " - _" #name "_start\n")

#define embed_sprite(name, w, h, t, path)         \
	extern const struct sdk_sprite name;      \
	asm(".align 4\n");                        \
	asm(".section \".flashdata.sprites\"\n"); \
	asm("_" #name "_start:\n");               \
	asm(".incbin \"" path "\"\n");            \
	asm(#name ":\n");                         \
	asm(".int _" #name "_start\n");           \
	asm(".int " SDK_TO_STRING((w)) "\n");     \
	asm(".int " SDK_TO_STRING((h)) "\n");     \
	asm(".int " SDK_TO_STRING((t)) "\n")

inline static void __unused sdk_draw_sprite(int x, int y, sdk_sprite_t sprite)
{
	/* Make sure not to read the data through cache. */
	const uint8_t *data = sprite->data - XIP_BASE + XIP_NOALLOC_BASE;
	tft_draw_sprite(x, y, sprite->w, sprite->h, data, sprite->transparency);
}
