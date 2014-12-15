#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>
#include "kurl.h"
#include "kson.h"

const char *ibs_url = "https://api.basespace.illumina.com";
const char *ibs_prefix = "v1pre3";
const int IBS_BUFSIZE = 0x10000;

#define kroundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))

char *ibs_read_secret(const char *fn)
{
	char *path, buf[33];
	FILE *fp;
	int l;
	if (fn == 0) {
		char *home;
		home = getenv("HOME");
		if (home == 0) return 0;
		l = strlen(home) + 12;
		path = (char*)malloc(strlen(home) + 12);
		strcat(strcpy(path, home), "/.ibssecret");
	} else path = (char*)fn;
	fp = fopen(path, "r");
	if (path != fn) free(path);
	if (fp == 0) return 0;
	l = fread(buf, 1, 32, fp);
	buf[l] = 0;
	fclose(fp);
	return l == 32? strdup(buf) : 0;
}

char *ibs_read_all(const char *url, int *len)
{
	int l, n = 0, m = 0;
	char *ret = 0, *buf;
	kurl_t *ku;

	ku = kurl_open(url, 0);
	if (ku == 0) return 0;
	buf = calloc(1, IBS_BUFSIZE);
	while ((l = kurl_read(ku, buf, IBS_BUFSIZE)) > 0) {
		if (n + l + 1 > m) {
			m = n + l + 1;
			kroundup32(m);
			ret = realloc(ret, m);
		}
		memcpy(ret + n, buf, l);
		n += l;
		ret[n] = 0;
	}
	kurl_close(ku);
	if (len) *len = n;
	return ret;
}

int64_t ibs_download(const char *secret, const char *id, int is_stdout)
{
	char url[1024], *rst, *fn = 0;
	uint8_t *buf = 0;
	kurl_t *ku = 0;
	FILE *fp = 0;
	int l;
	int64_t size = 0, start = 0, write_size = 0;
	kson_t *kson;

	// get file name and file size
	sprintf(url, "%s/%s/files/%s?access_token=%s", ibs_url, ibs_prefix, id, secret);
	rst = ibs_read_all(url, 0);
	if ((kson = kson_parse(rst)) != 0) {
		const kson_node_t *p;
		if ((p = kson_by_path(kson->root, 2, "Response", "Name")) != 0)
			fn = strdup(p->v.str);
		if ((p = kson_by_path(kson->root, 2, "Response", "Size")) != 0)
			size = atoll(p->v.str);
		kson_destroy(kson);
	}
	free(rst);
	if (fn == 0) {
		fprintf(stderr, "[E::%s] failed to find file '%s'.\n", __func__, id);
		return -1;
	}
	fprintf(stderr, "[M::%s] file name: %s; size: %lld\n", __func__, id, (long long)size);

	if (!is_stdout) {
		struct stat s;
		if (stat(fn, &s) == 0) start = s.st_size;
		if (start == size)
			fprintf(stderr, "[W::%s] file '%s' is complete. Downloading skipped.\n", __func__, fn);
	} else fp = stdout;
	sprintf(url, "%s/%s/files/%s/content?access_token=%s", ibs_url, ibs_prefix, id, secret);
	if ((ku = kurl_open(url, 0)) == 0) {
		fprintf(stderr, "[E::%s] failed to establish the connection.\n", __func__);
		goto ret_getfile;
	}
	if (start > 0 && kurl_seek(ku, start, SEEK_SET) < 0) {
		fprintf(stderr, "[E::%s] file '%s' present, but 'range' is not supported. Please manually delete the file first.\n", __func__, fn);
		goto ret_getfile;
	}
	if (fn && (fp = fopen(fn, "ab")) == 0) {
		fprintf(stderr, "[E::%s] failed to create file '%s' in the working directory.\n", __func__, fn);
		goto ret_getfile;
	} else fprintf(stderr, "[M::%s] writing/appending to file '%s'...\n", __func__, fn);

	buf = malloc(IBS_BUFSIZE);
	while ((l = kurl_read(ku, buf, IBS_BUFSIZE)) > 0)
		write_size += fwrite(buf, 1, l, fp);
	free(buf);
	fprintf(stderr, "[M::%s] %lld bytes transferred.\n", __func__, (long long)write_size);

ret_getfile:
	if (fp && fp != stdout) fclose(fp);
	if (ku) kurl_close(ku);
	if (fn) free(fn);
	return size;
}

int main(int argc, char *argv[])
{
	int i, c, is_stdout = 0;
	char *secret, *fn = 0;

	while ((c = getopt(argc, argv, "cs:")) == 0) {
		if (c == 's') fn = optarg;
		else if (c == 'c') is_stdout = 1;
	}
	if (optind == argc) {
		fprintf(stderr, "Usage: ibsget [-c] [-s secretFile] <id> [id2 [...]]\n");
		return 1;
	}
	if ((secret = ibs_read_secret(fn)) == 0) {
		fprintf(stderr, "[E::%s] failed to read the access_token from the provided file or from '$HOME/.ibssecret'.\n", __func__);
		return 0;
	}

	kurl_init();
	for (i = optind; i < argc; ++i)
		ibs_download(secret, argv[i], is_stdout);
	kurl_destroy();
	free(secret);
	return 0;
}
