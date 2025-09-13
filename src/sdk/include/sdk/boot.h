#pragma once
#include <sdk/image.h>

typedef struct sdk_game_info {
	char magic[8];
	char name[24];
	const sdk_image_t *cover_image;
} sdk_game_info_t;

#define sdk_game_info(name, cover)                                               \
	sdk_game_info_t __attribute__((section(".game_info"))) sdk_game_info = { \
		.magic = "KRECEK1",                                              \
		.na##me = (name),                                                \
		.cover_image = (cover),                                          \
	}

/*
 * Slot we are running from.
 */
extern int current_slot;

/*
 * Reboot the console into firmware in given slot.
 * We have 32 slots numbered from 0 to 31.
 * Each has 512 KiB of flash space.
 */
void sdk_reboot_into_slot(unsigned slot);
