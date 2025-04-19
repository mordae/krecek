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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#if !defined(__weak)
#define __weak __attribute__((__weak__))
#endif

struct sdk_config sdk_config = {};

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

static void sdk_stats_task(void);

task_t task_avail[NUM_CORES][MAX_TASKS] = {
	{
		MAKE_TASK(4, "stats", sdk_stats_task),
		MAKE_TASK(2, "input", sdk_input_task),
		MAKE_TASK(1, "paint", sdk_paint_task),
	},
	{
		MAKE_TASK(4, "tft", sdk_tft_task),
		MAKE_TASK(1, "audio", sdk_audio_task),
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

void __noreturn sdk_main(const struct sdk_config *conf)
{
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

	/* Adjust system clock before we start doing anything. */
	set_sys_clock_khz(CLK_SYS_HZ / KHZ, true);
	clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, CLK_SYS_HZ,
			CLK_SYS_HZ);

	/* Park the slave chip. */
	sdk_slave_park();

	/* Prevent power-off. */
	remote_gpio_pad_set(0, 1, 1, 1, 0, 1, 0, SLAVE_OFF_PIN);
	remote_gpio_set(SLAVE_OFF_PIN, 1);

	/* Park ICs connected to this chip over SPI. */
	gpio_init(TFT_CS_PIN);
	gpio_init(SD_CS_PIN);
	gpio_set_pulls(TFT_CS_PIN, 1, 0);
	gpio_set_pulls(SD_CS_PIN, 1, 0);

	/* Park ICs connected to slave over SPI. */
	remote_gpio_pad_set(0, 1, 1, 1, 0, 1, 0, SLAVE_RF_CS_PIN);
	remote_gpio_pad_set(0, 1, 1, 1, 0, 1, 0, SLAVE_TOUCH_CS_PIN);

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

	printf("sdk: Hello, welcome to Krecek!\n");

	/* Seed random number generator from ADC. */
	for (int i = 0; i < 16; i++) {
		adc_select_input(i % 5);
		srand(adc_read() + random());
	}

	task_init();

	sdk_slave_init();
	puts("sdk: slave init done");

	sdk_video_init();
	puts("sdk: video init done");

	sdk_input_init();
	puts("sdk: input init done");

	sdk_audio_init();
	puts("sdk: audio init done");

	game_start();

	sdk_audio_start();
	multicore_launch_core1(task_run_loop);
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
	(void)nsamples;
}

__weak void game_input(unsigned __unused dt)
{
}

__weak void game_paint(unsigned __unused dt)
{
}

uint32_t sdk_random()
{
	return get_rand_32();
}

void sdk_turn_off(void)
{
	remote_gpio_set(SLAVE_OFF_PIN, 0);
}
