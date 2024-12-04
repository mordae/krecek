#include <pico/stdlib.h>
#include <sys/time.h>
#include <stdlib.h>

uint32_t time_us_32()
{
	return time_us_64();
}

uint64_t time_us_64()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return 1000000ull * tv.tv_sec + tv.tv_usec;
}
