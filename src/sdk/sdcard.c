/*
 * Written using the instructions found at:
 *
 *     <http://elm-chan.org/docs/mmc/mmc_e.html>
 */

#include <sdk/sdcard.h>
#include <sdk/fatfs.h>

#include <hardware/spi.h>

#include <pico/stdlib.h>
#include <stdio.h>
#include <task.h>

#define START 64

#if !defined(SD_TRACE)
#define SD_TRACE 0
#endif

#define TIMEOUT 256

static void finish(void)
{
	uint8_t dummy;
	gpio_put(SD_CS_PIN, 1);
	spi_read_blocking(SD_SPI_DEV, 0xff, &dummy, 1);
}

static int receive_response(int timeout)
{
	for (int i = 0; i < timeout; i++) {
		uint8_t resp;
		spi_read_blocking(SD_SPI_DEV, 0xff, &resp, 1);

		if (resp != 0xff)
			return resp;

		if (i > 8)
			task_sleep_us(i);
	}

	return -1;
}

static void wait_while_busy(int timeout)
{
	uint8_t busy;

	for (int i = 0; i < timeout; i++) {
		spi_read_blocking(SD_SPI_DEV, 0xff, &busy, 1);

		if (0x00 != busy)
			return;

		if (i > 8)
			task_sleep_us(i);
	}

	printf("sdcard: Timed out waiting for write to finish.\n");
}

static int receive_data(uint8_t *buf, size_t n)
{
	int res = receive_response(TIMEOUT);

	if (res < 0) {
		printf("sdcard: Timed out waiting for data.\n");
		return res;
	}

	if ((0xfe != res) && (0xfc != res) && (0xfd != res)) {
		printf("sdcard: Invalid receive token=%i.\n", res);
		return res;
	}

	uint8_t crc[2];
	spi_read_blocking(SD_SPI_DEV, 0xff, buf, n);
	spi_read_blocking(SD_SPI_DEV, 0xff, crc, 2);

	return 0x00;
}

static int send_data(uint8_t tok, const uint8_t *buf, size_t n)
{
	uint8_t leader[2] = { 0xff, tok };
	uint8_t crc[2] = { 0xff, 0xff };
	spi_write_blocking(SD_SPI_DEV, leader, 2);
	spi_write_blocking(SD_SPI_DEV, buf, n);
	spi_write_blocking(SD_SPI_DEV, crc, 2);

	int res = receive_response(TIMEOUT);

	if (res < 0) {
		printf("sdcard: Timed out waiting for confirmation.\n");
		return res;
	}

	if (0x05 != (res & 0x1f)) {
		printf("sdcard: Invalid send token=%i.\n", res);
		return res;
	}

	wait_while_busy(TIMEOUT);
	return 0x00;
}

static void send_command(uint8_t cmd, uint32_t arg, uint8_t crc)
{
	uint8_t cmdbuf[] = {
		START | cmd, arg >> 24, (arg << 8) >> 24, (arg << 16) >> 24, (arg << 24) >> 24, crc,
	};

	gpio_put(SD_CS_PIN, 0);
	spi_write_blocking(SD_SPI_DEV, cmdbuf, sizeof(cmdbuf));
}

static int command_r1(uint8_t cmd, uint32_t arg, uint8_t crc)
{
	send_command(cmd, arg, crc);
	int res = receive_response(10);
	finish();
	return res;
}

static int command_r7(uint8_t cmd, uint32_t arg, uint8_t crc, uint8_t echo[4])
{
	send_command(cmd, arg, crc);
	int res = receive_response(10);

	spi_read_blocking(SD_SPI_DEV, 0xff, echo, 4);
	finish();
	return res;
}

static int command_data(uint8_t cmd, uint32_t arg, uint8_t crc, uint8_t *data, size_t len)
{
	send_command(cmd, arg, crc);
	int res = receive_response(10);

	if (0x00 == res)
		res = receive_data(data, len);

	finish();
	return res;
}

