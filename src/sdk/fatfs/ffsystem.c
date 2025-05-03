#include "ff.h"

#include <pico/stdlib.h>
#include <pico/mutex.h>

#if FF_USE_LFN == 3 /* Use dynamic memory allocation */
#include <stdlib.h>

void *ff_memalloc(UINT msize)
{
	return malloc((size_t)msize);
}

void ff_memfree(void *mblock)
{
	free(mblock);
}
#endif

auto_init_recursive_mutex(fatfs_mutex);

int ff_mutex_create(int vol)
{
	(void)vol;
	return true;
}

void ff_mutex_delete(int vol)
{
	(void)vol;
}

int ff_mutex_take(int vol)
{
	(void)vol;
	return recursive_mutex_enter_timeout_ms(&fatfs_mutex, FF_FS_TIMEOUT);
}

void ff_mutex_give(int vol)
{
	(void)vol;
	recursive_mutex_exit(&fatfs_mutex);
}
