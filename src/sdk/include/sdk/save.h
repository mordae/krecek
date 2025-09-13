#pragma once

#define SDK_NUM_SAVE_SLOTS 4
#define SDK_SAVE_SLOT_SIZE 4096

/*
 * Erase given save game slot and write new data of given length.
 * Length can be at most SDK_SAVE_SLOT_SIZE bytes.
 */
void sdk_save_write(int slot, const void *data, int len);

/*
 * Read given save game slot of at most length bytes or
 * SDK_SAVE_SLOT_SIZE, whichever is smaller.
 */
void sdk_save_read(int slot, void *data, int len);
