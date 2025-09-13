#include <pico/flash.h>
#include <sdk.h>
#include <sdk/remote.h>

#include <task.h>

#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/stdio_usb.h>
#include <pico/rand.h>

#include <hardware/adc.h>
#include <hardware/clocks.h>
#include <hardware/structs/bus_ctrl.h>
#include <hardware/structs/xip_ctrl.h>
#include <hardware/watchdog.h>
#include <hardware/flash.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#if !defined(__weak)
#define __weak __attribute__((__weak__))
#endif

struct sdk_config sdk_config = {};
uint64_t sdk_device_id;

int current_slot = 0;

/* From audio.c */
void sdk_audio_init(void);
void sdk_audio_task(void);
void sdk_audio_start(void);
void sdk_audio_report(void);

/* From video.c */
void sdk_video_init(void);
void sdk_tft_task(void);
void sdk_paint_task(void);

/* From input.c */
void sdk_input_init(void);
void sdk_input_task(void);

/* From slave.c */
void sdk_slave_park(void);
void sdk_slave_init(void);
void sdk_slave_resume(void);

/* From comms.c */
void sdk_comms_init(void);

/* From sdcard_task.c */
void sdk_card_init(void);
void sdk_card_task(void);

static void sdk_stats_task(void);

task_t task_avail[NUM_CORES][MAX_TASKS] = {
	{
		MAKE_TASK(4, "stats", sdk_stats_task),
		MAKE_TASK(2, "input", sdk_input_task),
		MAKE_TASK(1, "paint", sdk_paint_task),
	},
	{
		MAKE_TASK(4, "tft", sdk_tft_task),
		MAKE_TASK(2, "audio", sdk_audio_task),
		MAKE_TASK(2, "card", sdk_card_task),
	},
};

void __attribute__((__noreturn__, __format__(printf, 1, 2))) sdk_panic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);

	puts("");

	fflush(stdout);
	exit(1);
}

#define BOOT_JUMP 0xb007
#define BOOT_LAND 0x50f7

void sdk_reboot_into_slot(unsigned slot)
{
	unsigned target = XIP_BASE + (0x80000 * (slot % 32) + 0x100);

	// Verify where we go before we leap.
	unsigned *firmware = (unsigned *)target;
	unsigned sp = firmware[0];
	unsigned pc = firmware[1];

	printf("Asked to reboot into: 0x%08x\n", target);
	printf("Stack pointer:        0x%08x\n", sp);
	printf("Reset handler:        0x%08x\n", pc);

	unsigned ram_base = 0x20000000u;
	unsigned ram_end = ram_base + 264 * 1024;

	if (sp < ram_base || sp > ram_end) {
		puts("Refusing to reboot with invalid stack pointer.");
		return;
	}

	if (pc < XIP_BASE || pc >= (XIP_BASE + 16 * 1024 * 1024)) {
		puts("Refusing to reboot with invalid reset handler.");
		return;
	}

	watchdog_hw->scratch[0] = BOOT_JUMP;
	watchdog_hw->scratch[1] = target;
	watchdog_enable(1, true);

	while (true)
		tight_loop_contents();
}

static void core1_main(void)
{
	flash_safe_execute_core_init();
	task_run_loop();
}

