#include <pico/flash.h>
#include <sdk/boot.h>
#include <sdk/save.h>
#include <hardware/flash.h>
#include <string.h>

struct write_command {
	uint32_t page;
	const void *data;
	int len;
};

static void __not_in_flash_func(do_write)(void *arg)
{
	static uint8_t flash_write_buffer[256];

	struct write_command *cmd = arg;
	flash_range_erase(cmd->page, SDK_SAVE_SLOT_SIZE);

	for (int i = 0; i < cmd->len; i += 256) {
		int len = cmd->len - i;

		if (len >= 256) {
			flash_range_program(cmd->page + i, cmd->data + i, 256);
		} else {
			memset(flash_write_buffer, 0xff, sizeof(flash_write_buffer));
			memcpy(flash_write_buffer, cmd->data + i, len);
			flash_range_program(cmd->page + i, flash_write_buffer, 256);
		}
	}
}

void sdk_save_write(int slot, const void *data, int len)
{
	hard_assert(slot >= 0 && slot < SDK_NUM_SAVE_SLOTS);

	if (len > SDK_SAVE_SLOT_SIZE)
		len = SDK_SAVE_SLOT_SIZE;

	uint32_t base = 0xf80000 + current_slot * SDK_SAVE_SLOT_SIZE * SDK_NUM_SAVE_SLOTS;
	uint32_t page = base + SDK_SAVE_SLOT_SIZE * slot;

	struct write_command cmd = {
		.page = page,
		.data = data,
		.len = len,
	};

	int res = flash_safe_execute(do_write, &cmd, UINT32_MAX);
	hard_assert(PICO_OK == res);

	uint32_t *xip = (uint32_t *)(XIP_BASE + page);

	for (int i = 0; i < (SDK_SAVE_SLOT_SIZE >> 2); i++)
		xip[i] = 0;
}

void sdk_save_read(int slot, void *data, int len)
{
	hard_assert(slot >= 0 && slot < SDK_NUM_SAVE_SLOTS);

	if (len > SDK_SAVE_SLOT_SIZE)
		len = SDK_SAVE_SLOT_SIZE;

	uint32_t base = 0xf80000 + current_slot * SDK_SAVE_SLOT_SIZE * SDK_NUM_SAVE_SLOTS;
	uint32_t page = base + SDK_SAVE_SLOT_SIZE * slot;

	const void *xip = (const void *)(XIP_BASE + page);
	memcpy(data, xip, len);
}
