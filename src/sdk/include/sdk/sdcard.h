#pragma once
#include <pico/stdlib.h>

#include <stdint.h>

#if !defined(SD_INIT_BAUDRATE)
#define SD_INIT_BAUDRATE 400000
#endif

enum sdcard_status {
	SDCARD_READY = 0,
	SDCARD_IDLE = 1,
	SDCARD_MISSING = 2,
};

void sdcard_init(void);
bool sdcard_open(void);
enum sdcard_status sdcard_check(void);

/* Read/write a single 512 byte long block at given address. */
bool sdcard_read_block(uint32_t addr, uint8_t *buf);
bool sdcard_write_block(uint32_t addr, const uint8_t *buf);

/* Begin reading/writing multiple 512 byte long blocks from given address. */
bool sdcard_begin_read(uint32_t addr);
bool sdcard_begin_write(uint32_t addr);

/* Read/write next 512 byte long block. */
bool sdcard_read_next(uint8_t *buf);
bool sdcard_write_next(const uint8_t *buf);

/* Terminate multiple block read/write operation. */
void sdcard_end_read(void);
void sdcard_end_write(void);

struct sdcard_cid {
	uint8_t data[16];

	uint8_t mid;
	char name[8];
	uint8_t prv;
	uint32_t psn;
	uint8_t mdt_year;
	uint8_t mdt_month;
};

struct sdcard_csd {
	uint8_t data[16];

	uint8_t version;
	uint8_t taac;
	uint8_t nsac;
	uint8_t tran_speed;

	uint32_t ccc;
	uint32_t c_size;
	uint32_t c_size_mult;

	bool read_bl_partial : 1;
	bool write_blk_misalign : 1;
	bool read_blk_misalign : 1;
	bool dsr_imp : 1;
	bool erase_blk_en : 1;
	bool wp_grp_enable : 1;
	bool write_bl_partial : 1;
	bool file_format_grp : 1;
	bool copy : 1;
	bool perm_write_protect : 1;
	bool tmp_write_protect : 1;
	bool wp_upc : 1;

	uint8_t vdd_r_curr_min;
	uint8_t vdd_r_curr_max;
	uint8_t vdd_w_curr_min;
	uint8_t vdd_w_curr_max;
	uint8_t sector_size;
	uint8_t wp_grp_size;
	uint8_t r2w_factor;
	uint8_t read_bl_len;
	uint8_t write_bl_len;
	uint8_t file_format;

	/* Following are calculated for your convenience. */

	/* Size of the card in bytes. */
	uint64_t size;

	/* Maximum rated access speed in Hz. */
	uint32_t speed;
};

bool sdcard_read_cid(struct sdcard_cid *cid);
void sdcard_print_cid(struct sdcard_cid *cid);

bool sdcard_read_csd(struct sdcard_csd *csd);
void sdcard_print_csd(struct sdcard_csd *csd);
