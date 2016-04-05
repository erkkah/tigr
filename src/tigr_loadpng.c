#include "tigr_internal.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct {
	const unsigned char *p, *end;
} PNG;

static unsigned get32(const unsigned char *v)
{
	return (v[0] << 24) | (v[1] << 16) | (v[2] << 8) | v[3];
}

static const unsigned char *find(PNG *png, const char *chunk, unsigned minlen)
{
	const unsigned char *start;
	while (png->p < png->end)
	{
		unsigned len = get32(png->p+0);
		start = png->p;
		png->p += len + 12;
		if (memcmp(start+4, chunk, 4) == 0 && len >= minlen && png->p <= png->end)
			return start+8;
	}

	return NULL;
}

static unsigned char paeth(unsigned char a, unsigned char b, unsigned char c)
{
	int p  = a + b - c;
	int pa = abs(p-a), pb = abs(p-b), pc = abs(p-c);
	return (pa <= pb && pa <= pc) ? a : (pb <= pc) ? b : c;
}

static int unfilter(int w, int h, int bpp, unsigned char *raw)
{
	int len = w * bpp, x, y;
	unsigned char *prev = raw;
	for (y=0;y<h;y++,prev=raw,raw+=len)
	{
#define LOOP(A, B) for (x=0;x<bpp;x++) raw[x] += A; for (;x<len;x++) raw[x] += B; break
		switch (*raw++)
		{
		case 0: break;
		case 1: LOOP(0,         raw[x-bpp]);
		case 2: LOOP(prev[x],   prev[x]);
		case 3: LOOP(prev[x]/2, (raw[x-bpp] + prev[x])/2);
		case 4: LOOP(prev[x],   paeth(raw[x-bpp], prev[x], prev[x-bpp]));
		default: return 0;
		}
#undef LOOP
	}
	return 1;
}

static void convert(int bpp, int w, int h, unsigned char *src, TPixel *dest)
{
	int x,y;
	for (y=0;y<h;y++)
	{
		src++; // skip filter byte
		for (x=0;x<w;x++,src+=bpp)
		{
			switch (bpp) {
			case 1: *dest++ = tigrRGB (src[0], src[0], src[0]); break;
			case 2: *dest++ = tigrRGBA(src[0], src[0], src[0], src[1]); break;
			case 3: *dest++ = tigrRGB (src[0], src[1], src[2]); break;
			case 4: *dest++ = tigrRGBA(src[0], src[1], src[2], src[3]); break;
			}
		}
	}
}

static void depalette(int w, int h, unsigned char *src, TPixel *dest, const unsigned char *plte)
{
	int x,y,c;
	for (y=0;y<h;y++)
	{
		src++; // skip filter byte
		for (x=0;x<w;x++,src++)
		{
			c = src[0];
			*dest++ = tigrRGBA(plte[c*3+0], plte[c*3+1], plte[c*3+2], c?255:0);
		}
	}
}

#define FAIL() { errno = EINVAL; goto err; }
#define CHECK(X) if (!(X)) FAIL()
static int outsize(Tigr *bmp, int bpp) { return (bmp->w+1)*bmp->h * bpp; }

static Tigr *tigrLoadPng(PNG *png)
{
	const unsigned char *ihdr, *idat, *plte, *first;
	int depth, ctype, bpp;
	int datalen = 0;
	unsigned char *data = NULL, *out;
	Tigr *bmp = NULL;

	CHECK(memcmp(png->p, "\211PNG\r\n\032\n", 8) == 0); // PNG signature
	png->p += 8;
	first = png->p;

	// Read IHDR
	ihdr = find(png, "IHDR", 13);
	CHECK(ihdr);
	depth = ihdr[8];
	ctype = ihdr[9];
	switch (ctype) {
		case 0: bpp = 1; break; // greyscale
		case 2: bpp = 3; break; // RGB
		case 3: bpp = 1; break; // paletted
		case 4: bpp = 2; break; // grey+alpha
		case 6: bpp = 4; break; // RGBA
		default: FAIL();
	}

	// Allocate bitmap (+1 width to save room for stupid PNG filter bytes)
	bmp = tigrBitmap(get32(ihdr+0) + 1, get32(ihdr+4));
	CHECK(bmp);
	bmp->w--;

	// We don't support non-8bpp, interlacing, or wacky filter types.
	CHECK(depth == 8 && ihdr[10] == 0 && ihdr[11] == 0 && ihdr[12] == 0);

	// Join IDAT chunks.
	for (idat=find(png, "IDAT", 0); idat; idat=find(png, "IDAT", 0))
	{
		unsigned len = get32(idat-8);
		data = (unsigned char *)realloc(data, datalen + len);
		if (!data)
			break;

		memcpy(data+datalen, idat, len);
		datalen += len;
	}

	// Find palette.
	png->p = first;
	plte = find(png, "PLTE", 0);

	CHECK(data && datalen >= 6);
	CHECK((data[0] & 0x0f) == 0x08	// compression method (RFC 1950)
	   && (data[0] & 0xf0) <= 0x70	// window size
	   && (data[1] & 0x20) == 0);	// preset dictionary present

	out = (unsigned char *)bmp->pix + outsize(bmp, 4) - outsize(bmp, bpp);
	CHECK(tigrInflate(out, outsize(bmp, bpp), data+2, datalen-6));
	CHECK(unfilter(bmp->w, bmp->h, bpp, out));

	if (ctype == 3) {
		CHECK(plte);
		depalette(bmp->w, bmp->h, out, bmp->pix, plte);
	} else {
		convert(bpp, bmp->w, bmp->h, out, bmp->pix);
	}
	
	free(data);
	return bmp;

err:
	if (data) free(data);
	if (bmp)  tigrFree(bmp);
	return NULL;
}

#undef CHECK
#undef FAIL

Tigr *tigrLoadImageMem(const void *data, int length)
{
	PNG png;
	png.p = (unsigned char *)data;
	png.end = (unsigned char *)data + length;
	return tigrLoadPng(&png);
}

Tigr *tigrLoadImage(const char *fileName)
{
	int len;
	void *data;
	Tigr *bmp;

	data = tigrReadFile(fileName, &len);
	if (!data)
		return NULL;

	bmp = tigrLoadImageMem(data, len);
	free(data);
	return bmp;
}

