#include <hardware/regs/clocks.h>
#include <pico/stdlib.h>

#include <pico/time.h>
#include <stdint.h>
#include <stdio.h>
#include <tft.h>
#include <sdk.h>
#include <sdk/remote.h>

static int current_position_x;
static int current_position_y;
static int current_position_z1;
static int current_position_z2;

static int recent_positions_x[8];
static int recent_positions_y[8];
static int recent_positions_z1[8];
static int recent_positions_z2[8];

static int oldest_position_x = 0;
static int oldest_position_y = 0;
static int oldest_position_z1 = 0;
static int oldest_position_z2 = 0;

static int total_x = 0;
static int total_y = 0;
static int total_z1 = 0;
static int total_z2 = 0;

static int average_position_x = 0;
static int average_position_y = 0;
static int average_position_z1 = 0; // appears to be directly propational to x position, y position, and pressure
static int average_position_z2 = 0;

static bool touching;

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

static void poll_touchscreen()
{
	uint8_t control_byte_x =  0b10010011;
	uint8_t control_byte_y =  0b11010011;
	uint8_t control_byte_z1 = 0b10110011;
	uint8_t control_byte_z2 = 0b11000011;
	uint8_t null = 0;

	remote_gpio_set(SLAVE_TOUCH_CS_PIN, 0);
	exchange_bits(&control_byte_x, &null);
        get_12_bit_output(&current_position_x);
        remote_gpio_set(SLAVE_TOUCH_CS_PIN, 1);

	remote_gpio_set(SLAVE_TOUCH_CS_PIN, 0);
	exchange_bits(&control_byte_y, &null);
	get_12_bit_output(&current_position_y);
	remote_gpio_set(SLAVE_TOUCH_CS_PIN, 1);

	remote_gpio_set(SLAVE_TOUCH_CS_PIN, 0);
	exchange_bits(&control_byte_z1, &null);
	get_12_bit_output(&current_position_z1);
	remote_gpio_set(SLAVE_TOUCH_CS_PIN, 1);

	remote_gpio_set(SLAVE_TOUCH_CS_PIN, 0);
	exchange_bits(&control_byte_z2, &null);
	get_12_bit_output(&current_position_z2);
	remote_gpio_set(SLAVE_TOUCH_CS_PIN, 1);

	total_x += current_position_x - recent_positions_x[oldest_position_x];
	recent_positions_x[oldest_position_x] = current_position_x;
	oldest_position_x = (oldest_position_x + 1) % 8;
	average_position_x = total_x >> 3;

	total_y += current_position_y - recent_positions_y[oldest_position_y];
	recent_positions_y[oldest_position_y] = current_position_y;
	oldest_position_y = (oldest_position_y + 1) % 8;
	average_position_y = total_y >> 3;

	total_z1 += current_position_z1 - recent_positions_z1[oldest_position_z1];
	recent_positions_z1[oldest_position_z1] = current_position_z1;
	oldest_position_z1 = (oldest_position_z1 + 1) % 8;
	average_position_z1 = total_z1 >> 3;

	total_z2 += current_position_z2 - recent_positions_z2[oldest_position_z2];
	recent_positions_z2[oldest_position_z2] = current_position_z2;
	oldest_position_z2 = (oldest_position_z2 + 1) % 8;
	average_position_z2 = total_z2 >> 3;

	if (average_position_z1 > 50) {
    		touching = true;
	} else {
    		touching = false;
	}
}

void game_start(void)
{
	remote_gpio_pad_set(0, 1, 0, 1, 0, 1, 0, SLAVE_TOUCH_CS_PIN);
	remote_gpio_pad_set(0, 1, 0, 1, 0, 1, 0, SLAVE_TOUCH_MISO_PIN);
	remote_gpio_pad_set(0, 1, 0, 0, 1, 1, 0, SLAVE_TOUCH_MOSI_PIN);
	remote_gpio_pad_set(0, 1, 0, 0, 1, 1, 0, SLAVE_TOUCH_SCK_PIN);

	for (int i = 0; i < 8; i++) {
		recent_positions_x[i] = 0;
		recent_positions_y[i] = 0;
		recent_positions_z1[i] = 0;
		recent_positions_z2[i] = 0;
	}
}

void game_input(unsigned __unused dt_usec)
{
        poll_touchscreen(); 
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

	tft_draw_string(0, 0, rgb565(255, 255, 255), "x%5i y%5i\n", average_position_x, average_position_y);
	tft_draw_string(0, 20, rgb565(255, 255, 255), "z1%5i z2%5i\n", average_position_z1, average_position_z2);
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
