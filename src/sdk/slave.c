#include <pico/stdlib.h>

#include <hardware/clocks.h>

#include <hardware/regs/resets.h>
#include <hardware/regs/xosc.h>
#include <hardware/regs/pll.h>
#include <hardware/regs/io_qspi.h>

#include <sdk.h>
#include <sdk/slave.h>

#include <stdio.h>

#include "dap.h"

uint32_t sdk_peek(uint32_t addr)
{
	uint32_t out;

	if (!dap_peek(addr, &out))
		sdk_panic("sdk: dap_sdk_peek(0x%08x) failed\n", (unsigned)addr);

	return out;
}

void sdk_poke(uint32_t addr, uint32_t value)
{
	if (!dap_poke(addr, value))
		sdk_panic("sdk: dap_sdk_poke(0x%08x, 0x%08x) failed\n", (unsigned)addr,
			  (unsigned)value);
}

static uint32_t unreset = RESETS_RESET_SYSINFO_BITS | RESETS_RESET_SYSCFG_BITS |
			  RESETS_RESET_PWM_BITS | RESETS_RESET_PLL_SYS_BITS |
			  RESETS_RESET_PADS_QSPI_BITS | RESETS_RESET_PADS_BANK0_BITS |
			  RESETS_RESET_IO_QSPI_BITS | RESETS_RESET_IO_BANK0_BITS;

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

	/* Output inverse of the crystal clock */
	gpio_set_outover(CLK_OUT_PIN, GPIO_OVERRIDE_INVERT);
	clock_gpio_init_int_frac(CLK_OUT_PIN, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_XOSC_CLKSRC, 1,
				 0);

	/* Start XOSC */
	sdk_poke(XOSC_BASE + XOSC_STARTUP_OFFSET, 0);
	sdk_poke(XOSC_BASE + XOSC_CTRL_OFFSET,
		 ((XOSC_CTRL_ENABLE_VALUE_ENABLE << XOSC_CTRL_ENABLE_LSB) |
		  (XOSC_CTRL_FREQ_RANGE_VALUE_1_15MHZ << XOSC_CTRL_FREQ_RANGE_LSB)));

	/* Un-reset subsystems we wish to use */
	sdk_poke(RESETS_BASE + RESETS_RESET_OFFSET, RESETS_RESET_BITS & ~unreset);

	/* Prevent power-off */
	sdk_poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * SLAVE_SELECT_PIN,
		 (1 << 3) | (1 << 6));
	sdk_poke(IO_QSPI_BASE + IO_QSPI_GPIO_QSPI_SCLK_CTRL_OFFSET + 8 * SLAVE_OFF_QSPI_PIN,
		 IO_QSPI_GPIO_QSPI_SCLK_CTRL_FUNCSEL_BITS);
}

void sdk_slave_init()
{
	uint32_t status = 0;

	while (true) {
		status = sdk_peek(RESETS_BASE + RESETS_RESET_DONE_OFFSET);
		if ((status & unreset) == unreset)
			break;
	}

	sdk_poke(PLL_SYS_BASE + PLL_FBDIV_INT_OFFSET, 132);
	sdk_poke(PLL_SYS_BASE + PLL_PRIM_OFFSET, 0x62000);

	status = sdk_peek(PLL_SYS_BASE + PLL_PWR_OFFSET);
	sdk_poke(PLL_SYS_BASE + PLL_PWR_OFFSET, status & ~(PLL_PWR_PD_BITS | PLL_PWR_VCOPD_BITS));

	while (true) {
		status = sdk_peek(PLL_SYS_BASE + PLL_CS_OFFSET);
		if (status & PLL_CS_LOCK_BITS)
			break;
	}

	sdk_poke(PLL_SYS_BASE + PLL_PWR_OFFSET, 0);

	puts("sdk: slave PLL_SYS locked");

	sdk_poke(CLOCKS_BASE + CLOCKS_CLK_SYS_CTRL_OFFSET,
		 CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX);

	sdk_poke(CLOCKS_BASE + CLOCKS_CLK_PERI_CTRL_OFFSET, CLOCKS_CLK_PERI_CTRL_ENABLE_BITS);

	sdk_poke(CLOCKS_BASE + CLOCKS_CLK_REF_CTRL_OFFSET,
		 CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC << CLOCKS_CLK_REF_CTRL_SRC_LSB);

	sdk_poke(CLOCKS_BASE + CLOCKS_CLK_ADC_DIV_OFFSET, 3 << CLOCKS_CLK_ADC_DIV_INT_LSB);
	sdk_poke(CLOCKS_BASE + CLOCKS_CLK_ADC_CTRL_OFFSET,
		 CLOCKS_CLK_ADC_CTRL_ENABLE_BITS | (CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS
						    << CLOCKS_CLK_ADC_CTRL_AUXSRC_LSB));

	unreset |= RESETS_RESET_ADC_BITS;
	sdk_poke(RESETS_BASE + RESETS_RESET_OFFSET, RESETS_RESET_BITS & ~unreset);

	while (true) {
		status = sdk_peek(RESETS_BASE + RESETS_RESET_DONE_OFFSET);
		if ((status & unreset) == unreset)
			break;
	}

	/* We can now increase the access speed. */
	dap_set_delay_cycles(0);
}
