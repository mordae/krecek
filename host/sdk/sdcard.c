#include <errno.h>
#include <sdk/sdcard.h>
#include <stdarg.h>
#include <stdlib.h>
#define DIR DIR_
#include <sdk/fatfs.h>
#undef DIR

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

enum sdcard_status sdk_sdcard_status = SDCARD_READY;
struct sdcard_csd sdk_sdcard_csd = {
	.size = 32ull << 30, // 32 GiB
	.speed = 25 << 20,   // 25 MHz
};

FRESULT f_open(FIL *fp, const TCHAR *path, BYTE mode)
{
	if (!fp || fp->fp)
		return FR_INVALID_OBJECT;

	if (!path)
		return FR_INVALID_NAME;

	while (path[0] == '/' || path[0] == '.')
		path++;

	if (!path[0])
		path = ".";

	const char *fmode;

	switch (mode & ~FA_OPEN_ALWAYS) {
	case FA_READ:
		fmode = "r";
		break;

	case FA_READ | FA_WRITE:
		fmode = "r+";
		break;

	case FA_CREATE_ALWAYS | FA_WRITE:
		fmode = "w";
		break;

	case FA_CREATE_ALWAYS | FA_WRITE | FA_READ:
		fmode = "w+";
		break;

	case FA_OPEN_APPEND | FA_WRITE:
		fmode = "a";
		break;

	case FA_OPEN_APPEND | FA_WRITE | FA_READ:
		fmode = "a+";
		break;

	case FA_CREATE_NEW | FA_WRITE:
		fmode = "wx";
		break;

	case FA_CREATE_NEW | FA_WRITE | FA_READ:
		fmode = "w+x";
		break;

	default:
		return FR_INVALID_PARAMETER;
	}

	fp->fp = fopen(path, fmode);

	if (!fp->fp) {
		if (EEXIST == errno)
			return FR_EXIST;

		return FR_NO_PATH;
	}

	return FR_OK;
}

FRESULT f_close(FIL *fp)
{
	if (!fp || !fp->fp)
		return FR_INVALID_OBJECT;

	fclose(fp->fp);
	fp->fp = NULL;

	return FR_OK;
}

FRESULT f_read(FIL *fp, void *buff, UINT btr, UINT *br)
{
	if (!fp || !fp->fp)
		return FR_INVALID_OBJECT;

	*br = fread(buff, 1, btr, fp->fp);
	return FR_OK;
}

FRESULT f_write(FIL *fp, const void *buff, UINT btw, UINT *bw)
{
	if (!fp || !fp->fp)
		return FR_INVALID_OBJECT;

	*bw = fwrite(buff, 1, btw, fp->fp);
	return FR_OK;
}

FRESULT f_lseek(FIL *fp, FSIZE_t ofs)
{
	if (!fp || !fp->fp)
		return FR_INVALID_OBJECT;

	fseek(fp->fp, ofs, SEEK_SET);
	return FR_OK;
}

FRESULT f_truncate(FIL *fp)
{
	if (!fp || !fp->fp)
		return FR_INVALID_OBJECT;

	ftruncate(fileno(fp->fp), 0);
	return FR_OK;
}

FRESULT f_sync(FIL *fp)
{
	if (!fp || !fp->fp)
		return FR_INVALID_OBJECT;

	fflush(fp->fp);
	fsync(fileno(fp->fp));
	return FR_OK;
}

FRESULT f_opendir(DIR_ *dp, const TCHAR *path)
{
	while (path[0] == '/' || path[0] == '.')
		path++;

	if (!path[0])
		path = ".";

	if (!dp || dp->dp)
		return FR_INVALID_OBJECT;

	dp->dp = opendir(path);
	if (!dp->dp)
		return FR_NO_PATH;

	dp->path = strdup(path);
	return FR_OK;
}

FRESULT f_closedir(DIR_ *dp)
{
	if (!dp || !dp->dp)
		return FR_INVALID_OBJECT;

	closedir(dp->dp);
	free(dp->path);

	dp->dp = NULL;
	dp->path = NULL;

	return FR_OK;
}

