#define _GNU_SOURCE
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>

void sdk_save_write(int slot, const void *data, int len)
{
	char *dirname = NULL;
	char *filename = NULL;

	asprintf(&dirname, "saves");
	asprintf(&filename, "saves/slot%i.bin", slot);

	if (0 != mkdir(dirname, 0777)) {
		if (EEXIST != errno)
			error(1, errno, "failed to create %s", dirname);
	}

	FILE *fp = fopen(filename, "wb");

	if (NULL == fp)
		error(1, errno, "failed to open %s", filename);

	fwrite(data, 1, len, fp);
	fclose(fp);
}

void sdk_save_read(int slot, void *data, int len)
{
	char *filename = NULL;

	asprintf(&filename, "saves/slot%i.bin", slot);

	FILE *fp = fopen(filename, "rb");

	if (fp) {
		fread(data, 1, len, fp);
		fclose(fp);
	} else {
		memset(data, 0xff, len);
	}
}
