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

#pragma once
#include <stdint.h>
#include <stdbool.h>

enum dap_register {
	DAP_DP0 = 0x00,
	DAP_DP4 = 0x08,
	DAP_DP8 = 0x10,
	DAP_DPc = 0x18,
	DAP_AP0 = 0x00 | 0x02,
	DAP_AP4 = 0x08 | 0x02,
	DAP_AP8 = 0x10 | 0x02,
	DAP_APc = 0x18 | 0x02,
};

/*
 * Initialize the DAP using following pins.
 *
 * You still need to follow the initialization sequence:
 *
 *  - dap_reset
 *  - dap_select_target (for multi-drop systems)
 *  - dap_read_idcode
 */
void dap_init(int swdio, int swclk);

/*
 * Adjust tick delay cycles on the fly.
 */
void dap_set_delay_cycles(int cycles);

/*
 * Reinitialize the communication link.
 */
void dap_reset(void);

/*
 * Select multidrop target.
 */
void dap_select_target(uint32_t target);

/*
 * Read IDCODE register.
 *
 * Returns 0xffffffff in case of error.
 * Mandatory last step of the initialization sequence.
 */
uint32_t dap_read_idcode(void);

/*
 * Configure target for memory access.
 *
 * Optionally obtain AHB3-AP IDR.
 */
bool dap_setup_mem(uint32_t *idr);

/*
 * If you do not intend to continue with another command, you should
 * issue a noop so that the DAP can finish any pending work.
 */
void dap_noop(void);

/*
 * Read contents of a register.
 */
bool dap_get_reg(enum dap_register reg, uint32_t *value);

/*
 * Write to a register.
 */
bool dap_set_reg(enum dap_register reg, uint32_t value);

/*
 * Read word from target's memory.
 */
bool dap_peek(uint32_t addr, uint32_t *value);

/*
 * Read multiple consecutive words from target's memory.
 */
bool dap_peek_many(uint32_t addr, uint32_t *values, int len);

/*
 * Write word to the target's memory.
 */
bool dap_poke(uint32_t addr, uint32_t value);

/*
 * Read multiple consecutive words to target's memory.
 */
bool dap_poke_many(uint32_t addr, const uint32_t *values, int len);
