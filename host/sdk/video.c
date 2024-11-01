#include <sdk.h>

void sdk_video_init(void)
{
	tft_init();
}

void sdk_set_backlight(unsigned level)
{
	(void)level;
}
