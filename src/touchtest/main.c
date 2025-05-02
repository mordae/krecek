#include <hardware/regs/clocks.h>
#include <pico/stdlib.h>

#include <stdint.h>
#include <stdio.h>
#include <tft.h>
#include <sdk.h>
#include <sdk/remote.h>

static uint8_t position_x;
static uint8_t position_y;

void game_start(void)
{
	remote_gpio_pad_set(0, 1, 0, 1, 0, 1, 0, SLAVE_TOUCH_CS_PIN);
	remote_gpio_pad_set(0, 1, 0, 1, 0, 1, 0, SLAVE_TOUCH_MISO_PIN);
	remote_gpio_pad_set(0, 1, 0, 0, 1, 1, 0, SLAVE_TOUCH_MOSI_PIN);
	remote_gpio_pad_set(0, 1, 0, 0, 1, 1, 0, SLAVE_TOUCH_SCK_PIN);
}

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

void game_input(unsigned __unused dt_usec)
{
	uint8_t control_byte_x = 0b11011011;
	uint8_t control_byte_y = 0b10011011;
	uint8_t null = 0;

	exchange_bits(&control_byte_x, &null); 
	// we may posibly need to wait here
	exchange_bits(&null, &position_x); 

	exchange_bits(&control_byte_y, &null); 
	// we may posibly need to wait here
	exchange_bits(&null, &position_y); 
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	char buf[64];
	sprintf(buf, "x%i y%i\n", position_x, position_y); 
	tft_draw_string(0, 0, rgb565(255, 255, 255) , buf); 
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