void __noreturn sdk_main(const struct sdk_config *conf)
{
	unsigned target = 0;

	if (BOOT_JUMP == watchdog_hw->scratch[0]) {
		// If a destination firmware has been configured before reboot,
		// we want to jump into that firmware. But clear the target,
		// so that we don't jump repeatedly.
		target = watchdog_hw->scratch[1];
		watchdog_hw->scratch[0] = BOOT_LAND;
		watchdog_hw->scratch[1] = 0;

		// Our games / partitions are arranged into 512 KiB slots.
		unsigned end = target | 0x0007ffff; // 512 KiB
		unsigned self = (unsigned)sdk_main;

		// If we are to jump to ourselves, we are done.
		if (self >= target && self <= end)
			goto target_reached;

		// Otherwise prepare SP, PC.
		unsigned *next = (unsigned *)target;
		unsigned sp = next[0];
		unsigned pc = next[1];

		// Configure clk_sys from clk_ref, which in turn runs from ROSC.
		clock_configure(clk_ref, CLOCKS_CLK_REF_CTRL_SRC_VALUE_ROSC_CLKSRC_PH, 0, 12000000,
				12000000);
		clock_configure(clk_sys, CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF, 0, 12000000,
				12000000);

		// Turn off interrupts (NVIC ICER, NVIC ICPR)
		hw_set_bits((io_rw_32 *)0xe000e180, 0xffffffff);
		hw_set_bits((io_rw_32 *)0xe000e280, 0xffffffff);

		scb_hw->vtor = target;
		asm volatile("msr msp, %0; bx %1" ::"r"(sp), "r"(pc));

		__builtin_unreachable();
	}

target_reached:

	/*
	 * Let the SDK know what slot have we booted from so that we can
	 * e.g. locate save games.
	 */
	current_slot = (target >> 14) & 31;

	/* Apply configuration. */
	sdk_config = *conf;

	if (!sdk_config.backlight)
		sdk_config.backlight = SDK_BACKLIGHT_STD;

	/*
	 * Program charger current limit, where:
	 *
	 * - FLOAT =  100 mA max
	 * - HIGH  =  500 mA max
	 * - LOW   = 1000 mA max
	 *
	 * Use the most strict limit initially.
	 */
	gpio_init(CHG_ISET2_PIN);
	gpio_set_pulls(CHG_ISET2_PIN, 0, 0);

	uint32_t t1, t0 = time_us_32();

	if (BOOT_LAND == watchdog_hw->scratch[0]) {
		/* Clear the flag so that we don't confuse picotool load. */
		watchdog_hw->scratch[0] = 0;

		/*
		 * Resume communication with slave.
		 *
		 * This way we don't turn outselves off by letting go of the
		 * SLAVE_OFF_PIN and RP2040 pulling it down via boot default.
		 */
		sdk_slave_resume();
		t1 = t0;
	} else {
		/* Park the slave chip. */
		sdk_slave_park();

		/* Initialize it, because that also latches our power. */
		sdk_slave_init();

		t1 = time_us_32();
	}

	/* Park ICs connected to this chip over SPI. */
	gpio_init(TFT_CS_PIN);
	gpio_init(SD_CS_PIN);
	gpio_set_pulls(TFT_CS_PIN, 1, 0);
	gpio_set_pulls(SD_CS_PIN, 1, 0);

	/* Initialize the ADC. */
	adc_init();
	adc_set_temp_sensor_enabled(true);
	adc_gpio_init(USB_CC1_PIN);
	adc_gpio_init(USB_CC2_PIN);
	adc_gpio_init(BAT_VSENSE_PIN);
	adc_gpio_init(HP_SENSE_PIN);

	/* Start USB stdio interface. */
	stdio_usb_init();

	/* Detect USB power. */
	adc_select_input(USB_CC1_PIN - 26);
	int cc1 = (3300 * adc_read()) >> 12;

	adc_select_input(USB_CC2_PIN - 26);
	int cc2 = (3300 * adc_read()) >> 12;

	/*
	 * When connected to 5V VBUS we should see 417 mV.
	 * Tolerance on pull-up is 20%, so the range is 352-511 mV.
	 * Voltage can also drop somewhat, especially with long cable.
	 */
	if (cc1 > 250.0f || cc2 > 250.0f) {
		/*
		 * When connected over USB, wait for a bit for the host
		 * to connect to us so that it doesn't miss early messages.
		 */
		if (sdk_config.wait_for_usb) {
			for (int i = 0; i < 30; i++) {
				if (stdio_usb_connected())
					break;

				sleep_ms(100);
			}
		}
	}

	puts("\n---- boot ----");
	puts("sdk: Hello, welcome to Krecek!");

	if (t1 - t0) {
		printf("sdk: slave programmed in %u μs\n", (unsigned)(t1 - t0));
	} else {
		printf("sdk: slave reused\n");
	}

	/*
	 * Seed random number generator from ADC inputs, including
	 * temperature sensor. This should be somewhat sufficient as
	 * those inputs are pretty noisy in the LSBs.
	 */
	for (int i = 0; i < 32; i++) {
		adc_select_input(i % 5);
		srand(adc_read() + rand());
	}

	/* Determine ID from our flash chip. */
	flash_get_unique_id((uint8_t *)&sdk_device_id);
	sdk_device_id = __builtin_bswap64(sdk_device_id);

	printf("sdk: device_id=%llx\n", sdk_device_id);

	task_init();

	sdk_video_init();
	puts("sdk: video init done");

	sdk_input_init();
	puts("sdk: input init done");

	sdk_audio_init();
	puts("sdk: audio init done");

	sdk_comms_init();
	puts("sdk: comms init done");

	sdk_card_init();
	puts("sdk: card init done");

	game_start();
	puts("sdk: game started");

	sdk_audio_start();
	puts("sdk: audio started");

	multicore_launch_core1(core1_main);
	puts("sdk: core1 launched");

	task_run_loop();
}