void sdcard_init(void)
{
	unsigned rate = spi_init(SD_SPI_DEV, SD_BAUDRATE);
	printf("sdcard: Configured SPI: rate=%u\n", rate);

	printf("sdcard: Configure pins: cs=%i, sck=%i, mosi=%i, miso=%i\n", SD_CS_PIN, SD_SCK_PIN,
	       SD_MOSI_PIN, SD_MISO_PIN);

	gpio_set_pulls(SD_MOSI_PIN, true, false);
	gpio_set_pulls(SD_MISO_PIN, true, false);

	gpio_init(SD_CS_PIN);
	gpio_set_dir(SD_CS_PIN, GPIO_OUT);
	gpio_put(SD_CS_PIN, 1);

	gpio_set_function(SD_SCK_PIN, GPIO_FUNC_SPI);
	gpio_set_function(SD_MISO_PIN, GPIO_FUNC_SPI);
	gpio_set_function(SD_MOSI_PIN, GPIO_FUNC_SPI);
}

bool sdcard_open(void)
{
#if SD_TRACE
	puts("-- sdcard_open()");
#endif

	uint8_t dummy;
	int res = 0;

	/* Drop to 400 kHz for initialization. */
	unsigned oldrate = spi_get_baudrate(SD_SPI_DEV);
	spi_set_baudrate(SD_SPI_DEV, SD_INIT_BAUDRATE);

	/* Give at least 74 dummy cycles to init the card. */
	for (int i = 0; i < 10; i++)
		spi_read_blocking(SD_SPI_DEV, 0xff, &dummy, 1);

	/* Reset the card, switching to SPI mode. */
	res = command_r1(0, 0, 0x95);

	if (res < 0)
		goto fail;

	if (0x01 != res) {
		printf("sdcard: GO_IDLE_STATE => %i\n", res);
		goto fail;
	}

	sleep_us(10);

	/* Negotiate voltage. Required for newer cards. */
	uint8_t echo[4];
	res = command_r7(8, 0x000001aa, 0x87, echo);

	if ((res != 0) && (res != 1)) {
		printf("sdcard: SEND_IF_COND => %i\n", res);
		goto fail;
	}

	sleep_us(10);

	/* Read the OCR register. */
	res = command_r7(58, 0, 0xff, echo);

	if ((res != 0) && (res != 1)) {
		printf("sdcard: READ_OCR => %i\n", res);
		goto fail;
	}

	uint32_t ocr = (echo[0] << 24) | (echo[1] << 16) | (echo[2] << 8) | echo[0];
	const char *note = "";

	if ((ocr >> 29) & 1) {
		note = " (UHS-II)";
	} else if ((ocr >> 30) & 1) {
		note = " (UHS-I)";
	}

	printf("sdcard: OCR=0x%08x%s\n", (unsigned)ocr, note);

	sleep_us(10);

	/* Prefix for the next command. */
	res = command_r1(55, 0, 0x65);

	if ((res != 0) && (res != 1)) {
		printf("sdcard: APP_CMD => %i\n", res);
		printf("sdcard: Ancient card protocol, use a newer card.\n");
		goto fail;
	}

	sleep_us(10);

	res = command_r1(41, 0x40000000, 0x77);

	if ((res != 0) && (res != 1)) {
		printf("sdcard: ACMD41 => %i\n", res);
		printf("sdcard: Cart too old, new yet supported.\n");
		goto fail;
	}

	sleep_us(10);

	/* Configure block length. */
	res = command_r1(16, 512, 0xff);

	if ((res != 0) && (res != 1)) {
		printf("sdcard: SET_BLOCKLEN => %i\n", res);
		goto fail;
	}

	printf("sdcard: initialized\n");

	spi_set_baudrate(SD_SPI_DEV, oldrate);
	return true;

fail:
	spi_set_baudrate(SD_SPI_DEV, oldrate);
	return false;
}

enum sdcard_status sdcard_check(void)
{
#if SD_TRACE
	printf("-- sdcard_check()\n");
#endif

	/* Abuse blocksize adjustment to detect card status */
	int res = command_r1(16, 512, 0xff);

	if (0x00 == res)
		return SDCARD_READY;

	if (0x01 == res)
		return SDCARD_IDLE;

	return SDCARD_MISSING;
}

