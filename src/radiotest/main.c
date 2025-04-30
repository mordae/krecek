#include <hardware/regs/clocks.h>
#include <pico/stdlib.h>

#include <stdio.h>
#include <tft.h>
#include <sdk.h>
#include <sdk/remote.h>

void game_start(void)
{
	remote_gpio_pad_set(0, 1, 0, 1, 0, 1, 0, SLAVE_RF_CS_PIN);
	remote_gpio_pad_set(0, 1, 0, 1, 0, 1, 0, SLAVE_TOUCH_MISO_PIN);
	remote_gpio_pad_set(0, 1, 0, 0, 1, 1, 0, SLAVE_TOUCH_MOSI_PIN);
	remote_gpio_pad_set(0, 1, 0, 0, 1, 1, 0, SLAVE_TOUCH_SCK_PIN);

	remote_gpio_pad_set(0, 1, 0, 0, 0, 1, 1, SLAVE_RF_XOSC_PIN);
	remote_poke(CLOCKS_BASE + CLOCKS_CLK_GPOUT0_DIV_OFFSET, 5 << 8);
	remote_poke(CLOCKS_BASE + CLOCKS_CLK_GPOUT0_CTRL_OFFSET, (1 << 12) | (1 << 11));
	remote_poke(IO_BANK0_BASE + IO_BANK0_GPIO0_CTRL_OFFSET + 8 * SLAVE_RF_XOSC_PIN,
		    IO_BANK0_GPIO21_CTRL_FUNCSEL_VALUE_CLOCKS_GPOUT_0);
}

static uint8_t rf_status;
static uint8_t rf_partnum;
static uint8_t rf_version;
static uint8_t rf_rssi;

static void exchange_bits(uint8_t *tx, uint8_t *rx)
{
	for (int i = 0; i < 8; i++) {
		*rx <<= 1;

		remote_gpio_set(SLAVE_TOUCH_MOSI_PIN, *tx >> 7);
		remote_gpio_set(SLAVE_TOUCH_SCK_PIN, 1);
		*rx |= remote_gpio_get(SLAVE_TOUCH_MISO_PIN);
		remote_gpio_set(SLAVE_TOUCH_SCK_PIN, 0);

		*tx <<= 1;
	}
}

static uint8_t read_register(uint8_t addr)
{
	remote_gpio_set(SLAVE_RF_CS_PIN, 0);

	while (remote_gpio_get(SLAVE_TOUCH_MISO_PIN))
		/* wait */;

	addr |= 128;
	exchange_bits(&addr, &rf_status);

	uint8_t data = 0, null = 0;
	exchange_bits(&null, &data);

	remote_gpio_set(SLAVE_RF_CS_PIN, 1);

	return data;
}

static void write_register(uint8_t addr, uint8_t data)
{
	remote_gpio_set(SLAVE_RF_CS_PIN, 0);

	while (remote_gpio_get(SLAVE_TOUCH_MISO_PIN))
		/* wait */;

	exchange_bits(&addr, &rf_status);

	uint8_t null;
	exchange_bits(&data, &null);

	remote_gpio_set(SLAVE_RF_CS_PIN, 1);
}

void game_input(unsigned dt_usec)
{
	(void)dt_usec;

	if (sdk_inputs_delta.a > 0) {
		rf_partnum = read_register(0xf0);
		rf_version = read_register(0xf1);
		rf_rssi = read_register(0xf4);
	}

	if (sdk_inputs_delta.b > 0) {
		write_register(0x02, 0x80); /* Enable temperature sensor. */
	}
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;

	tft_fill(0);

	char buf[64];
	sprintf(buf, "S%02hhx P%02hhx V%02hhx R%02hhx\n", rf_status, rf_partnum, rf_version,
		rf_rssi);
	tft_draw_string(0, 0, rgb_to_rgb565(255, 255, 255), buf);
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = false,
		.off_on_select = true,
		.fps_color = rgb_to_rgb565(31, 31, 31),
	};

	sdk_main(&config);
}
