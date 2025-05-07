#include <hardware/regs/clocks.h>
#include <pico/stdlib.h>

#include <stdint.h>
#include <stdio.h>
#include <tft.h>
#include <sdk.h>
#include <sdk/remote.h>

static int position_x;
static int position_y;
static int position_z1;
static int position_z2;
static bool touching;

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

static void get_12_bit_output(int *rx)
{
    	*rx = 0;
	for (int i = 0; i < 12; i++) {
    		*rx <<= 1;

		remote_gpio_set(SLAVE_TOUCH_SCK_PIN, 1);
		*rx |= remote_gpio_get(SLAVE_TOUCH_MISO_PIN);
		remote_gpio_set(SLAVE_TOUCH_SCK_PIN, 0);
	}
}

void game_input(unsigned __unused dt_usec)
{
	uint8_t control_byte_x =  0b10010011;
	uint8_t control_byte_y =  0b11010011;
	uint8_t control_byte_z1 = 0b10110011;
	uint8_t control_byte_z2 = 0b11000011;
	uint8_t null = 0;

	remote_gpio_set(SLAVE_TOUCH_CS_PIN, 0);
	exchange_bits(&control_byte_x, &null);
        get_12_bit_output(&position_x);
        remote_gpio_set(SLAVE_TOUCH_CS_PIN, 1);

	remote_gpio_set(SLAVE_TOUCH_CS_PIN, 0);
	exchange_bits(&control_byte_y, &null);
	get_12_bit_output(&position_y);
	remote_gpio_set(SLAVE_TOUCH_CS_PIN, 1);

	remote_gpio_set(SLAVE_TOUCH_CS_PIN, 0);
	exchange_bits(&control_byte_z1, &null);
	get_12_bit_output(&position_z1);
	remote_gpio_set(SLAVE_TOUCH_CS_PIN, 1);

	remote_gpio_set(SLAVE_TOUCH_CS_PIN, 0);
	exchange_bits(&control_byte_z2, &null);
	get_12_bit_output(&position_z2);
	remote_gpio_set(SLAVE_TOUCH_CS_PIN, 1);

	if (position_z1 > 50) {
    		touching = true;
	} else {
    		touching = false;
	}
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(0);

	char buf[50];

	if (touching) {
    		snprintf(buf, 50, "touching!");
	} else {
    		snprintf(buf, 50, "not touching!");
	}

	tft_draw_string(0, 0, rgb565(255, 255, 255), "x%5i y%5i\n", position_x, position_y);
	tft_draw_string(0, 20, rgb565(255, 255, 255), "z1%5i z2%5i\n", position_z1, position_z2);
	tft_draw_string(0, 40, rgb_to_rgb565(255, 255, 255), buf);
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