bool sdcard_read_block(uint32_t addr, uint8_t *buf)
{
#if SD_TRACE
	printf("-- sdcard_read_block(%u)\n", (unsigned)addr);
#endif

	int res = command_data(17, addr, 0xff, buf, 512);

	if (0x00 != res) {
		printf("sdcard: READ_SINGLE_BLOCK => %i\n", res);
		return false;
	}

	return true;
}

bool sdcard_write_block(uint32_t addr, const uint8_t *buf)
{
#if SD_TRACE
	printf("-- sdcard_write_block(%u)\n", (unsigned)addr);
#endif

	send_command(24, addr, 0xff);
	int res = receive_response(10);

	if (0x00 == res) {
		res = send_data(0xfe, buf, 512);
	} else {
		printf("sdcard: WRITE_BLOCK => %i\n", res);
	}

	finish();
	return 0x00 == res;
}

bool sdcard_begin_read(uint32_t addr)
{
#if SD_TRACE
	printf("-- sdcard_begin_read(%u)\n", (unsigned)addr);
#endif

	send_command(18, addr, 0xff);
	int res = receive_response(10);

	if (0x00 == res)
		return true;

	finish();
	printf("sdcard: READ_MULTIPLE_BLOCK => %i\n", res);
	return false;
}

bool sdcard_begin_write(uint32_t addr)
{
#if SD_TRACE
	printf("-- sdcard_begin_write(%u)\n", (unsigned)addr);
#endif

	send_command(25, addr, 0xff);
	uint8_t resp = receive_response(10);

	if (0x00 == resp)
		return true;

	finish();
	printf("sdcard: WRITE_MULTIPLE_BLOCK => %i\n", resp);
	return false;
}

bool sdcard_read_next(uint8_t *buf)
{
#if SD_TRACE
	printf("-- sdcard_read_next()\n");
#endif

	uint8_t resp = receive_data(buf, 512);
	return 0x00 == resp;
}

bool sdcard_write_next(const uint8_t *buf)
{
#if SD_TRACE
	printf("-- sdcard_write_next()\n");
#endif

	uint8_t resp = send_data(0xfc, buf, 512);
	return 0x00 == resp;
}

void sdcard_end_read(void)
{
#if SD_TRACE
	printf("-- sdcard_end_read()\n");
#endif

	/* Ask to terminate the transmission. */
	send_command(12, 0, 0xff);

	/* Keep reading until the termination is confirmed. */
	uint8_t resp = 0x00;

	for (int i = 0; i < TIMEOUT; i++) {
		spi_read_blocking(SD_SPI_DEV, 0xff, &resp, 1);

		if (0xff == resp)
			break;
	}

	finish();
}

void sdcard_end_write(void)
{
#if SD_TRACE
	printf("-- sdcard_end_write()\n");
#endif

	/* Terminate the transmission. */
	uint8_t stop[2] = { 0xfd, 0xff };
	spi_write_blocking(SD_SPI_DEV, stop, 2);

	wait_while_busy(TIMEOUT);
	finish();
}

inline static bool read_bit(uint8_t *buf, size_t n)
{
	uint8_t byte = buf[n >> 3];
	return (byte >> (7 - (n & 7))) & 1;
}

static uint32_t read_bits(uint8_t *buf, size_t *used, size_t n)
{
	assert(n <= 32);

	uint32_t accum = 0;

	for (size_t i = 0; i < n; i++) {
		accum <<= 1;
		accum |= read_bit(buf, (*used)++);
	}

	return accum;
}

bool sdcard_read_cid(struct sdcard_cid *cid)
{
	uint8_t resp = command_data(10, 0, 0xff, cid->data, 16);

	if (0x00 != resp) {
		printf("sdcard: SEND_CID => %i\n", resp);
		return false;
	}

	size_t used = 0;

	cid->mid = read_bits(cid->data, &used, 8);
	cid->name[0] = read_bits(cid->data, &used, 8);
	cid->name[1] = read_bits(cid->data, &used, 8);
	cid->name[2] = read_bits(cid->data, &used, 8);
	cid->name[3] = read_bits(cid->data, &used, 8);
	cid->name[4] = read_bits(cid->data, &used, 8);
	cid->name[5] = read_bits(cid->data, &used, 8);
	cid->name[6] = read_bits(cid->data, &used, 8);
	cid->name[7] = '\0';
	cid->prv = read_bits(cid->data, &used, 8);
	cid->psn = read_bits(cid->data, &used, 32);
	used += 4;
	cid->mdt_year = read_bits(cid->data, &used, 8);
	cid->mdt_month = read_bits(cid->data, &used, 4);
	used += 8;

	if (128 != used)
		panic("sdcard_read_cid: read %zu/128 bits", used);

	return true;
}

