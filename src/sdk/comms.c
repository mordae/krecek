#include <pico/stdlib.h>

#include <hardware/pwm.h>

#include <sdk.h>
#include <sdk/remote.h>

#include <mailbin.h>
#include <task.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned slice;
static unsigned chan;

static uint32_t buffer = 0;
static int bits_left = 0;

#define MAX_DEPTH 8
static uint32_t stack[MAX_DEPTH];
static int stack_depth;

bool sdk_send_ir(uint32_t word)
{
	if (stack_depth >= MAX_DEPTH)
		return false;

	stack[stack_depth++] = word;
	return true;
}

static int64_t send_ir_bit(__unused alarm_id_t alarm, __unused void *arg)
{
	if (bits_left <= 0) {
		pwm_set_chan_level(slice, chan, 0);

		if (stack_depth) {
			buffer = stack[--stack_depth];
			bits_left = 32;
			return -2000;
		}

		return -500;
	}

	int bit = buffer >> 31;
	buffer <<= 1;
	bits_left--;

#define CARRIER 12000
#define DEVIATION 2000

	if (bit) {
		pwm_set_wrap(slice, CLK_SYS_HZ / (CARRIER + DEVIATION));
		pwm_set_chan_level(slice, chan, CLK_SYS_HZ / (CARRIER + DEVIATION) / 32);
	} else {
		pwm_set_wrap(slice, CLK_SYS_HZ / (CARRIER - DEVIATION));
		pwm_set_chan_level(slice, chan, CLK_SYS_HZ / (CARRIER - DEVIATION) / 32);
	}

	return -500;
}

void sdk_decode_ir_raw(int16_t sample, int *oI, int *oQ, int *dm)
{
	static int seq = 0;
	static int I, Q;
	static int active = 0;

	if (0 == seq) {
		Q = sample;
		seq++;
	} else if (1 == seq) {
		I = sample;
		seq++;
	} else if (2 == seq) {
		Q -= sample;
		seq++;
	} else if (3 == seq) {
		I -= sample;
		seq = 0;

		if (abs(I) + abs(Q) >= 64) {
			active = 32;
		} else if (active > 0) {
			active--;
		} else {
			return;
		}

		goto demodulate;
	}

	return;

demodulate:
	static int prev_demod;
	static int32_t prev_phase;

	static uint32_t bits_wip;
	static uint32_t bits_len;
	static uint32_t window;
	static uint32_t decim_ctr;

	int32_t demod;

	if (abs(I) + abs(Q) < 64) {
		demod = 0;

		if (!prev_demod) {
			bits_len = 0;
			decim_ctr = 0;
			prev_phase = 0;
		}
	} else {
		int32_t phase = atan2f(Q, I) * 683565275.576f;
		demod = phase - prev_phase;
		prev_phase = phase;
	}

	if (oI)
		*oI = I;

	if (oQ)
		*oQ = Q;

	if (dm)
		*dm = demod >> 16;

	prev_demod = demod;

	if (!demod)
		return;

	window <<= 1;
	window |= (demod > 0);

	decim_ctr++;

	if (6 == decim_ctr) {
		decim_ctr = 0;
		bits_wip <<= 1;
		const uint32_t mask = (1 << 5) - 1;
		int pop = __builtin_popcount(window & mask);
		bits_wip |= (pop >= 3);
		bits_len++;

		if (32 == bits_len) {
			sdk_message_t msg = {
				.type = SDK_MSG_IR,
				.ir = { .data = bits_wip },
			};
			game_inbox(msg);
			bits_len = 0;
		}
	}
}

void sdk_decode_ir(int16_t sample)
{
	return sdk_decode_ir_raw(sample, NULL, NULL, NULL);
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

bool sdk_send_rf(uint8_t addr, const uint8_t *data, int len)
{
	if (len > SDK_RF_MAX) {
		printf("sdk: sdk_send_rf(len=%i) too long\n", len);
		return true;
	}

	int sizes[MAILBIN_RF_SLOTS];
	uint32_t addrs[MAILBIN_RF_SLOTS];

	remote_peek_many(MAILBIN_BASE + offsetof(struct mailbin, rf_tx_size), (uint32_t *)sizes,
			 MAILBIN_RF_SLOTS);

	remote_peek_many(MAILBIN_BASE + offsetof(struct mailbin, rf_tx_addr), addrs,
			 MAILBIN_RF_SLOTS);

	for (int i = 0; i < MAILBIN_RF_SLOTS; i++) {
		if (sizes[i])
			continue;

		static uint8_t buf[SDK_RF_MAX + 4] __aligned(SDK_RF_MAX + 4);

		buf[0] = len + 1;
		buf[1] = addr;
		memcpy(buf + 2, data, len);

		remote_poke_many(addrs[i], (void *)buf, (len + 2 + 3) >> 2);
		remote_poke(MAILBIN_BASE + offsetof(struct mailbin, rf_tx_size) + i * 4, len + 2);

		return true;
	}

	return false;
}

bool sdk_set_rf_channel(int ch)
{
	if (ch < 1 || ch > 69)
		return false;

	remote_poke(MAILBIN_BASE + offsetof(struct mailbin, rf_channel), ch);
	return true;
}