static void sdk_stats_task(void)
{
	while (true) {
		task_sleep_ms(10 * 1000);

		printf("\x1b[1;32m");

		for (unsigned i = 0; i < NUM_CORES; i++)
			task_stats_report_reset(i);

		printf("\x1b[1;33m");
		task_stats_memory_reset();

		printf("\x1b[1;36m");
		sdk_audio_report();

		printf("sdk: batt=%6.1f cc=%6.1f\n", sdk_inputs.batt_mv, sdk_inputs.cc_mv);

		unsigned hit = xip_ctrl_hw->ctr_hit;
		unsigned acc = xip_ctrl_hw->ctr_acc;

		xip_ctrl_hw->ctr_hit = 0;
		xip_ctrl_hw->ctr_acc = 0;

		float ratio = acc ? (float)hit / (float)acc : 1;
		unsigned misses = acc - hit;

		// Approximate cache miss cost.
		float cost = 16 * PICO_FLASH_SPI_CLKDIV / (float)CLK_SYS_HZ;

		printf("\x1b[1;36m");
		printf("sdk: xip cache hits: %10u / %10u = %5.1f%%\n", hit, acc, 100.0f * ratio);
		printf("sdk: misses / waits: %10u   %10s ~ %5.1f%%\n", misses, "",
		       misses * cost * 10.0f);

		printf("\x1b[0m");
	}
}

void sdk_yield_every_us(uint32_t us)
{
	static uint32_t last_yield[NUM_CORES] = { 0 };

	unsigned core = get_core_num();
	uint32_t last = last_yield[core];
	uint32_t now = time_us_32();

	if (now - last >= us) {
		task_yield();
		last_yield[core] = now;
	}
}

__weak void game_start(void)
{
}

__weak void game_reset(void)
{
}

__weak void game_audio(int nsamples)
{
	for (int i = 0; i < nsamples; i++) {
		int16_t left, right;
		sdk_melody_sample(&left, &right);
		sdk_write_sample(left, right);

		sdk_read_sample(&left, &right);
		sdk_decode_ir(left);
	}
}

__weak void game_input(unsigned __unused dt)
{
}

__weak void game_paint(unsigned __unused dt)
{
}

__weak void game_inbox(sdk_message_t msg)
{
	if (SDK_MSG_IR == msg.type) {
		printf("sdk: ir=%08x\n", (unsigned)msg.ir.data);
	}
}

uint32_t sdk_random()
{
	return get_rand_32();
}

void sdk_turn_off(void)
{
	remote_poke(SIO_BASE + SIO_GPIO_OUT_CLR_OFFSET, 1u << SLAVE_OFF_PIN);
}