void sdcard_print_cid(struct sdcard_cid *cid)
{
	printf("  cid.mid                = %i\n", cid->mid);
	printf("  cid.name               = \"%s\"\n", cid->name);
	printf("  cid.prv                = %i\n", cid->prv);
	printf("  cid.psn                = 0x%08x\n", (unsigned)cid->psn);
	printf("  cid.mdt_year           = %i (%u)\n", cid->mdt_year, 2000 + cid->mdt_year);
	printf("  cid.mdt_month          = 0x%01hhx (%u)\n", cid->mdt_month, cid->mdt_month);
}

bool sdcard_read_csd(struct sdcard_csd *csd)
{
	uint8_t resp = command_data(9, 0, 0xff, csd->data, 16);

	if (0x00 != resp) {
		printf("sdcard: SEND_CSD => %i\n", resp);
		return false;
	}

	size_t used = 0;

	csd->version = read_bits(csd->data, &used, 2);
	used += 6;
	csd->taac = read_bits(csd->data, &used, 8);
	csd->nsac = read_bits(csd->data, &used, 8);
	csd->tran_speed = read_bits(csd->data, &used, 8);
	csd->ccc = read_bits(csd->data, &used, 12);
	csd->read_bl_len = read_bits(csd->data, &used, 4);
	csd->read_bl_partial = read_bits(csd->data, &used, 1);
	csd->write_blk_misalign = read_bits(csd->data, &used, 1);
	csd->read_blk_misalign = read_bits(csd->data, &used, 1);
	csd->dsr_imp = read_bits(csd->data, &used, 1);

	if (0 == csd->version) {
		used += 2;
		csd->c_size = read_bits(csd->data, &used, 12);
		csd->vdd_r_curr_min = read_bits(csd->data, &used, 3);
		csd->vdd_r_curr_max = read_bits(csd->data, &used, 3);
		csd->vdd_w_curr_min = read_bits(csd->data, &used, 3);
		csd->vdd_w_curr_max = read_bits(csd->data, &used, 3);
		csd->c_size_mult = read_bits(csd->data, &used, 3);

		uint32_t mult = 1ul << (csd->c_size_mult + 2);
		uint32_t blocknr = (csd->c_size + 1) * mult;
		uint32_t block_len = 1ul << csd->read_bl_len;
		csd->size = blocknr * block_len;
	} else if (1 == csd->version) {
		used += 6;
		csd->c_size = read_bits(csd->data, &used, 22);
		used += 1;

		csd->size = 512llu * 1024llu * (csd->c_size + 1llu);
	} else if (2 == csd->version) {
		csd->c_size = read_bits(csd->data, &used, 28);
		used += 1;

		csd->size = 512llu * 1024llu * (csd->c_size + 1llu);
	}

	csd->erase_blk_en = read_bits(csd->data, &used, 1);
	csd->sector_size = read_bits(csd->data, &used, 7);
	csd->wp_grp_size = read_bits(csd->data, &used, 7);
	csd->wp_grp_enable = read_bits(csd->data, &used, 1);
	used += 2;
	csd->r2w_factor = read_bits(csd->data, &used, 3);
	csd->write_bl_len = read_bits(csd->data, &used, 4);
	csd->write_bl_partial = read_bits(csd->data, &used, 1);
	used += 5;
	csd->file_format_grp = read_bits(csd->data, &used, 1);
	csd->copy = read_bits(csd->data, &used, 1);
	csd->perm_write_protect = read_bits(csd->data, &used, 1);
	csd->tmp_write_protect = read_bits(csd->data, &used, 1);
	csd->file_format = read_bits(csd->data, &used, 2);
	csd->wp_upc = read_bits(csd->data, &used, 1);
	used += 9;

	if (128 != used)
		panic("sdcard_read_csd: read %zu/128 bits");

	uint32_t speed_base[8] = { 100, 1000, 10000, 100000, 0, 0, 0, 0 };
	uint32_t speed_mult[16] = {
		0,    1000, 1200, 1300, 1500, 2000, 2500, 3000,
		3500, 4000, 4500, 5000, 5500, 6000, 7000, 8000,
	};

	csd->speed = speed_base[csd->tran_speed & 7];
	csd->speed *= speed_mult[csd->tran_speed >> 3];

	return true;
}

