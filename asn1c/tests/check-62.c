#undef	NDEBUG
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>	/* for chdir(2) */
#include <string.h>
#include <dirent.h>
#include <assert.h>
#include <errno.h>

#include <T.h>

enum expectation {
	EXP_OK,		/* Encoding/decoding must succeed */
	EXP_BROKEN,	/* Decoding must fail */
	EXP_RECLESS,	/* Reconstruction is allowed to yield less data */
};

static unsigned char buf[4096];
static int buf_offset;

static int
_buf_writer(const void *buffer, size_t size, void *app_key) {
	unsigned char *b, *bend;
	(void)app_key;
	assert(buf_offset + size < sizeof(buf));
	memcpy(buf + buf_offset, buffer, size);
	b = buf + buf_offset;
	bend = b + size;
	printf("=> [");
	for(; b < bend; b++)
		printf(" %02X", *b);
	printf("]:%ld\n", (long)size);
	buf_offset += size;
	return 0;
}

static int
save_object(T_t *st) {
	asn_enc_rval_t rval; /* Return value */

	buf_offset = 0;
	
	rval = der_encode(&asn_DEF_T, st, _buf_writer, 0);
	if (rval.encoded == -1) {
		fprintf(stderr,
			"Cannot encode %s: %s\n",
			rval.failed_type->name, strerror(errno));
		assert(rval.encoded != -1);
		return -1;	/* JIC */
	}

	fprintf(stderr, "SAVED OBJECT IN SIZE %d\n", buf_offset);

	return 0;
}

static T_t *
load_object(enum expectation expectation, char *fbuf, int size) {
	ber_dec_rval_t rval;
	T_t *st = 0;
	int csize;

	fprintf(stderr, "LOADING OBJECT OF SIZE %d\n", size);

	/* Perform multiple iterations with multiple chunks sizes */
	for(csize = 1; csize < 20; csize += 1) {
		int fbuf_offset = 0;
		int fbuf_left = size;
		int fbuf_chunk = csize;

		if(st) asn_DEF_T.free_struct(&asn_DEF_T, st, 0);
		st = 0;

		do {
			fprintf(stderr, "Decoding from %d with %d (left %d)\n",
				fbuf_offset, fbuf_chunk, fbuf_left);
			rval = ber_decode(0, &asn_DEF_T, (void **)&st,
				fbuf + fbuf_offset,
					fbuf_chunk < fbuf_left 
						? fbuf_chunk : fbuf_left);
			fbuf_offset += rval.consumed;
			fbuf_left -= rval.consumed;
			if(rval.code == RC_WMORE)
				fbuf_chunk += 1;	/* Give little more */
			else
				fbuf_chunk = csize;	/* Back off */
		} while(fbuf_left && rval.code == RC_WMORE);

		if(expectation != EXP_BROKEN) {
			assert(rval.code == RC_OK);
		} else {
			assert(rval.code != RC_OK);
			fprintf(stderr, "Failed, but this was expected\n");
			asn_DEF_T.free_struct(&asn_DEF_T, st, 0);
			st = 0;	/* ignore leak for now */
		}
	}

	if(st) asn_fprint(stderr, &asn_DEF_T, st);
	return st;
}


static void
process_data(enum expectation expectation, char *fbuf, int size) {
	T_t *st;
	int ret;

	st = load_object(expectation, fbuf, size);
	if(!st) return;

	ret = save_object(st);
	assert(buf_offset < sizeof(buf));
	assert(ret == 0);

	if(expectation == EXP_RECLESS) {
		assert(buf_offset > 0 && buf_offset < size);
		assert(memcmp(buf + 2, fbuf + 2, buf_offset - 2) == 0);
	} else {
		assert(buf_offset == size);
		assert(memcmp(buf, fbuf, buf_offset) == 0);
	}

	asn_DEF_T.free_struct(&asn_DEF_T, st, 0);
}

/*
 * Decode the .der files and try to regenerate them.
 */
static int
process(const char *fname) {
	char fbuf[4096];
	char *ext = strrchr(fname, '.');
	enum expectation expectation;
	int ret;
	int rd;
	FILE *fp;

	if(ext == 0 || strcmp(ext, ".ber"))
		return 0;

	switch(ext[-1]) {
	case 'B':	/* The file is intentionally broken */
		expectation = EXP_BROKEN; break;
	case 'L':	/* Extensions are present */
		expectation = EXP_RECLESS; break;
	default:
		expectation = EXP_OK; break;
	}

	fprintf(stderr, "\nProcessing file [../%s]\n", fname);

	ret = chdir("../data-62");
	assert(ret == 0);
	fp = fopen(fname, "r");
	ret = chdir("../test-check-62");
	assert(ret == 0);
	assert(fp);

	rd = fread(fbuf, 1, sizeof(fbuf), fp);
	fclose(fp);

	assert(rd < sizeof(fbuf));	/* expect small files */

	process_data(expectation, fbuf, rd);

	return 1;
}

int
main() {
	DIR *dir;
	struct dirent *dent;
	int processed_files = 0;

	dir = opendir("../data-62");
	assert(dir);

	while((dent = readdir(dir))) {
		if(strncmp(dent->d_name, "data-62-", 8))
			continue;

		if(process(dent->d_name))
			processed_files++;
	}

	assert(processed_files);
	closedir(dir);

	return 0;
}
