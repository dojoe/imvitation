#include "k.i.t.t.h"

void convert_forever_font()
{
	struct tga_header {
		uint8_t blah[12];
		uint16_t w, h;
		uint16_t blub;
	} hdr;

	uint8_t *buf1, *buf2;
	FILE *f;

	f = fopen("font-raw.tga", "rb");
	fread(&hdr, sizeof(hdr), 1, f);
	buf1 = (uint8_t *)malloc(hdr.w * hdr.h);
	buf2 = (uint8_t *)malloc(hdr.w * hdr.h);
	memset(buf2, 0xff, hdr.w * hdr.h);
	fread(buf1, hdr.w * hdr.h, 1, f);
	fclose(f);

	/* Compress pixels */
	int lasty = 0;
	int totw = 0;
	for (int y = 0; y < hdr.h; y++) {
		int x;
		for (x = 0; x < hdr.w; x++)
			if (buf1[x + y * hdr.w] == 0)
				break;
		if (x == hdr.w) { /* empty row */
			int h = y - lasty;
			if (h > 1) {
				int lastx = 0;
				for (x = 0; x < hdr.w; x++) {
					int y2;
					for (y2 = 0; y2 < h; y2++)
						if (buf1[x + (lasty + y2) * hdr.w] == 0)
							break;
					if (y2 == h) { /* empty column */
						if (x - lastx > 1) {
							int pw = (x - lastx) / 4;
							for (int px = 0; px < pw; px++)
								for (int py = 0; py < 16; py++)
									buf2[16 * (totw + px) + py] = buf1[hdr.w * (lasty + py * 4 + 1) + lastx + px * 4 + 1];
							totw += 16;
						}
						lastx = x;
					}
				}
			}
			lasty = y;
		}
	}

	/* Merge equal chars */
	int nchars = 0;
	for (int nsrc = 0; nsrc < totw / 16; nsrc++) {
		int newchar = 1;
		for (int ndst = 0; ndst < nchars; ndst++) {
			int i;
			for (i = 0; i < 256; i++)
				if (buf2[nsrc * 256 + i] != buf1[ndst * 256 + i])
					break;
			if (i == 256) {
				newchar = 0;
				break;
			}
		}
		if (newchar) {
			/* New char found */
			memcpy(buf1 + nchars * 256, buf2 + nsrc * 256, 256);
			nchars++;
		}
	}
	totw = nchars * 16;

	/* Transpose result */
	for (int y = 0; y < totw; y++)
		for (int x = 0; x < 16; x++)
			buf2[totw * x + y] = buf1[16 * y + x];
	hdr.w = totw;
	hdr.h = 16;

	f = fopen("font-cooked.tga", "wb");
	fwrite(&hdr, sizeof(hdr), 1, f);
	fwrite(buf2, hdr.w * hdr.h, 1, f);
	fclose(f);
}