void sdcard_print_csd(struct sdcard_csd *csd)
{
	static const char *min_current[8] = {
		"0.5", "1", "5", "10", "25", "35", "60", "100",
	};
	static const char *max_current[8] = {
		"1", "5", "10", "25", "35", "45", "80", "200",
	};
	static const char *format[4] = { "MBR", "FAT", "UFF", "Other" };

	printf("  csd.version            = %hu\n", csd->version);
	printf("  csd.taac               = %i\n", csd->taac);
	printf("  csd.nsac               = %i\n", csd->nsac);
	printf("  csd.tran_speed         = %i (%u)\n", csd->tran_speed, (unsigned)csd->speed);
	printf("  csd.ccc                = 0x%03x\n", (unsigned)csd->ccc);
	printf("  csd.read_bl_len        = %i (%u)\n", csd->read_bl_len, 1 << csd->read_bl_len);
	printf("  csd.read_bl_partial    = %hhu\n", csd->read_bl_partial);
	printf("  csd.write_blk_misalign = %hhu\n", csd->write_blk_misalign);
	printf("  csd.read_blk_misalign  = %hhu\n", csd->read_blk_misalign);
	printf("  csd.dsr_imp            = %hhu\n", csd->dsr_imp);
	printf("  csd.c_size             = 0x%x (%llu total)\n", (unsigned)csd->c_size, csd->size);
	if (0 == csd->version) {
		printf("  csd.vdd_r_curr_min     = 0x%hx (%s mA)\n", csd->vdd_r_curr_min,
		       min_current[csd->vdd_r_curr_min]);
		printf("  csd.vdd_r_curr_max     = 0x%hx (%s mA)\n", csd->vdd_r_curr_max,
		       max_current[csd->vdd_r_curr_max]);
		printf("  csd.vdd_w_curr_min     = 0x%hx (%s mA)\n", csd->vdd_w_curr_min,
		       min_current[csd->vdd_w_curr_min]);
		printf("  csd.vdd_w_curr_max     = 0x%hx (%s mA)\n", csd->vdd_w_curr_max,
		       max_current[csd->vdd_w_curr_max]);
		printf("  csd.c_size_mult        = 0x%x\n", (unsigned)csd->c_size_mult);
	}
	printf("  csd.erase_blk_en       = %hhu\n", csd->erase_blk_en);
	printf("  csd.sector_size        = %i (%u)\n", csd->sector_size, csd->sector_size + 1);
	printf("  csd.wp_grp_size        = %i (%u)\n", csd->wp_grp_size, csd->wp_grp_size + 1);
	printf("  csd.wp_grp_enable      = %hhu\n", csd->wp_grp_enable);
	printf("  csd.r2w_factor         = 0x%hhx (%u)\n", csd->r2w_factor, 1 << csd->r2w_factor);
	printf("  csd.write_bl_len       = 0x%01hhx (%u)\n", csd->write_bl_len,
	       1 << csd->write_bl_len);
	printf("  csd.write_bl_partial   = %hhu\n", csd->write_bl_partial);
	printf("  csd.file_format_grp    = %hhu\n", csd->file_format_grp);
	printf("  csd.copy               = %hhu\n", csd->copy);
	printf("  csd.perm_write_protect = %hhu\n", csd->perm_write_protect);
	printf("  csd.tmp_write_protect  = %hhu\n", csd->tmp_write_protect);
	printf("  csd.file_format        = 0x%01hhx (%s)\n", csd->file_format,
	       format[csd->file_format & 3]);
	printf("  csd.wp_upc             = %hhu\n", csd->wp_upc);
}
