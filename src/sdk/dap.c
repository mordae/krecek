/*
 * Copyright (C) Jan Hamal Dvořák <mordae@anilinux.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <pico/stdlib.h>

#include <stdio.h>

#include "dap.h"

/*
 * Idle cycles make debugging with logic analyzer easier,
 * but they slow the communication down.
 */
#if !defined(DAP_INSERT_IDLE_CYCLES)
#define DAP_INSERT_IDLE_CYCLES 0
#endif

/*
 * How many to count to for half of a bit period.
 */
#if !defined(DAP_DELAY_CYCLES)
#define DAP_DELAY_CYCLES 25
#endif

static int swdio_pin = -1;
static int swclk_pin = -1;

enum {
	DAP_FRAME = 0x81,
	DAP_APnDP = 0x02,
	DAP_RnW = 0x04,
};

enum dap_status {
	DAP_OK = 1,
	DAP_WAIT = 2,
	DAP_FAULT = 4,
	DAP_ERROR = 7,
};

inline static void dap_delay(void)
{
	for (int i = 0; i < DAP_DELAY_CYCLES; i++)
		asm volatile("");
}

inline static void dap_idle(int __unused ticks)
{
#if DAP_INSERT_IDLE_CYCLES
	while (ticks--)
		dap_delay();
#endif
}

inline static void dap_edge(int edge)
{
	gpio_put(swclk_pin, edge);
	dap_delay();
}

inline static void dap_clock(int ticks)
{
	while (ticks--) {
		dap_edge(0);
		dap_edge(1);
	}
}

static void dap_write(uint32_t word, int len)
{
	while (len--) {
		gpio_put(swdio_pin, word & 1);
		dap_clock(1);
		word >>= 1;
	}

	/* Idle low */
	gpio_put(swdio_pin, 0);
}

static uint32_t dap_read(int len)
{
	uint32_t value = 0;

	for (int i = 0; i < len; i++) {
		dap_edge(0);
		value |= (uint32_t)gpio_get(swdio_pin) << i;
		dap_edge(1);
	}

	return value;
}

static void dap_turn(int dir)
{
	if (GPIO_OUT == dir) {
		dap_clock(1);
		gpio_set_dir(swdio_pin, dir);
	} else {
		gpio_set_dir(swdio_pin, dir);
		dap_clock(1);
	}
}

void dap_init(int swdio, int swclk)
{
	swdio_pin = swdio;
	swclk_pin = swclk;

	gpio_init(swdio_pin);
	gpio_set_pulls(swdio_pin, true, false);

	gpio_init(swclk_pin);

	gpio_set_dir(swdio_pin, GPIO_OUT);
	gpio_put(swdio_pin, 0);

	gpio_set_dir(swclk_pin, GPIO_OUT);
	gpio_put(swclk_pin, 1);
}

void dap_disconnect(void)
{
	gpio_set_dir(swdio_pin, GPIO_IN);
	gpio_set_dir(swclk_pin, GPIO_IN);
}

void dap_reset(void)
{
	/*
	 * Initial line reset to make sure we do not send a valid command
	 * into an already initialized link by accident.
	 */
	dap_write(0xffffffff, 32);
	dap_write(0x00ffffff, 32);
	dap_idle(8);

	/*
	 * B5.3.4 Leaving dormant state
	 *
	 * 1. Send at least eight SWCLKTCK cycles with SWDIOTMS HIGH.
	 */
	dap_write(0xff, 8);
	dap_idle(8);

	/* 2. Send the 128-bit Selection Alert sequence on SWDIOTMS. */
	dap_write(0x6209f392, 32);
	dap_write(0x86852d95, 32);
	dap_write(0xe3ddafe9, 32);
	dap_write(0x19bc0ea2, 32);
	dap_idle(8);

	/*
	 * 3. Send four SWCLKTCK cycles with SWDIOTMS LOW.
	 * 4. Send the required activation code sequence on SWDIOTMS.
	 */
	dap_write(0xf1a0, 16);
	dap_idle(8);

	/*
	 * B4.3.3 Connection and line reset sequence
	 *
	 * A line reset is achieved by holding the data signal HIGH for at
	 * least 50 clock cycles, followed by at least two idle cycles.
	 */
	dap_write(0xffffffff, 32);
	dap_write(0x00ffffff, 32);
	dap_idle(8);
}

static inline uint32_t dap_parity(uint32_t value)
{
	return __builtin_popcount(value) & 1;
}

static enum dap_status dap_try_put(uint8_t req, uint32_t value)
{
	dap_idle(8);

	dap_write(req, 8);
	dap_idle(2);

	dap_turn(GPIO_IN);
	enum dap_status status = dap_read(3);
	dap_turn(GPIO_OUT);
	dap_idle(2);

	if (DAP_OK != status)
		return status;

	dap_write(value, 32);
	dap_idle(1);

	dap_write(dap_parity(value), 1);
	dap_idle(2);

	return status;
}

bool dap_set_reg(enum dap_register reg, uint32_t value)
{
	uint8_t req = DAP_FRAME | reg | (dap_parity(reg) << 5);

	for (int i = 0; i < 32; i++) {
		enum dap_status status = dap_try_put(req, value);

		if (DAP_WAIT == status)
			continue;

		return DAP_OK == status;
	}

	puts("dap: stalled");
	return false;
}

