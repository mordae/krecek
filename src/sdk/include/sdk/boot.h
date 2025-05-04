#pragma once
#include <sdk/image.h>

typedef struct sdk_game_info {
	char magic[8];
	char name[24];
	const sdk_image_t *cover_image;
} sdk_game_info_t;

#define sdk_game_info(name, cover)                                               \
	sdk_game_info_t __attribute__((section(".game_info"))) sdk_game_info = { \
		.magic = "KRECEK0",                                              \
		.na##me = (name),                                                \
		.cover_image = (cover),                                          \
	}

/*
 * Reboot the console into firmware in given slot.
 * We have 16 slots numbered from 0 to 15.
 * Each has 1 MiB of flash space.
 */
void sdk_reboot_into_slot(unsigned slot);
