#pragma once
#include <stdint.h>

typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint64_t QWORD;

typedef DWORD FSIZE_t;
typedef DWORD LBA_t;

typedef char TCHAR;
#define _T(x) x
#define _TEXT(x) x

typedef struct {
	void *fp;
} FIL;

typedef struct {
	void *dp;
	char *path;
} DIR;

typedef struct {
	FSIZE_t fsize;	 /* File size */
	WORD fdate;	 /* Modified date */
	WORD ftime;	 /* Modified time */
	BYTE fattrib;	 /* File attribute */
	TCHAR fname[13]; /* Primary file name */
} FILINFO;

typedef enum {
	FR_OK = 0,	    /* (0) Succeeded */
	FR_DISK_ERR,	    /* (1) A hard error occurred in the low level disk I/O layer */
	FR_INT_ERR,	    /* (2) Assertion failed */
	FR_NOT_READY,	    /* (3) The physical drive cannot work */
	FR_NO_FILE,	    /* (4) Could not find the file */
	FR_NO_PATH,	    /* (5) Could not find the path */
	FR_INVALID_NAME,    /* (6) The path name format is invalid */
	FR_DENIED,	    /* (7) Access denied due to prohibited access or directory full */
	FR_EXIST,	    /* (8) Access denied due to prohibited access */
	FR_INVALID_OBJECT,  /* (9) The file/directory object is invalid */
	FR_WRITE_PROTECTED, /* (10) The physical drive is write protected */
	FR_INVALID_DRIVE,   /* (11) The logical drive number is invalid */
	FR_NOT_ENABLED,	    /* (12) The volume has no work area */
	FR_NO_FILESYSTEM,   /* (13) There is no valid FAT volume */
	FR_MKFS_ABORTED,    /* (14) The f_mkfs() aborted due to any problem */
	FR_TIMEOUT, /* (15) Could not get a grant to access the volume within defined period */
	FR_LOCKED,  /* (16) The operation is rejected according to the file sharing policy */
	FR_NOT_ENOUGH_CORE,	/* (17) LFN working buffer could not be allocated */
	FR_TOO_MANY_OPEN_FILES, /* (18) Number of open files > FF_FS_LOCK */
	FR_INVALID_PARAMETER	/* (19) Given parameter is invalid */
} FRESULT;

FRESULT f_open(FIL *fp, const TCHAR *path, BYTE mode);
FRESULT f_close(FIL *fp);
FRESULT f_read(FIL *fp, void *buff, UINT btr, UINT *br);
FRESULT f_write(FIL *fp, const void *buff, UINT btw, UINT *bw);
FRESULT f_lseek(FIL *fp, FSIZE_t ofs);
FRESULT f_truncate(FIL *fp);
FRESULT f_sync(FIL *fp);
FRESULT f_opendir(DIR *dp, const TCHAR *path);
FRESULT f_closedir(DIR *dp);
FRESULT f_readdir(DIR *dp, FILINFO *fno);
FRESULT f_mkdir(const TCHAR *path);
FRESULT f_unlink(const TCHAR *path);
FRESULT f_rename(const TCHAR *path_old, const TCHAR *path_new);
FRESULT f_stat(const TCHAR *path, FILINFO *fno);
int f_putc(TCHAR c, FIL *fp);
int f_puts(const TCHAR *str, FIL *cp);
int f_printf(FIL *fp, const TCHAR *str, ...) __attribute__((format(printf, 2, 3)));
TCHAR *f_gets(TCHAR *buff, int len, FIL *fp);

bool f_eof(FIL *fp);
BYTE f_error(FIL *fp);
FSIZE_t f_tell(FIL *fp);
FSIZE_t f_size(FIL *fp);
FRESULT f_rmdir(const TCHAR *path);

#define f_rewind(fp) f_lseek((fp), 0)
#define f_rewinddir(dp) f_readdir((dp), 0)

#define FA_READ 0x01
#define FA_WRITE 0x02
#define FA_OPEN_EXISTING 0x00
#define FA_CREATE_NEW 0x04
#define FA_CREATE_ALWAYS 0x08
#define FA_OPEN_ALWAYS 0x10
#define FA_OPEN_APPEND 0x30

#define CREATE_LINKMAP ((FSIZE_t)0 - 1)

/* File attribute bits for directory entry (FILINFO.fattrib) */
#define AM_RDO 0x01 /* Read only */
#define AM_HID 0x02 /* Hidden */
#define AM_SYS 0x04 /* System */
#define AM_DIR 0x10 /* Directory */
#define AM_ARC 0x20 /* Archive */
