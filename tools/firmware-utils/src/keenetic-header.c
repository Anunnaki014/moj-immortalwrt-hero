// SPDX-License-Identifier: GPL-2.0-only
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAGIC_V1 "KND1"

struct keenetic_header {
	char magic[4];
	uint32_t crc;
	uint32_t size;
	uint8_t reserved[4];
};

static uint32_t crc32_table[256];

static void init_crc32_table(void)
{
	uint32_t c;
	int n, k;

	for (n = 0; n < 256; n++) {
		c = (uint32_t)n;
		for (k = 0; k < 8; k++) {
			if (c & 1)
				c = 0xedb88320L ^ (c >> 1);
			else
				c = c >> 1;
		}
		crc32_table[n] = c;
	}
}

static uint32_t calculate_crc32(uint8_t *buf, size_t len)
{
	uint32_t crc = 0xffffffffL;
	while (len--)
		crc = crc32_table[(crc ^ *buf++) & 0xff] ^ (crc >> 8);
	return crc ^ 0xffffffffL;
}

int main(int argc, char *argv[])
{
	FILE *in, *out;
	struct stat st;
	struct keenetic_header hdr;
	uint8_t *buf;
	size_t len;

	if (argc < 4) {
		fprintf(stderr, "Usage: %s <kernel> <in_file> <out_file>\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (stat(argv[2], &st) < 0) {
		fprintf(stderr, "Cannot stat %s\n", argv[2]);
		return EXIT_FAILURE;
	}

	in = fopen(argv[2], "rb");
	if (!in) {
		fprintf(stderr, "Cannot open %s\n", argv[2]);
		return EXIT_FAILURE;
	}

	out = fopen(argv[3], "wb");
	if (!out) {
		fprintf(stderr, "Cannot open %s\n", argv[3]);
		fclose(in);
		return EXIT_FAILURE;
	}

	len = st.st_size;
	buf = malloc(len);
	if (!buf) {
		fprintf(stderr, "Out of memory\n");
		fclose(in);
		fclose(out);
		return EXIT_FAILURE;
	}

	if (fread(buf, 1, len, in) != len) {
		fprintf(stderr, "Cannot read %s\n", argv[2]);
		free(buf);
		fclose(in);
		fclose(out);
		return EXIT_FAILURE;
	}

	init_crc32_table();
	
	memset(&hdr, 0, sizeof(hdr));
	memcpy(hdr.magic, MAGIC_V1, 4);
	hdr.size = __builtin_bswap32(len);
	hdr.crc = __builtin_bswap32(calculate_crc32(buf, len));

	fwrite(&hdr, 1, sizeof(hdr), out);
	fwrite(buf, 1, len, out);

	free(buf);
	fclose(in);
	fclose(out);

	return EXIT_SUCCESS;
}
