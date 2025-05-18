#include "common.h"

#include <pico/stdio/driver.h>
#include <pico/time.h>

static void mailbin_stdio_out_char(char c)
{
	mailbin.stdout_buffer[mailbin.stdout_head % MAILBIN_STDOUT_SIZE] = c;
	mailbin.stdout_head++;
}

static void mailbin_stdio_out_chars(const char *buf, int length)
{
	for (int i = 0; i < length; i++)
		mailbin_stdio_out_char(buf[i]);
}

static int mailbin_stdio_in_chars(char *buf, int length)
{
	(void)buf;
	(void)length;
	return PICO_ERROR_NO_DATA;
}

static void mailbin_stdio_out_flush(void)
{
}

stdio_driver_t mailbin_stdio = {
	.out_chars = mailbin_stdio_out_chars,
	.in_chars = mailbin_stdio_in_chars,
	.out_flush = mailbin_stdio_out_flush,
	.crlf_enabled = false,
};

void mailbin_stdio_init(void)
{
	stdio_set_driver_enabled(&mailbin_stdio, true);
}
