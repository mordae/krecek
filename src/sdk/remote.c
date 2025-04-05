#include <pico/stdlib.h>

#include <hardware/regs/adc.h>
#include <hardware/regs/io_qspi.h>
#include <hardware/regs/io_bank0.h>
#include <hardware/regs/pads_qspi.h>

#include <dap.h>

#define REG_SET_FIELD(name, value) (((value) << (name##_LSB)) & (name##_BITS))

uint32_t remote_peek(uint32_t addr)
{
	uint32_t out;

	if (!dap_peek(addr, &out))
		panic("dap_peek(0x%08x) failed\n", (unsigned)addr);

	return out;
}

void remote_poke(uint32_t addr, uint32_t value)
{
	if (!dap_poke(addr, value))
		panic("dap_poke(0x%08x, 0x%08x) failed\n", (unsigned)addr, (unsigned)value);
}

int remote_gpio_get(int pin)
{
	uint32_t tmp = remote_peek(IO_BANK0_BASE + IO_BANK0_GPIO0_STATUS_OFFSET + 8 * pin);
	return (tmp >> IO_BANK0_GPIO0_STATUS_INFROMPAD_LSB) & 1;
}

void remote_gpio_set(int pin, bool value)
{
	uint32_t reg = 0;

	reg |= REG_SET_FIELD(IO_BANK0_GPIO0_CTRL_FUNCSEL, 31);
	reg |= REG_SET_FIELD(IO_BANK0_GPIO0_CTRL_OEOVER, 1);
	reg |= REG_SET_FIELD(IO_BANK0_GPIO0_CTRL_OUTOVER, value ? 3 : 2);

	remote_poke(IO_BANK0_BASE + IO_BANK0_GPIO0_CTRL_OFFSET + 8 * pin, reg);
}

void remote_gpio_clear(int pin)
{
	remote_poke(IO_BANK0_BASE + IO_BANK0_GPIO0_CTRL_OFFSET + 8 * pin,
		    IO_BANK0_GPIO0_CTRL_RESET);
}

int remote_qspi_get(int pin)
{
	uint32_t tmp = remote_peek(IO_QSPI_BASE + IO_QSPI_GPIO_QSPI_SCLK_STATUS_OFFSET + 8 * pin);
	return (tmp >> IO_QSPI_GPIO_QSPI_SCLK_STATUS_INFROMPAD_LSB) & 1;
}

void remote_qspi_set(int pin, bool value)
{
	uint32_t reg = 0;

	reg |= REG_SET_FIELD(IO_QSPI_GPIO_QSPI_SCLK_CTRL_OEOVER, 1);
	reg |= REG_SET_FIELD(IO_QSPI_GPIO_QSPI_SCLK_CTRL_OUTOVER, value ? 3 : 2);
	reg |= REG_SET_FIELD(IO_QSPI_GPIO_QSPI_SCLK_CTRL_FUNCSEL,
			     IO_QSPI_GPIO_QSPI_SCLK_CTRL_FUNCSEL_VALUE_NULL);

	remote_poke(IO_QSPI_BASE + IO_QSPI_GPIO_QSPI_SCLK_CTRL_OFFSET + 8 * pin, reg);
}

void remote_qspi_clear(int pin)
{
	remote_poke(IO_QSPI_BASE + IO_QSPI_GPIO_QSPI_SCLK_CTRL_OFFSET + 8 * pin,
		    IO_QSPI_GPIO_QSPI_SCLK_CTRL_RESET);
}

int remote_adc_read(int gpio)
{
	uint32_t reg = 0;

	reg |= REG_SET_FIELD(ADC_CS_AINSEL, gpio - 26);
	reg |= REG_SET_FIELD(ADC_CS_EN, 1);
	reg |= REG_SET_FIELD(ADC_CS_START_MANY, 1);

	remote_poke(ADC_BASE + ADC_CS_OFFSET, reg);

	uint32_t sample = remote_peek(ADC_BASE + ADC_RESULT_OFFSET);
	return sample;
}

void remote_gpio_pad_set(bool od, bool ie, int drive, bool pue, bool pde, bool schmitt,
			 bool slewfast, int pad)
{
	uint32_t reg = 0;

	reg |= REG_SET_FIELD(PADS_BANK0_GPIO0_OD, od);
	reg |= REG_SET_FIELD(PADS_BANK0_GPIO0_IE, ie);
	reg |= REG_SET_FIELD(PADS_BANK0_GPIO0_DRIVE, drive);
	reg |= REG_SET_FIELD(PADS_BANK0_GPIO0_PUE, pue);
	reg |= REG_SET_FIELD(PADS_BANK0_GPIO0_PDE, pde);
	reg |= REG_SET_FIELD(PADS_BANK0_GPIO0_SCHMITT, schmitt);
	reg |= REG_SET_FIELD(PADS_BANK0_GPIO0_SLEWFAST, slewfast);

	remote_poke(PADS_BANK0_BASE + PADS_BANK0_GPIO0_OFFSET + 4 * pad, reg);
}

void remote_qspi_pad_set(bool od, bool ie, int drive, bool pue, bool pde, bool schmitt,
			 bool slewfast, int pad)
{
	uint32_t reg = 0;

	reg |= REG_SET_FIELD(PADS_QSPI_GPIO_QSPI_SCLK_OD, od);
	reg |= REG_SET_FIELD(PADS_QSPI_GPIO_QSPI_SCLK_IE, ie);
	reg |= REG_SET_FIELD(PADS_QSPI_GPIO_QSPI_SCLK_DRIVE, drive);
	reg |= REG_SET_FIELD(PADS_QSPI_GPIO_QSPI_SCLK_PUE, pue);
	reg |= REG_SET_FIELD(PADS_QSPI_GPIO_QSPI_SCLK_PDE, pde);
	reg |= REG_SET_FIELD(PADS_QSPI_GPIO_QSPI_SCLK_SCHMITT, schmitt);
	reg |= REG_SET_FIELD(PADS_QSPI_GPIO_QSPI_SCLK_SLEWFAST, slewfast);

	// Pads have different order than controls. No idea why.
	uint8_t remap[6] = { 0, 5, 1, 2, 3, 4 };
	remote_poke(PADS_QSPI_BASE + PADS_QSPI_GPIO_QSPI_SCLK_OFFSET + 4 * remap[pad], reg);
}