FRESULT f_readdir(DIR_ *dp, FILINFO *fno)
{
	if (!dp || !dp->dp)
		return FR_INVALID_OBJECT;

next:
	struct dirent *de = readdir(dp->dp);

	if (!de) {
		memset(fno, 0, sizeof(*fno));
		return FR_OK;
	}

	if (0 == strcmp(de->d_name, "."))
		goto next;

	if (0 == strcmp(de->d_name, ".."))
		goto next;

	if (strlen(de->d_name) > 11)
		goto next;

	strlcpy(fno->fname, de->d_name, 13);
	fno->fattrib = (de->d_type == DT_DIR ? AM_DIR : 0);

	char path[PATH_MAX];
	strlcpy(path, dp->path, PATH_MAX);
	strlcat(path, "/", PATH_MAX);
	strlcat(path, de->d_name, PATH_MAX);

	struct stat stbuf;
	if (0 == stat(path, &stbuf)) {
		fno->fattrib = S_ISDIR(stbuf.st_mode) ? AM_DIR : 0;
		fno->fsize = stbuf.st_size;
	}

	return FR_OK;
}

FRESULT f_mkdir(const TCHAR *path)
{
	while (path[0] == '/' || path[0] == '.')
		path++;

	if (0 == mkdir(path, 0777))
		return FR_OK;

	if (EEXIST == errno)
		return FR_EXIST;

	return FR_NO_PATH;
}

FRESULT f_unlink(const TCHAR *path)
{
	while (path[0] == '/' || path[0] == '.')
		path++;

	if (0 == unlink(path))
		return FR_OK;

	return FR_NO_PATH;
}

FRESULT f_rename(const TCHAR *path_old, const TCHAR *path_new)
{
	while (path_old[0] == '/' || path_old[0] == '.')
		path_old++;

	while (path_new[0] == '/' || path_new[0] == '.')
		path_new++;

	if (0 == rename(path_old, path_new))
		return FR_OK;

	return FR_NO_PATH;
}

FRESULT f_stat(const TCHAR *path, FILINFO *fno)
{
	if (!fno)
		return FR_INVALID_OBJECT;

	while (path[0] == '/' || path[0] == '.')
		path++;

	memset(fno, 0, sizeof(*fno));

	const char *slash = strchr(path, '/');

	if (slash) {
		strlcpy(fno->fname, slash + 1, 13);
	} else {
		strlcpy(fno->fname, path, 13);
	}

	struct stat stbuf;
	if (0 == stat(path, &stbuf)) {
		fno->fattrib = S_ISDIR(stbuf.st_mode) ? AM_DIR : 0;
		fno->fsize = stbuf.st_size;
	}

	return FR_OK;
}

int f_putc(TCHAR c, FIL *fp)
{
	if (!fp || !fp->fp)
		return -1;

	return fputc(c, fp->fp);
}

int f_puts(const TCHAR *str, FIL *fp)
{
	if (!fp || !fp->fp)
		return -1;

	return fputs(str, fp->fp);
}

int f_printf(FIL *fp, const TCHAR *str, ...)
{
	if (!fp || !fp->fp)
		return -1;

	va_list ap;
	va_start(ap, str);
	int res = vfprintf(fp->fp, str, ap);
	va_end(ap);
	return res;
}

TCHAR *f_gets(TCHAR *buff, int len, FIL *fp)
{
	if (!fp || !fp->fp)
		return NULL;

	return fgets(buff, len, fp->fp);
}

bool f_eof(FIL *fp)
{
	if (!fp || !fp->fp)
		return true;

	return feof(fp->fp);
}

BYTE f_error(FIL *fp)
{
	if (!fp || !fp->fp)
		return FR_INVALID_OBJECT;

	return ferror(fp->fp) ? FR_INVALID_PARAMETER : FR_OK;
}

FSIZE_t f_tell(FIL *fp)
{
	if (!fp || !fp->fp)
		return FR_INVALID_OBJECT;

	return ftell(fp->fp);
}

FSIZE_t f_size(FIL *fp)
{
	if (!fp || !fp->fp)
		return FR_INVALID_OBJECT;

	FSIZE_t offset = ftell(fp->fp);

	fseek(fp->fp, 0, SEEK_END);
	FSIZE_t size = ftell(fp->fp);

	fseek(fp->fp, offset, SEEK_SET);

	return size;
}

FRESULT f_rmdir(const TCHAR *path)
{
	if (0 == rmdir(path))
		return FR_OK;

	return FR_NO_PATH;
}

const char *f_strerror(int err)
{
	static const char *errors[21] = {
		"Succeeded",
		"Hard I/O Error",
		"Assertion failed",
		"Drive not ready",
		"File not found",
		"Path not found",
		"Invalid path format",
		"Directory full",
		"Access denied",
		"Invalid FIL/DIR",
		"Write protected",
		"Invalid drive num",
		"No work area",
		"No FAT volume",
		"Aborted",
		"Timeout",
		"Operation rejected",
		"No LFN buffer",
		"Too many open files",
		"Invalid parameter",
		"Invalid file format",
	};

	err = abs(err);

	if (err >= 21)
		return "Unknown error";

	return errors[err];
}
