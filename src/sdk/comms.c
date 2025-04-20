#include <pico/stdlib.h>

#include <hardware/pwm.h>

#include <sdk.h>
#include <stdio.h>
#include <task.h>

static unsigned slice;
static unsigned chan;

static uint32_t buffer = 0;
static int bits_left = 0;

bool sdk_send_ir(uint32_t word)
{
	if (bits_left)
		return false;

	buffer = word;
	bits_left = 32;
	return true;
}

static int64_t send_ir_bit(__unused alarm_id_t alarm, __unused void *arg)
{
	if (bits_left <= 0) {
		pwm_set_chan_level(slice, chan, 0);
		return -1000;
	}

	int bit = buffer >> 31;
	buffer <<= 1;
	bits_left--;

	if (bit) {
		pwm_set_wrap(slice, CLK_SYS_HZ / 20000);
		pwm_set_chan_level(slice, chan, CLK_SYS_HZ / 20000 / 10);
	} else {
		pwm_set_wrap(slice, CLK_SYS_HZ / 10000);
		pwm_set_chan_level(slice, chan, CLK_SYS_HZ / 10000 / 10);
	}

	return -1000;
}

void sdk_comms_init(void)
{
	slice = pwm_gpio_to_slice_num(IR_TX_PIN);
	chan = pwm_gpio_to_channel(IR_TX_PIN);

	gpio_set_pulls(IR_TX_PIN, 0, 0);
	gpio_set_function(IR_TX_PIN, GPIO_FUNC_PWM);

	pwm_set_wrap(slice, CLK_SYS_HZ / 10000);
	pwm_set_chan_level(slice, chan, 0);
	pwm_set_enabled(slice, true);

	alarm_pool_init_default();
	(void)add_alarm_in_us(1000, send_ir_bit, NULL, true);
}
