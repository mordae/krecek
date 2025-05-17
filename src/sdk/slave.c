#include <pico/stdlib.h>

#include <hardware/clocks.h>

#include <hardware/regs/resets.h>
#include <hardware/regs/xosc.h>
#include <hardware/regs/pll.h>
#include <hardware/regs/io_qspi.h>
#include <hardware/regs/watchdog.h>
#include <hardware/regs/m0plus.h>

#include <sdk.h>
#include <sdk/remote.h>

#include <stdio.h>

#include <slave.bin.h>

#include "dap.h"
#include <mailbin.h>

#define UNRESET                                                                             \
	(RESETS_RESET_SYSINFO_BITS | RESETS_RESET_SYSCFG_BITS | RESETS_RESET_PLL_SYS_BITS | \
	 RESETS_RESET_TIMER_BITS)

void sdk_slave_park()
{
	dap_init(DAP_SWDIO_PIN, DAP_SWCLK_PIN);

	dap_reset();
	dap_select_target(DAP_RESCUE);
	unsigned idcode = dap_read_idcode();

	const unsigned CDBGPWRUPREQ = 1u << 28;
	dap_set_reg(DAP_DP4, CDBGPWRUPREQ);
	dap_set_reg(DAP_DP4, 0);

	dap_reset();
	dap_select_target(DAP_CORE0);
	idcode = dap_read_idcode();
	dap_setup_mem((uint32_t *)&idcode);
	dap_noop();
}

void sdk_slave_init()
{
	/* Output inverse of the crystal clock */
	gpio_set_outover(CLK_OUT_PIN, GPIO_OVERRIDE_INVERT);
	clock_gpio_init_int_frac(CLK_OUT_PIN, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_XOSC_CLKSRC, 1,
				 0);

#if 1
	/* Start XOSC */
	remote_poke(XOSC_BASE + XOSC_STARTUP_OFFSET, 0);
	remote_poke(XOSC_BASE + XOSC_CTRL_OFFSET,
		    ((XOSC_CTRL_ENABLE_VALUE_ENABLE << XOSC_CTRL_ENABLE_LSB) |
		     (XOSC_CTRL_FREQ_RANGE_VALUE_1_15MHZ << XOSC_CTRL_FREQ_RANGE_LSB)));

	/* Un-reset subsystems we wish to use */
	remote_poke(RESETS_BASE + RESETS_RESET_OFFSET, RESETS_RESET_BITS & ~UNRESET);

	uint32_t status = 0;

	while (true) {
		status = remote_peek(RESETS_BASE + RESETS_RESET_DONE_OFFSET);
		if ((status & UNRESET) == UNRESET)
			break;
	}

	/* Clock at roughly the same frequency as master. */
	remote_poke(PLL_SYS_BASE + PLL_FBDIV_INT_OFFSET, CLK_SYS_HZ / 1000000);
	remote_poke(PLL_SYS_BASE + PLL_PRIM_OFFSET, 0x62000);

	status = remote_peek(PLL_SYS_BASE + PLL_PWR_OFFSET);
	remote_poke(PLL_SYS_BASE + PLL_PWR_OFFSET,
		    status & ~(PLL_PWR_PD_BITS | PLL_PWR_VCOPD_BITS));

	while (true) {
		status = remote_peek(PLL_SYS_BASE + PLL_CS_OFFSET);
		if (status & PLL_CS_LOCK_BITS)
			break;
	}

	remote_poke(PLL_SYS_BASE + PLL_PWR_OFFSET, 0);

	remote_poke(CLOCKS_BASE + CLOCKS_CLK_REF_CTRL_OFFSET,
		    CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC);

	remote_poke(CLOCKS_BASE + CLOCKS_CLK_SYS_CTRL_OFFSET,
		    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF);

	remote_poke(CLOCKS_BASE + CLOCKS_CLK_PERI_CTRL_OFFSET, CLOCKS_CLK_PERI_CTRL_ENABLE_BITS);

	/* We can now increase the access speed. */
	dap_set_delay_cycles(0);
#endif

	/* Upload firmware. */
	const uint32_t *firmware = (const uint32_t *)slave_bin;

	static_assert((SLAVE_BIN_SIZE & 3) == 0);

	for (int i = 0; i < SLAVE_BIN_SIZE >> 2; i++) {
		if (!dap_poke(SRAM_BASE + (i << 2), firmware[i]))
			sdk_panic("slave firmware upload failed");
	}

	puts("sdk: slave firmware uploaded");

	/* Wipe mailbin magic. */
	dap_poke(MAILBIN_BASE, 0);

	/* Halt */
	dap_poke(PPB_BASE + 0xedf0, 0xa05f000b);

	/* Write target address to PC */
	dap_poke(PPB_BASE + 0xedf8, SRAM_BASE);
	dap_poke(PPB_BASE + 0xedf4, 0x0001000f);

	/* Write end of RAM to SP */
	dap_poke(PPB_BASE + 0xedf8, SRAM_END);
	dap_poke(PPB_BASE + 0xedf4, 0x00010012);

	/*
	 * Since the firmware will reconfigure PLL,
	 * we cannot keep our high speed.
	 */
	dap_set_delay_cycles(DAP_DELAY_CYCLES);

#if 0
	for (int i = 0; i < 1024; i++) {
		if (!dap_poke(PPB_BASE + 0xedf4, 0x0000000f)) {
			puts("poke at 0xedf4 failed");
			break;
		}

		uint32_t pc;
		if (!dap_peek(PPB_BASE + 0xedf8, &pc)) {
			puts("peek at 0xedf8 failed");
			break;
		}

		if (0x00000030 == pc || 0xffffffff == pc) {
			printf("PC = %08x\n", (unsigned)pc);
			break;
		}

		uint32_t load = pc & ~3u;
		uint32_t insn;
		if (!dap_peek(load, &insn))
			puts("peek at pc failed");

		if (pc & 3)
			insn >>= 16;

		dap_poke(PPB_BASE + 0xedf0, 0xa05f000d);

		uint32_t dhcsr;
		if (!dap_peek(PPB_BASE + 0xedf0, &dhcsr)) {
			puts("peek at DHCSR failed");
			break;
		}

		printf("%08x: %04x  ; => %08x\n", (unsigned)pc, (unsigned)insn & 0xffff,
		       (unsigned)dhcsr);
	}
#endif

	/* Resume the firmware. */
	dap_poke(PPB_BASE + 0xedf0, 0xa05f0000);

	/* Wait for PLL lock. */
	while (true) {
		uint32_t status = remote_peek(PLL_SYS_BASE + PLL_CS_OFFSET);
		if (status & PLL_CS_LOCK_BITS)
			break;

		sleep_us(10);
	}

	/* OK. */
	dap_set_delay_cycles(0);
}

bool sdk_slave_fetch_mailbin(struct mailbin *bin)
{
	return dap_peek_many(MAILBIN_BASE, (void *)bin, sizeof(*bin) / 4);
}
