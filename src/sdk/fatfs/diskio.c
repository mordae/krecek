#include <pico/stdlib.h>
#include <sdk.h>

#include "ff.h"
#include "diskio.h"

DSTATUS disk_status(BYTE pdrv)
{
	if (0 != pdrv)
		panic("diskio: attempted to use drive %hhu", pdrv);

	enum sdcard_status status = sdcard_check();

	if (SDCARD_READY == status)
		return 0;

	return STA_NOINIT;
}

DSTATUS disk_initialize(BYTE pdrv)
{
	if (0 == disk_status(pdrv))
		return 0;

	sdcard_open();

	return disk_status(pdrv);
}

DRESULT disk_read(BYTE pdrv,	/* Physical drive nmuber to identify the drive */
		  BYTE *buff,	/* Data buffer to store read data */
		  LBA_t sector, /* Start sector in LBA */
		  UINT count	/* Number of sectors to read */
)
{
	if (0 != pdrv)
		panic("diskio: attempted to use drive %hhu", pdrv);

	if (0 != disk_status(pdrv))
		return RES_NOTRDY;

	if (0 == count)
		return RES_OK;

	if (1 == count) {
		if (sdcard_read_block(sector, buff))
			return RES_OK;

		return RES_ERROR;
	}

	if (!sdcard_begin_read(sector))
		return RES_ERROR;

	DRESULT dres = RES_OK;

	for (UINT i = 0; i < count; i++) {
		if (!sdcard_read_next(buff + 512 * i)) {
			dres = RES_ERROR;
			break;
		}
	}

	sdcard_end_read();
	return dres;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(BYTE pdrv,	     /* Physical drive nmuber to identify the drive */
		   const BYTE *buff, /* Data to be written */
		   LBA_t sector,     /* Start sector in LBA */
		   UINT count	     /* Number of sectors to write */
)
{
	if (0 != pdrv)
		panic("diskio: attempted to use drive %hhu", pdrv);

	if (0 != disk_status(pdrv))
		return RES_NOTRDY;

	if (0 == count)
		return RES_OK;

	if (1 == count) {
		if (sdcard_write_block(sector, buff))
			return RES_OK;

		return RES_ERROR;
	}

	if (!sdcard_begin_write(sector))
		return RES_ERROR;

	DRESULT dres = RES_OK;

	for (UINT i = 0; i < count; i++) {
		if (!sdcard_write_next(buff + 512 * i)) {
			dres = RES_ERROR;
			break;
		}
	}

	sdcard_end_write();
	return dres;
}

#endif

DRESULT disk_ioctl(BYTE pdrv, /* Physical drive number */
		   BYTE cmd,  /* Control code */
		   void *buff /* Buffer to send/receive control data */
)
{
	if (0 != pdrv)
		panic("diskio: attempted to use drive %hhu", pdrv);

	if (0 != disk_status(pdrv))
		return RES_NOTRDY;

	switch (cmd) {
	case CTRL_SYNC:
		return RES_OK;

	case GET_SECTOR_COUNT:
		LBA_t *seccount = buff;
		(*seccount) = sdk_sdcard_csd.size / 512;
		return RES_OK;

	case GET_SECTOR_SIZE:
		WORD *size = buff;
		(*size) = 512;
		return RES_OK;

	case GET_BLOCK_SIZE:
		DWORD *blksize = buff;
		(*blksize) = 1;
		return RES_OK;

	case CTRL_TRIM:
		return RES_OK;
	}

	return RES_PARERR;
}
