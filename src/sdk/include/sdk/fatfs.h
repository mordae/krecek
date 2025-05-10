#pragma once
#include "sdcard.h"

extern enum sdcard_status sdk_sdcard_status;
extern struct sdcard_csd sdk_sdcard_csd;

#if defined(KRECEK)
#include "../../fatfs/ff.h"
#else
#include "hostff.h"
#endif

extern const char *f_strerror(int err);