static enum dap_status dap_try_read(uint8_t req, uint32_t *value)
{
	*value = 0xffffffff;

	dap_idle(8);

	dap_write(req, 8);
	dap_idle(2);

	dap_turn(GPIO_IN);
	dap_idle(2);

	enum dap_status status = dap_read(3);
	dap_idle(1);

	if (DAP_OK != status)
		goto fail;

	*value = dap_read(32);
	dap_idle(1);

	uint32_t parity = dap_read(1);
	dap_idle(1);

	if (dap_parity(*value) != parity)
		status = DAP_ERROR;

fail:
	dap_turn(GPIO_OUT);
	dap_idle(2);

	return status;
}

bool dap_get_reg(enum dap_register reg, uint32_t *value)
{
	uint8_t req = DAP_RnW | reg;
	req |= DAP_FRAME | (dap_parity(req) << 5);

	for (int i = 0; i < 32; i++) {
		enum dap_status status = dap_try_read(req, value);

		if (DAP_WAIT == status)
			continue;

		return DAP_OK == status;
	}

	puts("dap: stalled");
	return false;
}

void dap_select_target(uint32_t target)
{
	dap_idle(8);

	uint32_t req = DAP_DPc | dap_parity(DAP_DPc);
	dap_write(DAP_FRAME | req, 8);
	dap_idle(2);

	dap_turn(GPIO_IN);
	dap_read(3);
	dap_turn(GPIO_OUT);
	dap_idle(2);

	dap_write(target, 32);
	dap_idle(1);

	dap_write(dap_parity(target), 1);
	dap_idle(2);
}

uint32_t dap_read_idcode(void)
{
	uint32_t value;
	dap_get_reg(DAP_DP0, &value);
	return value;
}

bool dap_setup_mem(uint32_t *idr)
{
	uint32_t value;

	if (idr)
		*idr = 0xffffffff;

	/*
	 * Thanks Jeremy Bentham for his investigation:
	 * https://github.com/jbentham/picoreg/blob/main/picoreg_gpio.py#L298
	 */

	/* Clear error bits */
	if (!dap_set_reg(DAP_DP0, 0x1f)) {
		puts("dap: failed to set DP0");
		return false;
	}

	/* Set AP and DP bank 0 */
	if (!dap_set_reg(DAP_DP8, 0x00)) {
		puts("dap: failed to set DP8");
		return false;
	}

	/* Power up, disable sticky errs */
	if (!dap_set_reg(DAP_DP4, 0x51000f00)) {
		puts("dap: failed to set DP4");
		return false;
	}

	/* Read status */
	if (!dap_get_reg(DAP_DP4, &value)) {
		puts("dap: failed to get DP4");
		return false;
	}

	/* Set AP bank F, DP bank 0 */
	if (!dap_set_reg(DAP_DP8, 0xf0)) {
		puts("dap: failed to set DP8");
		return false;
	}

	/* Issue read AHB3-AP IDR */
	if (!dap_get_reg(DAP_APc, &value)) {
		puts("dap: failed to get APc");
		return false;
	}

	/* Obtain the value */
	if (!dap_get_reg(DAP_DPc, &value)) {
		puts("dap: failed to get DPc");
		return false;
	}

	/* Set AP bank D0, DP bank 0 */
	if (!dap_set_reg(DAP_DP8, 0xd00)) {
		puts("dap: failed to set DP8");
		return false;
	}

	/* Setup CSW */
	if (!dap_set_reg(DAP_AP0, 0x80000052)) {
		puts("dap: failed to set AP0");
		return false;
	}

	/* Set AP and DP bank 0 */
	if (!dap_set_reg(DAP_DP8, 0)) {
		puts("dap: failed to set DP8");
		return false;
	}

	if (idr)
		*idr = value;

	return true;
}

void dap_noop(void)
{
	dap_clock(8);
}

bool dap_peek(uint32_t addr, uint32_t *value)
{
	if (!dap_set_reg(DAP_AP4, addr))
		return false;

	if (!dap_get_reg(DAP_APc, value))
		return false;

	if (!dap_get_reg(DAP_DPc, value))
		return false;

	return true;
}

bool dap_peek_many(uint32_t addr, uint32_t *values, int len)
{
	if (!dap_set_reg(DAP_AP4, addr))
		return false;

	if (!dap_get_reg(DAP_APc, values))
		return false;

	while (len--)
		if (!dap_get_reg(DAP_APc, values++))
			return false;

	if (!dap_get_reg(DAP_DPc, values))
		return false;

	return true;
}

bool dap_poke(uint32_t addr, uint32_t value)
{
	if (!dap_set_reg(DAP_AP4, addr))
		return false;

	if (!dap_set_reg(DAP_APc, value))
		return false;

	return true;
}

bool dap_poke_many(uint32_t addr, const uint32_t *values, int len)
{
	if (!dap_set_reg(DAP_AP4, addr))
		return false;

	while (len--)
		if (!dap_set_reg(DAP_APc, *values++))
			return false;

	return true;
}
