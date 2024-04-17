#include <sdk.h>

#include <task.h>

#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/stdio_usb.h>

#include <hardware/adc.h>
#include <hardware/clocks.h>

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

	fflush(stdout);
	exit(1);
}

void __noreturn sdk_main(const struct sdk_config *conf)
{
	set_sys_clock_khz(CLK_SYS_HZ / KHZ, true);
	clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, CLK_SYS_HZ,
			CLK_SYS_HZ);

	sdk_config = *conf;

	if (!sdk_config.backlight)
		sdk_config.backlight = SDK_BACKLIGHT_STD;

	sdk_slave_park();
	stdio_usb_init();

	if (sdk_config.wait_for_usb) {
		for (int i = 0; i < 30; i++) {
			if (stdio_usb_connected())
				break;

			sleep_ms(100);
		}
	}

	printf("sdk: Hello, welcome to Krecek!\n");

	adc_init();

	adc_gpio_init(USB_CC1_PIN);
	adc_gpio_init(USB_CC2_PIN);
	adc_gpio_init(BAT_VSENSE_PIN);
	adc_gpio_init(HP_SENSE_PIN);

	for (int i = 0; i < 16; i++)
		srand(adc_read() + random());

	/* Park the RF chip. */
	gpio_init(RF_CS_PIN);
	gpio_set_dir(RF_CS_PIN, GPIO_OUT);
	gpio_set_dir(RF_CS_PIN, 1);

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

		for (unsigned i = 0; i < NUM_CORES; i++)
			task_stats_report_reset(i);

		sdk_audio_report();
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
