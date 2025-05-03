#include <sdk.h>
#include <stdio.h>
#include <task.h>

#include <pico/mutex.h>

mutex_t sdk_sdcard_mutex;

enum sdcard_status sdk_sdcard_status = SDCARD_MISSING;
struct sdcard_csd sdk_sdcard_csd;

void sdk_card_task(void)
{
	static FATFS sdfs;
	bool mounted = false;

	mutex_enter_blocking(&sdk_sdcard_mutex);
	sdcard_init();

	while (true) {
		sdk_sdcard_status = sdcard_check();

		if (SDCARD_MISSING == sdk_sdcard_status) {
			sdcard_open();
			sdk_sdcard_status = sdcard_check();
		}

		mutex_exit(&sdk_sdcard_mutex);

		if (mounted && SDCARD_READY != sdk_sdcard_status) {
			f_mount(NULL, "", 1);
			puts("sdcard: un-mounted");
			mounted = false;
		} else if (!mounted && SDCARD_READY == sdk_sdcard_status) {
			mutex_enter_blocking(&sdk_sdcard_mutex);
			sdcard_read_csd(&sdk_sdcard_csd);
			mutex_exit(&sdk_sdcard_mutex);

			f_mount(&sdfs, "", 1);

			char label[32];
			DWORD serial;

			if (FR_OK == f_getlabel("", label, &serial)) {
				printf("sdcard: mounted \"%s\" (%08x) as /\n", label,
				       (unsigned)serial);
			} else {
				puts("sdcard: mounted as /");
			}

			mounted = true;
		}

		task_sleep_ms(250);
		mutex_enter_blocking(&sdk_sdcard_mutex);
	}
}

void sdk_card_init(void)
{
	mutex_init(&sdk_sdcard_mutex);
}
