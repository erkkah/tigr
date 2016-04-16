//////// Start of inlined file: tigr_amalgamated.c ////////


#include "tigr.h"

//////// Start of inlined file: tigr_bitmaps.c ////////

//////// Start of inlined file: tigr_internal.h ////////

// can't use pragma once here because this file probably will endup in .c
#ifndef __TIGR_INTERNAL_H__
#define __TIGR_INTERNAL_H__

#define _CRT_SECURE_NO_WARNINGS NOPE

// Creates a new bitmap, with extra payload bytes.
Tigr *tigrBitmap2(int w, int h, int extra);

// Resizes an existing bitmap.
void tigrResize(Tigr *bmp, int w, int h);

// Calculates the biggest scale that a bitmap can fit into an area at.
int tigrCalcScale(int bmpW, int bmpH, int areaW, int areaH);

// Calculates a new scale, taking minimum-scale flags into account.
int tigrEnforceScale(int scale, int flags);

// Calculates the correct position for a bitmap to fit into a window.
void tigrPosition(Tigr *bmp, int scale, int windowW, int windowH, int out[4]);

// ----------------------------------------------------------
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef TIGR_GAPI_D3D9
#include <d3d9.h>
typedef struct {
	int lost;
	IDirect3DDevice9 *dev;
	D3DPRESENT_PARAMETERS params;
	IDirect3DTexture9 *sysTex[2], *vidTex[2];
	IDirect3DVertexShader9 *vs;
	IDirect3DPixelShader9 *ps;
} D3D9Stuff;
#endif

#ifdef TIGR_GAPI_GL
#ifdef _WIN32
#include <GL/gl.h>
#else
#include <OpenGL/gl3.h>
#endif
typedef struct {
	#ifdef _WIN32
	HGLRC hglrc;
	HDC dc;
	#endif
	GLuint tex[2];
	GLuint vao;
	GLuint program;
	GLuint uniform_projection;
	GLuint uniform_model;
	GLuint uniform_parameters;
	int gl_legacy;
} GLStuff;
#endif

typedef struct {
	int shown, closed;
	#ifdef TIGR_GAPI_D3D9
	D3D9Stuff d3d9;
	#endif
	#ifdef TIGR_GAPI_GL
	GLStuff gl;
	#endif

	#ifdef _WIN32
	wchar_t *wtitle;
	DWORD dwStyle;
	RECT oldPos;
	#endif
	#ifdef __APPLE__
	void *glContext;
	#endif

	Tigr *widgets;
	int widgetsWanted;
	unsigned char widgetAlpha;
	float widgetsScale;

	int hblur, vblur;
	float scanlines, contrast;

	int flags;
	int scale;
	int pos[4];
	int lastChar;
	char keys[256], prev[256];
	#ifdef __APPLE__
	int mouseButtons;
	#endif
} TigrInternal;
// ----------------------------------------------------------

TigrInternal *tigrInternal(Tigr *bmp);

void tigrGAPICreate(Tigr *bmp);
void tigrGAPIDestroy(Tigr *bmp);
void tigrGAPIBegin(Tigr *bmp);
void tigrGAPIEnd(Tigr *bmp);
void tigrGAPIResize(Tigr *bmp, int width, int height);
void tigrGAPIPresent(Tigr *bmp, int w, int h);

#endif


//////// End of inlined file: tigr_internal.h ////////

#include <stdlib.h>
#include <string.h>

// Expands 0-255 into 0-256
#define EXPAND(X) ((X) + ((X) > 0))

#define CLIP0(X, X2, W) if (X < 0) { W += X; X2 -= X; X = 0; }
#define CLIP1(X, DW, W) if (X + W > DW) W = DW - X;
#define CLIP() \
	CLIP0(dx, sx, w);		\
	CLIP0(dy, sy, h);		\
	CLIP0(sx, dx, w);		\
	CLIP0(sy, dy, h);		\
	CLIP1(dx, dst->w, w);	\
	CLIP1(dy, dst->h, h);	\
	CLIP1(sx, src->w, w);	\
	CLIP1(sy, src->h, h);	\
	if (w <= 0 || h <= 0)	\
		return


Tigr *tigrBitmap2(int w, int h, int extra)
{
	Tigr *tigr = (Tigr *)calloc(1, sizeof(Tigr) + extra);
	tigr->w = w;
	tigr->h = h;
	tigr->pix = (TPixel *)calloc(w*h, sizeof(TPixel));
	return tigr;
}

Tigr *tigrBitmap(int w, int h)
{
	return tigrBitmap2(w, h, 0);
}

void tigrResize(Tigr *bmp, int w, int h)
{
	int y, cw, ch;
	TPixel *newpix = (TPixel *)calloc(w*h, sizeof(TPixel));
	cw = (w < bmp->w) ? w : bmp->w;
	ch = (h < bmp->h) ? h : bmp->h;

	// Copy any old data across.
	for (y=0;y<ch;y++)
		memcpy(newpix+y*w, bmp->pix+y*bmp->w, cw*sizeof(TPixel));

	free(bmp->pix);
	bmp->pix = newpix;
	bmp->w = w;
	bmp->h = h;
}

int tigrCalcScale(int bmpW, int bmpH, int areaW, int areaH)
{
	// We want it as big as possible in the window, but still
	// maintaining the correct aspect ratio, and always
	// having an integer pixel size.
	int scale = 0;
	for(;;)
	{
		scale++;
		if (bmpW*scale > areaW || bmpH*scale > areaH)
		{
			scale--;
			break;
		}
	}
	return (scale > 1) ? scale : 1;
}

int tigrEnforceScale(int scale, int flags)
{
	if ((flags & TIGR_4X) && scale < 4) scale = 4;
	if ((flags & TIGR_3X) && scale < 3) scale = 3;
	if ((flags & TIGR_2X) && scale < 2) scale = 2;
	return scale;
}

void tigrPosition(Tigr *bmp, int scale, int windowW, int windowH, int out[4])
{
	// Center the image on screen at this scale.
	out[0] = (windowW - bmp->w*scale) / 2;
	out[1] = (windowH - bmp->h*scale) / 2;
	out[2] = out[0] + bmp->w*scale;
	out[3] = out[1] + bmp->h*scale;
}

void tigrClear(Tigr *bmp, TPixel color)
{
	int count = bmp->w * bmp->h;
	int n;
	for (n=0;n<count;n++)
		bmp->pix[n] = color;
}

void tigrFill(Tigr *bmp, int x, int y, int w, int h, TPixel color)
{
	TPixel *td;
	int dt, i;

	if (x < 0) { w += x; x = 0; }
	if (y < 0) { h += y; y = 0; }
	if (x + w > bmp->w) { w = bmp->w - x; }
	if (y + h > bmp->h) { h = bmp->h - y; }
	if (w <= 0 || h <= 0)
		return;

	td = &bmp->pix[y*bmp->w + x];
	dt = bmp->w;
	do {
		for (i=0;i<w;i++)
			td[i] = color;
		td += dt;
	} while(--h);
}

void tigrLine(Tigr *bmp, int x0, int y0, int x1, int y1, TPixel color)
{
	int sx, sy, dx, dy, err, e2;
	dx = abs(x1 - x0);
	dy = abs(y1 - y0);
	if (x0 < x1) sx = 1; else sx = -1;
	if (y0 < y1) sy = 1; else sy = -1;
	err = dx - dy;

	tigrPlot(bmp, x0, y0, color);
	while (x0 != x1 || y0 != y1)
	{
		tigrPlot(bmp, x0, y0, color);
		e2 = 2*err;
		if (e2 > -dy) { err -= dy; x0 += sx; }
		if (e2 <  dx) { err += dx; y0 += sy; }
	}
}

void tigrRect(Tigr *bmp, int x, int y, int w, int h, TPixel color)
{
	int x1, y1;
	if (w <= 0 || h <= 0)
		return;
	
	x1 = x + w-1;
	y1 = y + h-1;
	tigrLine(bmp, x, y, x1, y, color);
	tigrLine(bmp, x1, y, x1, y1, color);
	tigrLine(bmp, x1, y1, x, y1, color);
	tigrLine(bmp, x, y1, x, y, color);
}

TPixel tigrGet(Tigr *bmp, int x, int y)
{
	TPixel empty = { 0,0,0,0 };
	if (x >= 0 && y >= 0 && x < bmp->w && y < bmp->h)
		return bmp->pix[y*bmp->w+x];
	return empty;
}

void tigrPlot(Tigr *bmp, int x, int y, TPixel pix)
{
	int xa, i, a;
	if (x >= 0 && y >= 0 && x < bmp->w && y < bmp->h)
	{
		xa = EXPAND(pix.a);
		i = y*bmp->w+x;

		a = xa * EXPAND(pix.a);
		bmp->pix[i].r += (unsigned char)((pix.r - bmp->pix[i].r)*a >> 16);
		bmp->pix[i].g += (unsigned char)((pix.g - bmp->pix[i].g)*a >> 16);
		bmp->pix[i].b += (unsigned char)((pix.b - bmp->pix[i].b)*a >> 16);
		bmp->pix[i].a += (unsigned char)((pix.a - bmp->pix[i].a)*a >> 16);
	}
}

void tigrBlit(Tigr *dst, Tigr *src, int dx, int dy, int sx, int sy, int w, int h)
{
	TPixel *td, *ts;
	int st, dt;
	CLIP();

	ts = &src->pix[sy*src->w + sx];
	td = &dst->pix[dy*dst->w + dx];
	st = src->w;
	dt = dst->w;
	do {
		memcpy(td, ts, w*sizeof(TPixel));
		ts += st;
		td += dt;
	} while(--h);
}

void tigrBlitTint(Tigr *dst, Tigr *src, int dx, int dy, int sx, int sy, int w, int h, TPixel tint)
{
	TPixel *td, *ts;
	int x, st, dt, xr,xg,xb,xa;
	CLIP();

	xr = EXPAND(tint.r);
	xg = EXPAND(tint.g);
	xb = EXPAND(tint.b);
	xa = EXPAND(tint.a);

	ts = &src->pix[sy*src->w + sx];
	td = &dst->pix[dy*dst->w + dx];
	st = src->w;
	dt = dst->w;
	do {
		for (x=0;x<w;x++)
		{
			unsigned r = (xr * ts[x].r) >> 8;
			unsigned g = (xg * ts[x].g) >> 8;
			unsigned b = (xb * ts[x].b) >> 8;
			unsigned a = xa * EXPAND(ts[x].a);
			td[x].r += (unsigned char)((r - td[x].r)*a >> 16);
			td[x].g += (unsigned char)((g - td[x].g)*a >> 16);
			td[x].b += (unsigned char)((b - td[x].b)*a >> 16);
			td[x].a += (unsigned char)((ts[x].a - td[x].a)*a >> 16);
		}
		ts += st;
		td += dt;
	} while(--h);
}

void tigrBlitAlpha(Tigr *dst, Tigr *src, int dx, int dy, int sx, int sy, int w, int h, float alpha)
{
	alpha = (alpha < 0) ? 0 : (alpha > 1 ? 1 : alpha);
	tigrBlitTint(dst, src, dx, dy, sx, sy, w, h, tigrRGBA(0xff,0xff,0xff,(unsigned char)(alpha*255)));
}

#undef CLIP0
#undef CLIP1
#undef CLIP

//////// End of inlined file: tigr_bitmaps.c ////////

//////// Start of inlined file: tigr_loadpng.c ////////

//#include "tigr_internal.h"
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


//////// End of inlined file: tigr_loadpng.c ////////

//////// Start of inlined file: tigr_savepng.c ////////

//#include "tigr_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct {
	unsigned crc, adler, bits, prev, runlen;
	FILE *out;
	unsigned crcTable[256];
} Save;

static const unsigned crctable[16] = { 0, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
	0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c };

static void put(Save *s, unsigned v)
{
	fputc(v, s->out);
	s->crc = (s->crc >> 4) ^ crctable[(s->crc & 15) ^ (v & 15)];
	s->crc = (s->crc >> 4) ^ crctable[(s->crc & 15) ^ (v >> 4)];
}

static void updateAdler(Save *s, unsigned v)
{
	unsigned s1 = s->adler & 0xffff, s2 = (s->adler >> 16) & 0xffff;
	s1 = (s1 +  v) % 65521;
	s2 = (s2 + s1) % 65521;
	s->adler = (s2 << 16) + s1;
}

static void put32(Save *s, unsigned v)
{
	put(s, (v >> 24) & 0xff);
	put(s, (v >> 16) & 0xff);
	put(s, (v >>  8) & 0xff);
	put(s, v & 0xff);
}

void putbits(Save *s, unsigned data, unsigned bitcount)
{
	while (bitcount--)
	{
		unsigned prev = s->bits;
		s->bits = (s->bits >> 1) | ((data & 1) << 7);
		data >>= 1;
		if (prev & 1)
		{
			put(s, s->bits);
			s->bits = 0x80;
		}
	}
}

void putbitsr(Save *s, unsigned data, unsigned bitcount)
{
	while (bitcount--)
		putbits(s, data >> bitcount, 1);
}

static void begin(Save *s, const char *id, unsigned len)
{
	put32(s, len);
	s->crc = 0xffffffff;
	put(s, id[0]); put(s, id[1]); put(s, id[2]); put(s, id[3]);
}

static void literal(Save *s, unsigned v)
{
	// Encode a literal/length using the built-in tables.
	// Could do better with a custom table but whatever.
	     if (v < 144)   putbitsr(s, 0x030+v-  0, 8);
	else if (v < 256)   putbitsr(s, 0x190+v-144, 9);
	else if (v < 280)   putbitsr(s, 0x000+v-256, 7);
	else                putbitsr(s, 0x0c0+v-280, 8);
}

static void encodelen(Save *s, unsigned code, unsigned bits, unsigned len)
{
	literal(s, code + (len >> bits));
	putbits(s, len, bits);
	putbits(s, 0, 5);
}

static void endrun(Save *s)
{
	s->runlen--;
	literal(s, s->prev);

	     if (s->runlen >= 67) encodelen(s, 277, 4, s->runlen - 67);
	else if (s->runlen >= 35) encodelen(s, 273, 3, s->runlen - 35);
	else if (s->runlen >= 19) encodelen(s, 269, 2, s->runlen - 19);
	else if (s->runlen >= 11) encodelen(s, 265, 1, s->runlen - 11);
	else if (s->runlen >=  3) encodelen(s, 257, 0, s->runlen -  3);
	else while (s->runlen--) literal(s, s->prev);
}

static void encodeByte(Save *s, unsigned char v)
{
	updateAdler(s, v);

	// Simple RLE compression. We could do better by doing a search
	// to find matches, but this works pretty well TBH.
	if (s->prev == v && s->runlen < 115)
	{
		s->runlen++;
	} else {
		if (s->runlen)
			endrun(s);

		s->prev = v;
		s->runlen = 1;
	}
}

static void savePngHeader(Save *s, Tigr *bmp)
{
	fwrite("\211PNG\r\n\032\n", 8, 1, s->out);
	begin(s, "IHDR", 13);
	put32(s, bmp->w);
	put32(s, bmp->h);
	put(s, 8); // bit depth
	put(s, 6); // RGBA
	put(s, 0); // compression (deflate)
	put(s, 0); // filter (standard)
	put(s, 0); // interlace off
	put32(s, ~s->crc);
}

static long savePngData(Save *s, Tigr *bmp, long dataPos)
{
	int x, y;
	long dataSize;
	begin(s, "IDAT", 0);
	put(s, 0x08); // zlib compression method
	put(s, 0x1d); // zlib compression flags
	putbits(s, 3, 3); // zlib last block + fixed dictionary
	for (y=0;y<bmp->h;y++)
	{
		TPixel *row = &bmp->pix[y*bmp->w];
		TPixel prev = tigrRGBA(0, 0, 0, 0);

		encodeByte(s, 1); // sub filter
		for (x=0;x<bmp->w;x++)
		{
			encodeByte(s, row[x].r - prev.r);
			encodeByte(s, row[x].g - prev.g);
			encodeByte(s, row[x].b - prev.b);
			encodeByte(s, row[x].a - prev.a);
			prev = row[x];
		}
	}
	endrun(s);
	literal(s, 256); // terminator
	while (s->bits != 0x80)
		putbits(s, 0, 1);
	put32(s, s->adler);
	dataSize = (ftell(s->out) - dataPos) - 8;
	put32(s, ~s->crc);
	return dataSize;
}

int tigrSaveImage(const char *fileName, Tigr *bmp)
{
	Save s;
	long dataPos, dataSize, err;
	
	// TODO - unicode?
	FILE *out = fopen(fileName, "wb");
	if (!out)
		return 1;

	s.out = out;
	s.adler = 1;
	s.bits = 0x80;
	s.prev = 0xffff;
	s.runlen = 0;

	savePngHeader(&s, bmp);
	dataPos = ftell(s.out);
	dataSize = savePngData(&s, bmp, dataPos);

	// End chunk.
	begin(&s, "IEND", 0);
	put32(&s, ~s.crc);

	// Write back payload size.
	fseek(out, dataPos, SEEK_SET);
	put32(&s, dataSize);

	err = ferror(out);
	fclose(out);
	return !err;
}

//////// End of inlined file: tigr_savepng.c ////////

//////// Start of inlined file: tigr_utils.c ////////

//#include "tigr_internal.h"
#include <stdio.h>
#include <stdlib.h>

void *tigrReadFile(const char *fileName, int *length)
{
	// TODO - unicode?
	FILE *file;
	char *data;
	size_t len;

	if (length)
		*length = 0;

	file = fopen(fileName, "rb");
	if (!file)
		return NULL;

	fseek(file, 0, SEEK_END);
	len = ftell(file);
	fseek(file, 0, SEEK_SET);

	data = (char *)malloc(len+1);
	if (!data)
	{
		fclose(file);
		return NULL;
	}

	if (fread(data, 1, len, file) != len) {
		free(data);
		fclose(file);
		return NULL;
	}
	data[len] = '\0';
	fclose(file);

	if (length)
		*length = len;

	return data;
}

// Reads a single UTF8 codepoint.
const char *tigrDecodeUTF8(const char *text, int *cp)
{
	unsigned char c = *text++;
	int extra = 0, min = 0;
	*cp = 0;
		 if (c >= 0xf0) { *cp = c & 0x07; extra = 3; min = 0x10000; }
	else if (c >= 0xe0) { *cp = c & 0x0f; extra = 2; min = 0x800; }
	else if (c >= 0xc0) { *cp = c & 0x1f; extra = 1; min = 0x80; }
	else if (c >= 0x80) { *cp = 0xfffd; }
	else *cp = c;
	while (extra--) {
		c = *text++;
		if ((c & 0xc0) != 0x80) { *cp = 0xfffd; break; }
		(*cp) = ((*cp) << 6) | (c & 0x3f);
	}
	if (*cp < min) *cp = 0xfffd;
	return text;
}

char *tigrEncodeUTF8(char *text, int cp)
{
	if (cp < 0 || cp > 0x10ffff) cp = 0xfffd;

#define EMIT(X,Y,Z) *text++ = X | ((cp >> Y)&Z)
		 if (cp <     0x80) { EMIT(0x00,0,0x7f); }
	else if (cp <    0x800) { EMIT(0xc0,6,0x1f); EMIT(0x80, 0, 0x3f); }
	else if (cp <  0x10000) { EMIT(0xe0,12,0xf); EMIT(0x80, 6, 0x3f); EMIT(0x80, 0, 0x3f); }
	else                    { EMIT(0xf0,18,0x7); EMIT(0x80, 12, 0x3f); EMIT(0x80, 6, 0x3f); EMIT(0x80, 0, 0x3f); }
	return text;
#undef EMIT
}

void tigrSetPostFX(Tigr *bmp, int hblur, int vblur, float scanlines, float contrast)
{
	TigrInternal *win = tigrInternal(bmp);
	win->hblur = hblur;
	win->vblur = vblur;
	win->scanlines = scanlines;
	win->contrast = contrast;
}

//////// End of inlined file: tigr_utils.c ////////

//////// Start of inlined file: tigr_inflate.c ////////

//#include "tigr_internal.h"
#include <stdlib.h>
#include <setjmp.h>

typedef struct {
	unsigned bits, count;
	const unsigned char *in, *inend;
	unsigned char *out, *outend;
	jmp_buf jmp;
	unsigned litcodes[288], distcodes[32], lencodes[19];
	int tlit, tdist, tlen;
} State;

#define FAIL() longjmp(s->jmp, 1)
#define CHECK(X) if (!(X)) FAIL()

// Built-in DEFLATE standard tables.
static char order[] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
static char lenBits[29+2] = { 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,  0,0 };
static int lenBase[29+2] = { 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258,  0,0 };
static char distBits[30+2] = { 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,  0,0 };
static int distBase[30+2] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577 };

// Table to bit-reverse a byte.
static const unsigned char reverseTable[256] = {
#define R2(n)    n,     n + 128,     n + 64,     n + 192
#define R4(n) R2(n), R2(n +  32), R2(n + 16), R2(n +  48)
#define R6(n) R4(n), R4(n +   8), R4(n +  4), R4(n +  12)
	R6(0), R6(2), R6(1), R6(3)
};

static unsigned rev16(unsigned n) { return (reverseTable[n&0xff] << 8) | reverseTable[(n>>8)&0xff]; }

static int bits(State *s, int n)
{
	int v = s->bits & ((1 << n)-1);
	s->bits >>= n;
	s->count -= n;
	while (s->count < 16)
	{
		CHECK(s->in != s->inend);
		s->bits |= (*s->in++) << s->count;
		s->count += 8;
	}
	return v;
}

static unsigned char *emit(State *s, int len)
{
	s->out += len;
	CHECK(s->out <= s->outend);
	return s->out-len;
}

static void copy(State *s, const unsigned char *src, int len)
{
	unsigned char *dest = emit(s, len);
	while (len--) *dest++ = *src++;
}

static int build(State *s, unsigned *tree, unsigned char *lens, int symcount)
{
	int n, codes[16], first[16], counts[16]={0};

	// Frequency count.
	for (n=0;n<symcount;n++) counts[lens[n]]++;

	// Distribute codes.
	counts[0] = codes[0] = first[0] = 0;
	for (n=1;n<=15;n++) {
		codes[n] = (codes[n-1] + counts[n-1]) << 1;
		first[n] = first[n-1] + counts[n-1];
	}
	CHECK(first[15]+counts[15] <= symcount);

	// Insert keys into the tree for each symbol.
	for (n=0;n<symcount;n++)
	{
		int len = lens[n];
		if (len != 0) {
			int code = codes[len]++, slot = first[len]++;
			tree[slot] = (code << (32-len)) | (n << 4) | len;
		}
	}

	return first[15];
}

static int decode(State *s, unsigned tree[], int max)
{
	// Find the next prefix code.
	unsigned lo = 0, hi = max, key;
	unsigned search = (rev16(s->bits) << 16) | 0xffff;
	while (lo < hi) {
		unsigned guess = (lo + hi) / 2;
		if (search < tree[guess]) hi = guess;
		else lo = guess + 1;
	}

	// Pull out the key and check it.
	key = tree[lo-1];
	CHECK(((search^key) >> (32-(key&0xf))) == 0);

	bits(s, key & 0xf);
	return (key >> 4) & 0xfff;
}

static void run(State *s, int sym)
{
	int length = bits(s, lenBits[sym]) + lenBase[sym];
	int dsym = decode(s, s->distcodes, s->tdist);
	int offs = bits(s, distBits[dsym]) + distBase[dsym];
	copy(s, s->out - offs, length);
}

static void block(State *s)
{
	for (;;) {
		int sym = decode(s, s->litcodes, s->tlit);
		     if (sym < 256) *emit(s, 1) = (unsigned char)sym;
		else if (sym > 256) run(s, sym-257);
		else break;
	}
}

static void stored(State *s)
{
	// Uncompressed data block.
	int len; 
	bits(s, s->count & 7);
	len = bits(s, 16);
	CHECK(((len^s->bits)&0xffff) == 0xffff);
	CHECK(s->in + len <= s->inend);

	copy(s, s->in, len);
	s->in += len;
	bits(s, 16);
}

static void fixed(State *s)
{
	// Fixed set of Huffman codes.
	int n;
	unsigned char lens[288+32];
	for (n=  0;n<=143;n++) lens[n] = 8;
	for (n=144;n<=255;n++) lens[n] = 9;
	for (n=256;n<=279;n++) lens[n] = 7;
	for (n=280;n<=287;n++) lens[n] = 8;
	for (n=0;n<32;n++) lens[288+n] = 5;

	// Build lit/dist trees.
	s->tlit  = build(s, s->litcodes, lens, 288);
	s->tdist = build(s, s->distcodes, lens+288, 32);
}

static void dynamic(State *s)
{
	int n, i, nlit, ndist, nlen;
	unsigned char lenlens[19] = {0}, lens[288+32];
	nlit = 257 + bits(s, 5);
	ndist = 1 + bits(s, 5);
	nlen = 4 + bits(s, 4);
	for (n=0;n<nlen;n++)
		lenlens[order[n]] = (unsigned char)bits(s, 3);

	// Build the tree for decoding code lengths.
	s->tlen = build(s, s->lencodes, lenlens, 19);

	// Decode code lengths.
	for (n=0;n<nlit+ndist;)
	{
		int sym = decode(s, s->lencodes, s->tlen);
		switch (sym) {
		case 16: for (i =  3+bits(s,2); i; i--,n++) lens[n] = lens[n-1]; break;
		case 17: for (i =  3+bits(s,3); i; i--,n++) lens[n] = 0; break;
		case 18: for (i = 11+bits(s,7); i; i--,n++) lens[n] = 0; break;
		default: lens[n++] = (unsigned char)sym; break;
		}
	}

	// Build lit/dist trees.
	s->tlit  = build(s, s->litcodes, lens, nlit);
	s->tdist = build(s, s->distcodes, lens+nlit, ndist);
}

int tigrInflate(void *out, unsigned outlen, const void *in, unsigned inlen)
{
	int last;
	State *s = (State *)calloc(1, sizeof(State));

	// We assume we can buffer 2 extra bytes from off the end of 'in'.
	s->in  = (unsigned char *)in;  s->inend  = s->in  + inlen + 2;
	s->out = (unsigned char *)out; s->outend = s->out + outlen;
	s->bits = 0; s->count = 0; bits(s, 0);

	if (setjmp(s->jmp) == 1) {
		free(s);
		return 0;
	}

	do {
		last = bits(s, 1);
		switch (bits(s, 2)) {
		case 0: stored(s); break;
		case 1: fixed(s); block(s); break;
		case 2: dynamic(s); block(s); break;
		case 3: FAIL();
		}
	} while(!last);

	free(s);
	return 1;
}

#undef CHECK
#undef FAIL

//////// End of inlined file: tigr_inflate.c ////////

//////// Start of inlined file: tigr_print.c ////////

//#include "tigr_internal.h"
//////// Start of inlined file: tigr_font.h ////////

// Auto-generated by incbin.pl from font.png

const unsigned char tigr_font[] = {
	0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,
	0x00,0x00,0x00,0xfd,0x00,0x00,0x00,0x5c,0x08,0x03,0x00,0x00,0x00,0x92,0xab,0x43,
	0x85,0x00,0x00,0x03,0x00,0x50,0x4c,0x54,0x45,0x5f,0x53,0x87,0x00,0x00,0x00,0x54,
	0x54,0x54,0xff,0xff,0xfa,0x31,0x2a,0x42,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,
	0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,
	0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,
	0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,
	0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,
	0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,
	0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,
	0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,
	0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,
	0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,
	0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,
	0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,
	0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,
	0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,
	0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,
	0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,
	0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,
	0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,
	0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,
	0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,
	0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,
	0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,
	0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,
	0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,
	0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,
	0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,
	0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,
	0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,
	0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,
	0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,
	0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,
	0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,
	0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,
	0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,
	0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,
	0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,
	0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,
	0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,
	0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,
	0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,
	0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,
	0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,
	0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,
	0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,
	0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,
	0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,
	0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,
	0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x4a,0x44,0xb7,0x5a,0x00,0x00,0x0b,
	0xc5,0x49,0x44,0x41,0x54,0x78,0x9c,0xed,0x5c,0x8b,0x82,0xe4,0x26,0x0e,0x44,0xc0,
	0xff,0x7f,0xf3,0x8d,0x91,0x4a,0x2f,0xc0,0x8d,0x7b,0x7a,0x76,0x93,0x9b,0x74,0x32,
	0xc6,0xc6,0xbc,0x0a,0x09,0x50,0x21,0xbc,0xa5,0xff,0xe6,0x5f,0xe9,0x85,0x7f,0xbd,
	0x35,0xba,0xfe,0xae,0xa0,0x34,0xba,0xfe,0xff,0x7a,0xf5,0x75,0x2d,0xee,0xd1,0x52,
	0x16,0x49,0x8e,0x58,0x17,0xa4,0x5b,0x4e,0x2c,0x49,0x9b,0x14,0x28,0xcf,0x1a,0x7f,
	0xdd,0x5d,0xe5,0x8d,0x08,0x8d,0xcf,0xcf,0x2e,0xbd,0x2f,0x5d,0x5a,0x45,0xc5,0x15,
	0xfd,0x09,0xf4,0xd2,0x08,0xba,0xfe,0x1b,0x8f,0xe4,0x52,0xd6,0xce,0x89,0xa5,0x8b,
	0x10,0x2c,0xd0,0x7f,0x25,0x23,0xa0,0x20,0x2e,0x80,0x9a,0x8f,0xe7,0x36,0x8f,0x67,
	0xfa,0x2a,0x17,0xf1,0xd7,0x93,0x4f,0xe7,0xc2,0x8e,0xaa,0x2b,0xaa,0xa3,0x50,0xf4,
	0x3b,0xe8,0xe9,0xca,0x6b,0x32,0x60,0xd8,0x7d,0x5c,0xeb,0x48,0x31,0x6a,0x20,0x15,
	0xbb,0x4b,0xb4,0x43,0xdf,0x18,0x8d,0xca,0x76,0x14,0xf0,0x55,0x1a,0xa3,0xe3,0xb0,
	0x48,0xaf,0x73,0xef,0x8e,0x8a,0x0a,0x09,0x2e,0xc4,0xa7,0x50,0x14,0x90,0x9b,0xc5,
	0x0a,0x32,0x24,0x84,0x22,0xdf,0x41,0x5f,0xbe,0xf2,0x0e,0x99,0x5e,0x9d,0xc0,0xb2,
	0xbd,0x1a,0x41,0x15,0xfd,0xca,0x08,0x19,0x90,0xa0,0x1f,0xc0,0xaa,0xb6,0xa1,0x14,
	0x7f,0x3b,0xfa,0x44,0x7b,0x73,0x88,0x45,0x34,0x82,0xa5,0x86,0xbc,0x5f,0x35,0xa0,
	0xd7,0x91,0x9e,0x2c,0x1d,0xb5,0x66,0x21,0xf7,0xda,0xf8,0xbb,0xc0,0x97,0xa1,0x21,
	0xa3,0x30,0x16,0x02,0x67,0x79,0x0b,0xbd,0x48,0x0d,0x9a,0x4f,0x18,0xae,0x5c,0x4d,
	0xe3,0x10,0xe2,0x09,0xb2,0xbf,0xfa,0x8b,0xc5,0x65,0xcd,0x29,0x26,0x1d,0xd5,0x70,
	0x19,0x25,0x23,0xfe,0xab,0xc1,0xe4,0xc7,0x7b,0xd6,0x74,0xa4,0xd3,0x67,0x42,0x3c,
	0x37,0x02,0xca,0xde,0xfc,0x30,0xe0,0x2c,0x4f,0xd1,0x93,0x0d,0xa1,0xa4,0xf9,0x57,
	0xb9,0x2c,0x12,0x3c,0x7b,0xf4,0x97,0x62,0x70,0xb5,0x2b,0xcd,0x87,0xd2,0xd4,0x80,
	0xde,0xba,0xb1,0x78,0xd4,0x59,0xd3,0x91,0x4e,0x9f,0xed,0x7d,0xf7,0xca,0xce,0x9d,
	0x2d,0x1d,0xca,0x55,0x7c,0x43,0xf6,0x4e,0xf3,0x1b,0x97,0xdb,0x08,0x22,0x11,0xa1,
	0x1a,0xfa,0x52,0x04,0x09,0x1a,0xda,0x5d,0x81,0xb9,0xf5,0x3a,0xeb,0xd5,0xb2,0x46,
	0x95,0xc3,0x90,0x6e,0x1a,0xf7,0x50,0x76,0x9e,0xf3,0xa5,0x19,0x23,0xfc,0x8c,0xe6,
	0x8f,0xfe,0xf4,0x9d,0x21,0xb2,0x87,0x86,0x56,0x27,0xe9,0xb2,0x42,0x2f,0x89,0x6d,
	0x6e,0xf0,0x9a,0x2e,0xf1,0xaa,0x03,0x7e,0x7c,0xcb,0xe4,0xe2,0xf3,0x6b,0xc8,0x73,
	0xd3,0x95,0xcf,0xcd,0xf9,0xb5,0x94,0xf7,0x65,0xef,0x34,0x9f,0x27,0x3f,0x95,0xa7,
	0x3e,0x5e,0x1a,0x86,0x01,0x2d,0x33,0x4f,0x5e,0xef,0x9d,0x30,0x46,0x8a,0x2a,0x89,
	0x47,0x52,0x12,0x25,0x45,0x88,0xf8,0x2b,0x79,0x78,0xd6,0xf4,0xb5,0xf8,0xfc,0x2e,
	0x7d,0xd7,0x8e,0x0f,0x0d,0x78,0x1b,0x7d,0x2e,0x68,0xf7,0x68,0x29,0xfb,0x9c,0xa8,
	0x47,0x6d,0xe8,0xdb,0x02,0xbe,0x1b,0x76,0xed,0xf8,0x8c,0xfe,0x3d,0xcd,0xff,0x0c,
	0xfa,0xa8,0x0d,0x3f,0x88,0x3e,0x97,0x9e,0xaa,0xfa,0x3b,0xe8,0x57,0xb7,0xff,0x64,
	0xf4,0xbf,0xf9,0xc7,0xf3,0x84,0x2e,0xc3,0xd1,0xd4,0x4a,0xcf,0x79,0xca,0xb6,0xf7,
	0x58,0x88,0x5c,0x7c,0x5a,0xc0,0x8a,0x2c,0x5c,0x79,0x3d,0xbf,0xad,0x30,0xb3,0x9d,
	0xc5,0xca,0x57,0xe6,0x1b,0xdf,0x94,0x66,0x0b,0x8f,0x9a,0x44,0xa6,0x2e,0x9d,0xd7,
	0x33,0x82,0x35,0xb3,0x23,0x15,0x1a,0x82,0x84,0x78,0x32,0x82,0x2a,0x49,0x2d,0x63,
	0xd2,0x05,0x2b,0xa2,0x37,0x7b,0xff,0x65,0x45,0x3e,0x44,0xc1,0xe7,0xe8,0x9d,0xd5,
	0x58,0x6d,0xd9,0xa5,0x2e,0x76,0x9b,0xa1,0x1f,0x76,0x18,0xac,0x99,0xa9,0xf0,0x5c,
	0xa9,0xca,0xb0,0x94,0xe9,0xbd,0x19,0xb2,0x05,0x3a,0x11,0x96,0xed,0x96,0x95,0x67,
	0x55,0x61,0x92,0xa0,0xd8,0xcc,0x0d,0x56,0xd2,0x1a,0x3d,0xf9,0xaa,0x1a,0xcc,0x7d,
	0x90,0x31,0x52,0x03,0xf3,0x02,0xeb,0xd6,0xe4,0xce,0xfc,0x15,0xb6,0x9c,0x6b,0x5c,
	0x68,0xb5,0xa9,0x13,0x0c,0x6b,0x0a,0xbd,0xa0,0x96,0x76,0x42,0xdf,0x60,0xa6,0xa3,
	0x1c,0x95,0xf9,0x0b,0xf4,0x66,0xdd,0x28,0x2f,0xb8,0x43,0x5f,0x48,0xaa,0x32,0xfb,
	0xc9,0x20,0xf4,0x62,0xe8,0xaf,0xd5,0xd0,0xd6,0x64,0x41,0x5f,0xcc,0x92,0x8d,0x61,
	0x42,0x4f,0x30,0x5c,0x11,0x36,0x9f,0x7e,0x2d,0xfb,0x80,0x1e,0x5c,0x2e,0x0e,0x19,
	0xdf,0xdd,0xa1,0x40,0x92,0xf1,0xbf,0x47,0x6f,0xf4,0x49,0xe8,0x67,0x13,0xf3,0xd4,
	0x3a,0x5e,0xe8,0x80,0xda,0xd4,0x25,0x6a,0xfe,0xd0,0x9b,0xdd,0xb0,0x4a,0x95,0x26,
	0xcd,0x0f,0x20,0x16,0xe8,0xa7,0x5e,0xdc,0x69,0x3a,0xa1,0xad,0xb1,0x40,0x0a,0x4a,
	0xb5,0xd4,0x91,0xd6,0x74,0x83,0xc0,0xf7,0x93,0x31,0x0c,0x9e,0xea,0x36,0xe8,0xa9,
	0xb1,0x01,0xbb,0x56,0xac,0x5d,0x6f,0xe4,0x5e,0x49,0x8d,0x54,0x99,0x9f,0xa2,0xd7,
	0xa1,0x91,0x7b,0x65,0x85,0xde,0xf1,0x06,0xcd,0x50,0x73,0x4a,0xe5,0x47,0x5d,0xc8,
	0xd0,0x12,0x7d,0x21,0x5b,0xd1,0xe6,0x61,0x67,0x1b,0x17,0x9e,0xa4,0x44,0xf2,0x11,
	0x45,0x81,0xfd,0x8e,0x79,0x25,0x9c,0xd0,0xef,0xd8,0x4c,0x7e,0x0f,0x4c,0xfa,0x4c,
	0x5a,0x41,0xa5,0x50,0xb2,0x93,0xbd,0xad,0x78,0x45,0xb6,0xd7,0x9a,0x23,0x60,0xe3,
	0xa9,0xb3,0x65,0x6c,0xa4,0x42,0x49,0x48,0x20,0x1b,0x99,0xa4,0x44,0xf2,0x11,0xf2,
	0x09,0x59,0xe1,0x4d,0x97,0x18,0x3f,0x62,0x71,0xe3,0x59,0x8d,0x2f,0x70,0xa0,0x4e,
	0xef,0x61,0xbc,0xe7,0xe7,0x50,0xa0,0xec,0x85,0xe1,0x8d,0xf2,0xa9,0x31,0xc7,0xb9,
	0xdb,0x52,0x74,0x4b,0xc0,0x2c,0xdd,0x9f,0x31,0x48,0x57,0xe1,0x7d,0x02,0x6b,0xe3,
	0xf3,0x02,0xd7,0x29,0x7a,0xbe,0x0d,0x9a,0x7f,0xd4,0xa8,0x0f,0x86,0x3f,0x57,0xe0,
	0x7f,0xe8,0xe7,0xf0,0x16,0xfd,0x6f,0xfe,0xf9,0xbd,0x28,0x5b,0xcb,0x5c,0x17,0xf9,
	0xd7,0x62,0x3e,0xe6,0xd4,0x98,0x57,0xd3,0xe2,0xf6,0xae,0xac,0xbb,0x99,0x0d,0xb7,
	0x92,0x9e,0xb2,0xf9,0x3d,0x3f,0xff,0xf6,0x53,0xe8,0x5b,0xf3,0x93,0x92,0x2e,0x54,
	0x32,0xaf,0x26,0x83,0xe9,0x8f,0xa3,0xc7,0x6a,0x09,0x1f,0xd3,0x11,0xfa,0xb1,0x6b,
	0xbb,0xa2,0x96,0x4a,0x55,0xe3,0x6b,0xc7,0x60,0xd5,0x4e,0x70,0xb2,0x6f,0xce,0xbe,
	0x56,0xb3,0xc7,0x17,0x9b,0x9e,0x5d,0x41,0x25,0x54,0xb3,0xb4,0x89,0xd6,0x2f,0xd4,
	0xe0,0x73,0x4e,0x46,0x26,0xb3,0xaf,0xd0,0x0f,0xb3,0x61,0xe1,0x30,0x8b,0x14,0x96,
	0xd0,0xa7,0x8b,0xe8,0x80,0xde,0x0c,0x1d,0x33,0x44,0x17,0x19,0xf6,0xa1,0xaf,0xe6,
	0x0c,0xbd,0x63,0xb3,0x70,0x49,0xb5,0xc6,0xfe,0xb7,0x2b,0xfa,0xc6,0xad,0x35,0xf4,
	0x46,0x5d,0x44,0xa1,0xa2,0xe5,0xb6,0xc5,0x86,0x6c,0x38,0xf4,0x66,0x5f,0x37,0xc7,
	0xe9,0x5e,0x9a,0xd0,0xe4,0x3c,0x67,0xdb,0x6a,0xe2,0xc6,0x37,0x5e,0x98,0xbf,0xcb,
	0xcb,0xbe,0x0e,0x32,0x7b,0x6d,0x2b,0xbf,0x42,0xdf,0x26,0xd2,0x95,0xd0,0xc3,0x5d,
	0x72,0x80,0x5e,0x93,0x8c,0x6c,0x35,0x17,0xbb,0xd5,0xe9,0x4b,0x49,0x85,0xaa,0x6c,
	0xab,0x81,0x8b,0x39,0x5a,0xe6,0xa5,0x81,0xd7,0x45,0xcd,0x2f,0xb2,0x2d,0x7f,0x80,
	0x7e,0xd1,0x9c,0x29,0xfa,0x04,0xbd,0x11,0x9c,0xbb,0x81,0xba,0x61,0x39,0xea,0xe8,
	0xde,0x70,0x9a,0x69,0x20,0x21,0x7b,0x2d,0x4b,0xf4,0xaf,0xc7,0x7d,0xd4,0x7c,0xcf,
	0x6f,0xcc,0x67,0x0a,0x76,0xd1,0x8c,0x6c,0x6c,0xd1,0xeb,0x0c,0x31,0xa3,0xcf,0xb4,
	0xc9,0x17,0x84,0x66,0x08,0x89,0xf1,0xb5,0x0a,0xa7,0x99,0x36,0x70,0x62,0xab,0x39,
	0x1d,0x56,0xbc,0xfa,0x00,0x7d,0xf1,0x4e,0x16,0xe5,0x37,0x1a,0xed,0x7c,0x29,0x20,
	0x1b,0xcd,0x91,0x11,0x59,0xf1,0x7c,0x54,0xe4,0x1e,0x99,0xb5,0xb4,0x66,0xd5,0x20,
	0x17,0x9a,0x21,0x24,0x66,0xcd,0x71,0x6a,0x60,0x55,0x96,0x1d,0x2e,0x6c,0x78,0xb9,
	0xcc,0xab,0x70,0x84,0x7e,0xb3,0x82,0xae,0xa3,0x75,0x20,0x58,0xb4,0xa8,0xfc,0xc2,
	0xc2,0x5c,0x95,0xa3,0xe3,0xbc,0x64,0x6b,0xea,0x75,0xed,0xa9,0x18,0xcb,0x6e,0x06,
	0x6d,0x4f,0xd9,0x3e,0x8b,0xde,0x3b,0x1f,0x51,0x21,0x4d,0x51,0x3f,0x8f,0x3e,0xb6,
	0xe3,0x4f,0xa1,0xf7,0xce,0x47,0xad,0x70,0x8a,0xba,0x41,0xaf,0x9a,0x5e,0x56,0x8c,
	0xf6,0x18,0x7d,0x6c,0xc7,0x7b,0xe8,0x7f,0xf3,0x2f,0xf4,0x91,0x84,0x79,0x2c,0x15,
	0x8b,0xf7,0x61,0x93,0xcd,0x62,0x31,0x28,0xcd,0x4f,0x20,0x16,0x2c,0x17,0x63,0x2f,
	0x3a,0xec,0x80,0xb0,0x36,0x16,0x9b,0x31,0x72,0xa5,0xbd,0x60,0xe1,0x6d,0x84,0x54,
	0xbe,0x16,0xff,0xeb,0x2e,0x8f,0x6b,0xd8,0x01,0x7a,0x31,0xb2,0xad,0xe9,0xf9,0x4c,
	0xc2,0x12,0xbd,0x98,0x91,0x7c,0x92,0xa9,0xe8,0x49,0x87,0xa2,0x53,0x20,0xeb,0xb4,
	0xbd,0x78,0x8e,0xde,0xce,0x8c,0x55,0x1c,0x9a,0x71,0xb5,0x6c,0xd0,0x4b,0x0a,0x39,
	0x73,0xf0,0x12,0xfd,0x30,0xf2,0x29,0xa0,0x9f,0x1a,0x52,0x66,0x2f,0xde,0x38,0xc8,
	0x20,0x87,0xd9,0x44,0xde,0x14,0xd0,0x17,0x00,0x56,0x2d,0xf0,0x47,0x3e,0x20,0x4d,
	0x2c,0xe2,0x2b,0xf4,0x62,0xab,0xb2,0x1c,0xaf,0x1e,0x08,0xb5,0xb0,0xc9,0x67,0x54,
	0xeb,0xfa,0x4d,0x14,0xe9,0x15,0xfa,0x91,0x7a,0x78,0xbb,0xba,0xa2,0x5c,0x69,0x3e,
	0x9c,0x69,0xea,0xad,0xe3,0x9e,0xae,0xc6,0xe8,0x71,0xc2,0xa9,0x4b,0xa3,0xc6,0x41,
	0x47,0xe5,0x20,0xe3,0x7f,0x77,0xe4,0x83,0x57,0xef,0x8e,0x45,0xdc,0x99,0x2d,0x6e,
	0xdc,0x14,0x27,0xc7,0x02,0x82,0x45,0x1d,0xb6,0x20,0x57,0x23,0xe5,0x0f,0x21,0x3a,
	0x07,0xde,0x11,0x7a,0x12,0xcf,0x0c,0x00,0xa9,0x28,0x5e,0x68,0x3e,0x22,0x65,0xfe,
	0x56,0x86,0x42,0x15,0x9a,0x5f,0xa5,0xd0,0x26,0x64,0x43,0xbb,0x2c,0x68,0xaa,0xaa,
	0x96,0x9a,0x2d,0xa4,0x9b,0xc3,0xb3,0x08,0x44,0x02,0x66,0xe6,0xab,0x4a,0x5e,0x42,
	0xf8,0x7a,0x99,0x8e,0xf8,0x1d,0xc9,0xde,0xcf,0x7a,0x1b,0xcd,0xcf,0x5b,0xe5,0x8a,
	0x5e,0xa6,0x19,0x31,0xf0,0x71,0xa8,0x53,0x35,0x1f,0x1c,0xc4,0xcf,0xab,0xeb,0x55,
	0x70,0x9e,0xf5,0xe6,0x46,0xc8,0x64,0x61,0x4e,0xd7,0x06,0x87,0x32,0xd0,0x4b,0x75,
	0x62,0x1e,0x1f,0xc9,0x1e,0xd3,0x33,0x38,0x86,0x4c,0x46,0x18,0x9d,0x0d,0x3c,0xd2,
	0x8d,0x00,0xe9,0x2a,0xa3,0xd1,0x62,0xe0,0xd3,0x34,0xee,0xe5,0x28,0xd9,0x3d,0xfa,
	0x05,0xda,0xd5,0xe4,0x03,0x1e,0xa4,0xe6,0x3e,0x54,0x1c,0xe8,0xb5,0x3a,0x6e,0xed,
	0x2b,0xf4,0x45,0xcd,0xb4,0x33,0xf4,0x51,0xf3,0xc5,0xff,0x5b,0x1d,0x43,0x01,0xfd,
	0x90,0xb9,0x2e,0xcc,0x7a,0xdf,0x45,0x2f,0xb5,0x38,0x7f,0x2d,0x18,0x53,0x98,0x94,
	0xe9,0x1c,0xbd,0x9a,0x69,0xbd,0xe1,0xd0,0xac,0xda,0xed,0x95,0x27,0x25,0xef,0x26,
	0xb1,0xb0,0x60,0x42,0xd2,0xe3,0x6c,0xe3,0x2d,0xd8,0x91,0xac,0x78,0x76,0xf7,0x01,
	0xf4,0x52,0x8b,0xf3,0xec,0x04,0x62,0xa6,0x44,0xad,0x9f,0x9d,0xdd,0xf2,0xd5,0x77,
	0x73,0x2b,0x75,0x85,0x9c,0x9a,0x12,0xc3,0x9e,0x5f,0xcf,0xb1,0xdd,0xdf,0x7d,0x1f,
	0xfd,0xaa,0x96,0x45,0x79,0xfa,0xf8,0x00,0xbd,0xe5,0xdb,0x56,0xfc,0x1f,0xfa,0xff,
	0x27,0xf4,0xbf,0xf9,0x97,0x3b,0x5f,0xa7,0x75,0x5b,0xf4,0xc3,0x46,0x9e,0xbe,0xc3,
	0xd9,0x75,0x18,0x01,0xf2,0x31,0x01,0xe9,0x97,0x32,0x7a,0xd5,0x24,0x32,0x1b,0x7b,
	0x7b,0xa2,0xe9,0x26,0x60,0x24,0xfa,0x66,0x53,0xb8,0x38,0x21,0x2f,0xdd,0x33,0xab,
	0xa2,0x67,0xe0,0xa7,0xbb,0x03,0xf1,0xaf,0xd1,0xc3,0x06,0xe7,0x35,0x34,0xa2,0x17,
	0x5a,0xc0,0xe7,0x95,0xf9,0x61,0x18,0x01,0xf2,0x7d,0x40,0x3a,0x3d,0xec,0x93,0xc0,
	0x2c,0x71,0x1f,0xb2,0x10,0x3e,0x6c,0x88,0xe8,0x9b,0xfb,0x12,0xc0,0x8b,0xa0,0x83,
	0xf5,0x50,0x42,0x4a,0x24,0x5d,0xe2,0xee,0x1e,0xa1,0x6f,0x8e,0x44,0x34,0x66,0x68,
	0xf3,0xe1,0x0c,0x66,0x75,0x38,0xf6,0xc4,0xa7,0xdd,0xe4,0xb8,0x17,0xdb,0x20,0x4a,
	0xc1,0x70,0x65,0xa3,0xd7,0x23,0x13,0xf5,0x00,0x4d,0x36,0xeb,0xb8,0xde,0xa2,0x27,
	0xd3,0x38,0x81,0xdf,0x19,0x68,0x95,0x72,0xa6,0xbb,0xd7,0x4b,0x7e,0x44,0x0f,0xc3,
	0xa6,0x81,0x58,0xeb,0x0e,0x2d,0x7a,0xa1,0x13,0x3e,0xce,0x11,0x68,0x6c,0x28,0x8a,
	0xb9,0x57,0xe1,0xcb,0x80,0xdd,0x01,0x0e,0xec,0xd0,0x63,0x5f,0x56,0xde,0xb2,0x55,
	0xca,0x3b,0x9b,0x69,0xeb,0x5e,0x77,0x8b,0x4d,0xf3,0xa5,0x24,0x92,0x0e,0xd5,0x31,
	0x70,0x95,0x63,0x4c,0x1b,0x76,0xe6,0x03,0xf4,0xf0,0x51,0x69,0xd3,0x66,0x5f,0x0e,
	0x89,0x31,0x67,0x54,0x96,0x58,0xf7,0x58,0xf6,0xaa,0x91,0x32,0x36,0x44,0xd1,0x5b,
	0x13,0x4b,0x5a,0x50,0x88,0x55,0x21,0xde,0x2a,0xbf,0x01,0x1d,0xf6,0xaa,0x33,0x7a,
	0x54,0x51,0x6d,0x84,0xb1,0xb1,0x87,0xa3,0x86,0xda,0x61,0xda,0xdd,0x2f,0x0d,0x9e,
	0x49,0xf6,0x10,0x8f,0x17,0x85,0x3b,0x29,0xca,0x1d,0xaf,0x6c,0x58,0x4e,0xbb,0xc9,
	0xa1,0x18,0x68,0x24,0xcf,0x16,0xaa,0xf9,0x51,0xf6,0xb8,0xa2,0x9f,0xdc,0x06,0xb4,
	0xed,0xd2,0xca,0xf9,0x49,0xbf,0xe9,0x27,0x55,0x54,0x53,0x6e,0xee,0x65,0xd4,0xaf,
	0x77,0xba,0x13,0xf2,0x4d,0xcd,0x8f,0x5e,0x14,0x8c,0xfb,0x42,0x2a,0x2b,0xe2,0x29,
	0x4d,0x99,0x2b,0xe9,0x10,0xd7,0xb9,0x8f,0x1a,0xf6,0x28,0x12,0x7a,0x67,0x2e,0xeb,
	0x45,0xd1,0x33,0x77,0x8c,0x73,0xbe,0xa5,0x72,0x23,0x9b,0x8f,0xd9,0xee,0xee,0x9e,
	0xa1,0xa7,0x84,0x96,0xa0,0xc3,0x26,0x7b,0x1c,0x47,0x13,0x2d,0x0e,0x4f,0x45,0xbe,
	0xe7,0x0b,0x30,0x7d,0x12,0x8b,0x8e,0x34,0xd9,0xd0,0x83,0x71,0xb5,0x1b,0xf4,0xb2,
	0xd9,0x21,0x0a,0x64,0x4b,0x48,0xbe,0x6b,0xed,0x99,0xe6,0xcb,0xa6,0x98,0x9e,0xf7,
	0xaa,0x90,0x22,0xc1,0x33,0xab,0xc7,0xd1,0x20,0x41,0x8a,0x32,0x0c,0xe2,0x9d,0x92,
	0x58,0xb4,0xdb,0xbf,0x88,0xb2,0x37,0xf7,0x4d,0xd8,0xee,0x5a,0x98,0x89,0x50,0x20,
	0x1e,0x65,0xf3,0x5d,0x93,0x3d,0xb7,0x73,0xf4,0x71,0xa5,0xcc,0x61,0xde,0x7b,0x8f,
	0x78,0x77,0xe8,0x57,0x85,0xf5,0x9c,0xcf,0x34,0x3f,0xa5,0xed,0x76,0x0a,0x61,0x5d,
	0x8f,0x3b,0x76,0x1c,0xef,0x30,0x8c,0x1e,0xa0,0x9f,0xc5,0x34,0x35,0xe6,0x07,0xd1,
	0x7b,0x37,0x87,0x56,0xd8,0x5e,0xa1,0xdf,0xdc,0x1d,0x6c,0x6e,0xad,0x04,0xfb,0xf7,
	0xd0,0x2f,0x2b,0x24,0x0c,0xf2,0x87,0xe8,0xe1,0xea,0xb9,0x45,0xff,0x9b,0x7f,0x61,
	0x5c,0x61,0xd0,0xc7,0x1d,0x2c,0xf1,0x8a,0xf8,0x1d,0x2d,0xed,0xde,0x06,0x3a,0xe4,
	0xb2,0x76,0xb3,0x62,0x41,0x4c,0x24,0x4e,0xae,0x62,0xeb,0x36,0xcb,0x6c,0x49,0x8e,
	0x1a,0x32,0x89,0x7b,0x93,0xd1,0x32,0x28,0xcf,0xa2,0xd8,0xee,0x3e,0x56,0x69,0x29,
	0x03,0x8b,0x28,0xe7,0x81,0xdb,0x1c,0x1f,0x66,0xce,0xe8,0x61,0x0a,0x17,0xf9,0x00,
	0x40,0xbe,0x04,0xc6,0x2e,0x6f,0x85,0x1d,0x5b,0xba,0x7b,0xd3,0x65,0x59,0x74,0x99,
	0xb1,0x65,0x76,0xd8,0x90,0x09,0xfd,0x26,0xa3,0xb5,0x9c,0xdc,0x37,0xe4,0xc5,0x4e,
	0x69,0xb9,0x7e,0x38,0x0d,0x23,0xfa,0x62,0xa6,0xf1,0x3e,0x58,0x5d,0x8b,0x33,0xea,
	0xb1,0x9d,0x7a,0xd4,0x00,0x31,0xae,0x8b,0xf8,0x72,0xc2,0x49,0x96,0x9a,0xc2,0xe2,
	0x9f,0xa1,0x6d,0x52,0xc4,0x40,0xef,0xf2,0x1e,0x85,0xea,0x41,0x50,0xcb,0x18,0xeb,
	0xf4,0x3a,0x60,0x56,0x33,0x5f,0xb9,0xed,0xc2,0x11,0x9e,0x34,0x40,0x36,0x91,0xa1,
	0x3b,0xeb,0xa3,0x6d,0xab,0x10,0x39,0x78,0xec,0x8d,0x32,0x8e,0xf3,0xba,0x90,0x7d,
	0x94,0x14,0xbf,0xb6,0xa2,0x4d,0x50,0x36,0x57,0x32,0x11,0x59,0x57,0x1e,0x81,0x20,
	0x3d,0xa2,0x95,0xc9,0xd8,0x3a,0x0c,0xc7,0x8e,0x80,0x9e,0x20,0x7b,0xa7,0xa8,0xc7,
	0xa1,0x68,0x90,0x93,0x7d,0xdd,0x04,0x3b,0xf4,0x5e,0xf3,0x9f,0xa1,0xc7,0x6c,0xda,
	0x16,0x5f,0xc0,0x6c,0xba,0x0b,0xfd,0x25,0x56,0xf7,0xfb,0x9a,0x0f,0xd9,0xfb,0x7f,
	0x56,0x80,0x7b,0x61,0x1d,0x9c,0xa0,0x0f,0x8e,0xc7,0x97,0xa1,0x7c,0x53,0x27,0xba,
	0x73,0x80,0x5e,0x17,0x8d,0x26,0x56,0x77,0x40,0xff,0x54,0xf3,0x0b,0xf4,0x20,0x1e,
	0x41,0x7b,0x3c,0xeb,0x21,0x73,0x54,0xd0,0x97,0xa1,0xb8,0x73,0x09,0x92,0x7c,0x95,
	0xc1,0x7b,0x20,0x71,0x30,0x50,0xde,0xf8,0x73,0x67,0x0f,0x42,0x3d,0xca,0x66,0xb4,
	0xa8,0xee,0x83,0xd5,0x75,0xfa,0x1c,0x68,0xfa,0x37,0x06,0x76,0x21,0xaf,0x37,0xec,
	0xc6,0x09,0x9f,0xeb,0x6c,0x33,0xc0,0x43,0x03,0x4b,0x7a,0xe4,0xad,0x25,0xee,0x6a,
	0x3e,0x09,0x61,0x67,0x38,0x1b,0xe0,0x26,0xd8,0x5d,0xdf,0xa8,0x78,0xb5,0xde,0x3f,
	0x6c,0xb9,0xc5,0x7c,0x13,0xbd,0x67,0xaf,0xbf,0x0f,0xfd,0xa2,0x41,0xff,0x2a,0xf4,
	0xbf,0xf9,0x57,0xf6,0x1c,0xc1,0x91,0x8f,0xb6,0x63,0x39,0x1a,0x6e,0xca,0x68,0x04,
	0x5e,0xd3,0x99,0xd9,0xf8,0xe3,0x66,0x0d,0xdc,0x00,0xe1,0x6d,0x33,0xda,0x87,0x38,
	0x8e,0x3f,0xcc,0x36,0xc6,0xae,0xa2,0x5c,0x92,0x0b,0xb0,0x95,0x5b,0xf4,0xf7,0x3c,
	0x43,0x76,0xfb,0xed,0x94,0xdb,0x68,0x53,0x53,0xdb,0xa5,0x1e,0x34,0xc3,0x51,0x96,
	0x84,0xfe,0x19,0xc7,0xf1,0x87,0xd9,0xca,0x73,0x96,0x63,0xc6,0xde,0x79,0x1e,0x5e,
	0xa6,0xe5,0x94,0x9b,0xec,0xd8,0xc9,0xf1,0xa8,0xd1,0xb7,0x0f,0x8a,0xc2,0xf6,0xd3,
	0x78,0x7e,0x83,0xe3,0xe8,0x8b,0x2e,0x5f,0xa0,0xeb,0x91,0xaf,0x07,0xe1,0x53,0xfb,
	0x10,0x7f,0xe6,0xb4,0x44,0x5b,0xb4,0x4d,0x87,0x45,0xc1,0x56,0x7a,0x93,0xe3,0x44,
	0xf4,0xe4,0xed,0xb6,0xe3,0x10,0x56,0x9b,0x63,0xb8,0xb7,0xdc,0x20,0xa3,0x6f,0x4a,
	0xb7,0x1e,0xda,0xb9,0xb0,0xb3,0xda,0x29,0xc7,0xf1,0xe8,0x43,0x44,0xef,0xdf,0xb0,
	0xf3,0x23,0xc7,0xd3,0xe7,0x5d,0x38,0xa3,0x2f,0x8f,0x1a,0xaf,0x20,0xd4,0xcd,0x7c,
	0xc8,0x71,0x5e,0xa2,0x7f,0xc3,0xce,0x7f,0x9c,0xe7,0x73,0xe8,0x31,0x4d,0x1c,0x72,
	0x9c,0x7b,0xf4,0x6f,0x6b,0xfe,0x09,0x21,0x91,0x70,0x46,0x4f,0x79,0x0e,0x3a,0x2d,
	0x4a,0x66,0xfa,0x73,0x8e,0x83,0x76,0x2b,0xab,0x42,0x04,0xcf,0xf9,0xc6,0x58,0x1e,
	0x84,0xca,0x23,0xda,0x09,0x31,0x11,0x76,0x64,0x87,0xc2,0xfe,0x24,0xc7,0x21,0xfb,
	0xd2,0x2b,0x44,0xf4,0xfe,0x51,0x96,0x73,0x1b,0xe2,0xcf,0x5d,0xde,0xa9,0xf6,0xb9,
	0x9d,0x1b,0x34,0xdf,0x47,0x7c,0x00,0xfd,0xc2,0x47,0xf7,0x0f,0x43,0xef,0xf7,0x20,
	0x82,0x67,0xfa,0x03,0xe8,0x8f,0xc3,0xbf,0x86,0xde,0xff,0xeb,0x21,0xe1,0xdf,0x3a,
	0xec,0xbf,0xdd,0x97,0xf3,0x3f,0xca,0x1b,0xaa,0xdf,0xfc,0xa4,0x05,0x0a,0x00,0x00,
	0x00,0x00,0x49,0x45,0x4e,0x44,0xae,0x42,0x60,0x82 };

int tigr_font_size = (int)sizeof(tigr_font);


//////// End of inlined file: tigr_font.h ////////

#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>

#ifdef _MSC_VER
#define vsnprintf _vsnprintf
#endif

TigrFont tigrStockFont;
TigrFont *tfont = &tigrStockFont;

// Converts 8-bit codepage entries into Unicode code points.
static int cp1252[] = {
	0x20ac,0xfffd,0x201a,0x0192,0x201e,0x2026,0x2020,0x2021,0x02c6,0x2030,0x0160,0x2039,0x0152,0xfffd,0x017d,0xfffd,
	0xfffd,0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,0x02dc,0x2122,0x0161,0x203a,0x0153,0xfffd,0x017e,0x0178,
	0x00a0,0x00a1,0x00a2,0x00a3,0x00a4,0x00a5,0x00a6,0x00a7,0x00a8,0x00a9,0x00aa,0x00ab,0x00ac,0x00ad,0x00ae,0x00af,
	0x00b0,0x00b1,0x00b2,0x00b3,0x00b4,0x00b5,0x00b6,0x00b7,0x00b8,0x00b9,0x00ba,0x00bb,0x00bc,0x00bd,0x00be,0x00bf,
	0x00c0,0x00c1,0x00c2,0x00c3,0x00c4,0x00c5,0x00c6,0x00c7,0x00c8,0x00c9,0x00ca,0x00cb,0x00cc,0x00cd,0x00ce,0x00cf,
	0x00d0,0x00d1,0x00d2,0x00d3,0x00d4,0x00d5,0x00d6,0x00d7,0x00d8,0x00d9,0x00da,0x00db,0x00dc,0x00dd,0x00de,0x00df,
	0x00e0,0x00e1,0x00e2,0x00e3,0x00e4,0x00e5,0x00e6,0x00e7,0x00e8,0x00e9,0x00ea,0x00eb,0x00ec,0x00ed,0x00ee,0x00ef,
	0x00f0,0x00f1,0x00f2,0x00f3,0x00f4,0x00f5,0x00f6,0x00f7,0x00f8,0x00f9,0x00fa,0x00fb,0x00fc,0x00fd,0x00fe,0x00ff,
};
static int border(Tigr *bmp, int x, int y)
{
	TPixel top = tigrGet(bmp, 0, 0);
	TPixel c = tigrGet(bmp, x, y);
	return (c.r == top.r && c.g == top.g && c.b == top.b) || x >= bmp->w || y >= bmp->h;
}

static void scan(Tigr *bmp, int *x, int *y, int *rowh)
{
	while (*y < bmp->h)
	{
		if (*x >= bmp->w) {
			*x = 0;
			(*y) += *rowh;
			*rowh = 1;
		}
		if (!border(bmp, *x, *y))
			return;
		(*x)++;
	}
}

int tigrLoadGlyphs(TigrFont *font, int codepage)
{
	int i, x=0, y=0, w, h, rowh=1;
	TigrGlyph *g;
	switch (codepage) {
		case 0:    font->numGlyphs = 128-32; break;
		case 1252: font->numGlyphs = 256-32; break;
	}

	font->glyphs = (TigrGlyph *)calloc(font->numGlyphs, sizeof(TigrGlyph));
	for (i=32;i<font->numGlyphs+32;i++)
	{
		// Find the next glyph.
		scan(font->bitmap, &x, &y, &rowh);
		if (y >= font->bitmap->h)
		{
			errno = EINVAL;
			return 0;
		}

		// Scan the width and height
		w = h = 0;
		while (!border(font->bitmap, x+w, y)) w++;
		while (!border(font->bitmap, x, y+h)) h++;

		// Look up the Unicode code point.
		g = &font->glyphs[i-32];
		if (i < 128) g->code = i; // ASCII
		else if (codepage == 1252) g->code = cp1252[i-128];
		else { errno = EINVAL; return 0; }

		g->x = x; g->y = y; g->w = w; g->h = h;
		x += w;
		if (h != font->glyphs[0].h) { errno = EINVAL; return 0; }

		if (h > rowh)
			rowh = h;
	}

	// Sort by code point.
	for (i=1;i<font->numGlyphs;i++)
	{
		int j = i;
		TigrGlyph g = font->glyphs[i];
		while (j > 0 && font->glyphs[j-1].code > g.code) {
			font->glyphs[j] = font->glyphs[j-1];
			j--;
		}
		font->glyphs[j] = g;
	}

	return 1;
}

TigrFont *tigrLoadFont(Tigr *bitmap, int codepage)
{
	TigrFont *font = (TigrFont *)calloc(1, sizeof(TigrFont));
	font->bitmap = bitmap;
	if (!tigrLoadGlyphs(font, codepage))
	{
		tigrFreeFont(font);
		return NULL;
	}
	return font;
}

void tigrFreeFont(TigrFont *font)
{
	tigrFree(font->bitmap);
	free(font->glyphs);
	free(font);
}

static TigrGlyph *get(TigrFont *font, int code)
{
	unsigned lo = 0, hi = font->numGlyphs;
	while (lo < hi) {
		unsigned guess = (lo + hi) / 2;
		if (code < font->glyphs[guess].code) hi = guess;
		else lo = guess + 1;
	}

	if (lo == 0 || font->glyphs[lo-1].code != code)
		return &font->glyphs['?' - 32];
	else
		return &font->glyphs[lo-1];
}

void tigrSetupFont(TigrFont *font)
{
	// Load the stock font if needed.
	if (font == tfont && !tfont->bitmap)
	{
		tfont->bitmap = tigrLoadImageMem(tigr_font, tigr_font_size);
		tigrLoadGlyphs(tfont, 1252);
	}
}

void tigrPrint(Tigr *dest, TigrFont *font, int x, int y, TPixel color, const char *text, ...)
{
	char tmp[1024];
	TigrGlyph *g;
	va_list args;
	const char *p;
	int start = x, c;

	tigrSetupFont(font);

	// Expand the formatting string.
	va_start(args, text);
	vsnprintf(tmp, sizeof(tmp), text, args);
	tmp[sizeof(tmp)-1] = 0;
	va_end(args);

	// Print each glyph.
	p = tmp;
	while (*p)
	{
		p = tigrDecodeUTF8(p, &c);
		if (c == '\r')
			continue;
		if (c == '\n') {
			x = start;
			y += tigrTextHeight(font, "");
			continue;
		}
		g = get(font, c);
		tigrBlitTint(dest, font->bitmap, x, y, g->x, g->y, g->w, g->h, color);
		x += g->w;
	}
}

int tigrTextWidth(TigrFont *font, const char *text)
{
	int x = 0, w = 0, c;
	tigrSetupFont(font);

	while (*text)
	{
		text = tigrDecodeUTF8(text, &c);
		if (c == '\n' || c == '\r') {
			x = 0;
		} else {
			x += get(font, c)->w;
			w = (x > w) ? x : w;
		}
	}
	return w;
}

int tigrTextHeight(TigrFont *font, const char *text)
{
	int rowh, h, c;
	tigrSetupFont(font);

	h = rowh = get(font, 0)->h;
	while (*text)
	{
		text = tigrDecodeUTF8(text, &c);
		if (c == '\n' && *text)
			h += rowh; 
	}
	return h;
}

//////// End of inlined file: tigr_print.c ////////


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
//////// Start of inlined file: tigr_upscale_d3d9_vs.h ////////

#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler 10.0.10011.16384
//
// Parameters:
//
//   float4 screenSize;
//   float4 texSize;
//
//
// Registers:
//
//   Name         Reg   Size
//   ------------ ----- ----
//   screenSize   c0       1
//   texSize      c1       1
//

    vs_2_0
    def c2, -0.5, 2, -1, 0
    def c3, 1, -1, 0, 0
    dcl_position v0
    dcl_texcoord v1
    mul oT0.xy, v1, c1
    add r0.xyz, v0.xyxw, c2.x
    rcp r1.xw, c0.x
    rcp r1.y, c0.y
    mul r0.xyz, r0, r1.xyww
    mad r0.xyz, r0, c2.y, c2.z
    mad oPos.xyw, r0.xyzz, c3.xyzz, c3.zzzx
    mov oPos.z, v0.z

// approximately 8 instruction slots used
#endif

const BYTE tigrUpscaleVSCode[] =
{
      0,   2, 254, 255, 254, 255, 
     42,   0,  67,  84,  65,  66, 
     28,   0,   0,   0, 111,   0, 
      0,   0,   0,   2, 254, 255, 
      2,   0,   0,   0,  28,   0, 
      0,   0,   8, 131,   0,   0, 
    104,   0,   0,   0,  68,   0, 
      0,   0,   2,   0,   0,   0, 
      1,   0,   2,   0,  80,   0, 
      0,   0,   0,   0,   0,   0, 
     96,   0,   0,   0,   2,   0, 
      1,   0,   1,   0,   6,   0, 
     80,   0,   0,   0,   0,   0, 
      0,   0, 115,  99, 114, 101, 
    101, 110,  83, 105, 122, 101, 
      0, 171,   1,   0,   3,   0, 
      1,   0,   4,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
    116, 101, 120,  83, 105, 122, 
    101,   0, 118, 115,  95,  50, 
     95,  48,   0,  77, 105,  99, 
    114, 111, 115, 111, 102, 116, 
     32,  40,  82,  41,  32,  72, 
     76,  83,  76,  32,  83, 104, 
     97, 100, 101, 114,  32,  67, 
    111, 109, 112, 105, 108, 101, 
    114,  32,  49,  48,  46,  48, 
     46,  49,  48,  48,  49,  49, 
     46,  49,  54,  51,  56,  52, 
      0, 171,  81,   0,   0,   5, 
      2,   0,  15, 160,   0,   0, 
      0, 191,   0,   0,   0,  64, 
      0,   0, 128, 191,   0,   0, 
      0,   0,  81,   0,   0,   5, 
      3,   0,  15, 160,   0,   0, 
    128,  63,   0,   0, 128, 191, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  31,   0,   0,   2, 
      0,   0,   0, 128,   0,   0, 
     15, 144,  31,   0,   0,   2, 
      5,   0,   0, 128,   1,   0, 
     15, 144,   5,   0,   0,   3, 
      0,   0,   3, 224,   1,   0, 
    228, 144,   1,   0, 228, 160, 
      2,   0,   0,   3,   0,   0, 
      7, 128,   0,   0, 196, 144, 
      2,   0,   0, 160,   6,   0, 
      0,   2,   1,   0,   9, 128, 
      0,   0,   0, 160,   6,   0, 
      0,   2,   1,   0,   2, 128, 
      0,   0,  85, 160,   5,   0, 
      0,   3,   0,   0,   7, 128, 
      0,   0, 228, 128,   1,   0, 
    244, 128,   4,   0,   0,   4, 
      0,   0,   7, 128,   0,   0, 
    228, 128,   2,   0,  85, 160, 
      2,   0, 170, 160,   4,   0, 
      0,   4,   0,   0,  11, 192, 
      0,   0, 164, 128,   3,   0, 
    164, 160,   3,   0,  42, 160, 
      1,   0,   0,   2,   0,   0, 
      4, 192,   0,   0, 170, 144, 
    255, 255,   0,   0
};

//////// End of inlined file: tigr_upscale_d3d9_vs.h ////////

//////// Start of inlined file: tigr_upscale_d3d9_ps.h ////////

#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler 10.0.10011.16384
//
// Parameters:
//
//   sampler2D image;
//   float4 params;
//   float4 texSize;
//
//
// Registers:
//
//   Name         Reg   Size
//   ------------ ----- ----
//   texSize      c1       1
//   params       c2       1
//   image        s0       1
//

    ps_2_0
    def c0, 0.5, -0.5, 0, 0
    dcl t0.xy
    dcl_2d s0
    frc r0.w, t0.y
    add r0.x, -r0.w, c0.x
    mov r1.w, c0.x
    mad r0.x, c2.z, r0.x, r1.w
    add r0.x, r0.x, r0.x
    frc r0.yz, t0.zxyw
    add r0.yz, -r0, t0.zxyw
    add r0.yz, r0, c0.x
    lrp r1.xy, c2, t0, r0.yzxw
    mul r2.x, r1.x, c1.z
    mul r2.y, r1.y, c1.w
    texld r2, r2, s0
    mad r0.xyz, r2, r0.x, c0.y
    mad r2.xyz, c2.w, r0, r1.w
    mov oC0, r2

// approximately 15 instruction slots used (1 texture, 14 arithmetic)
#endif

const BYTE tigrUpscalePSCode[] =
{
      0,   2, 255, 255, 254, 255, 
     52,   0,  67,  84,  65,  66, 
     28,   0,   0,   0, 151,   0, 
      0,   0,   0,   2, 255, 255, 
      3,   0,   0,   0,  28,   0, 
      0,   0,   8, 131,   0,   0, 
    144,   0,   0,   0,  88,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,  96,   0, 
      0,   0,   0,   0,   0,   0, 
    112,   0,   0,   0,   2,   0, 
      2,   0,   1,   0,  10,   0, 
    120,   0,   0,   0,   0,   0, 
      0,   0, 136,   0,   0,   0, 
      2,   0,   1,   0,   1,   0, 
      6,   0, 120,   0,   0,   0, 
      0,   0,   0,   0, 105, 109, 
     97, 103, 101,   0, 171, 171, 
      4,   0,  12,   0,   1,   0, 
      1,   0,   1,   0,   0,   0, 
      0,   0,   0,   0, 112,  97, 
    114,  97, 109, 115,   0, 171, 
      1,   0,   3,   0,   1,   0, 
      4,   0,   1,   0,   0,   0, 
      0,   0,   0,   0, 116, 101, 
    120,  83, 105, 122, 101,   0, 
    112, 115,  95,  50,  95,  48, 
      0,  77, 105,  99, 114, 111, 
    115, 111, 102, 116,  32,  40, 
     82,  41,  32,  72,  76,  83, 
     76,  32,  83, 104,  97, 100, 
    101, 114,  32,  67, 111, 109, 
    112, 105, 108, 101, 114,  32, 
     49,  48,  46,  48,  46,  49, 
     48,  48,  49,  49,  46,  49, 
     54,  51,  56,  52,   0, 171, 
     81,   0,   0,   5,   0,   0, 
     15, 160,   0,   0,   0,  63, 
      0,   0,   0, 191,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     31,   0,   0,   2,   0,   0, 
      0, 128,   0,   0,   3, 176, 
     31,   0,   0,   2,   0,   0, 
      0, 144,   0,   8,  15, 160, 
     19,   0,   0,   2,   0,   0, 
      8, 128,   0,   0,  85, 176, 
      2,   0,   0,   3,   0,   0, 
      1, 128,   0,   0, 255, 129, 
      0,   0,   0, 160,   1,   0, 
      0,   2,   1,   0,   8, 128, 
      0,   0,   0, 160,   4,   0, 
      0,   4,   0,   0,   1, 128, 
      2,   0, 170, 160,   0,   0, 
      0, 128,   1,   0, 255, 128, 
      2,   0,   0,   3,   0,   0, 
      1, 128,   0,   0,   0, 128, 
      0,   0,   0, 128,  19,   0, 
      0,   2,   0,   0,   6, 128, 
      0,   0, 210, 176,   2,   0, 
      0,   3,   0,   0,   6, 128, 
      0,   0, 228, 129,   0,   0, 
    210, 176,   2,   0,   0,   3, 
      0,   0,   6, 128,   0,   0, 
    228, 128,   0,   0,   0, 160, 
     18,   0,   0,   4,   1,   0, 
      3, 128,   2,   0, 228, 160, 
      0,   0, 228, 176,   0,   0, 
    201, 128,   5,   0,   0,   3, 
      2,   0,   1, 128,   1,   0, 
      0, 128,   1,   0, 170, 160, 
      5,   0,   0,   3,   2,   0, 
      2, 128,   1,   0,  85, 128, 
      1,   0, 255, 160,  66,   0, 
      0,   3,   2,   0,  15, 128, 
      2,   0, 228, 128,   0,   8, 
    228, 160,   4,   0,   0,   4, 
      0,   0,   7, 128,   2,   0, 
    228, 128,   0,   0,   0, 128, 
      0,   0,  85, 160,   4,   0, 
      0,   4,   2,   0,   7, 128, 
      2,   0, 255, 160,   0,   0, 
    228, 128,   1,   0, 255, 128, 
      1,   0,   0,   2,   0,   8, 
     15, 128,   2,   0, 228, 128, 
    255, 255,   0,   0
};

//////// End of inlined file: tigr_upscale_d3d9_ps.h ////////

//////// Start of inlined file: tigr_d3d9.c ////////

//#include "tigr_internal.h"

#ifdef TIGR_GAPI_D3D9
IDirect3D9 *tigrD3D = NULL;
extern const unsigned char tigrUpscaleVSCode[], tigrUpscalePSCode[];

void tigrGAPICreate(Tigr *bmp)
{
	TigrInternal *win = tigrInternal(bmp);
	D3D9Stuff *d3d = &win->d3d9;
	DWORD flags;
	D3DCAPS9 caps;

	if (!tigrD3D)
	{
		tigrD3D = Direct3DCreate9(D3D_SDK_VERSION);
		if (!tigrD3D)
			tigrError(bmp, "Cannot initialize Direct3D 9.");
	}

	// Initialize D3D
	ZeroMemory(&d3d->params, sizeof(d3d->params));
	d3d->params.Windowed				= TRUE;
	d3d->params.SwapEffect				= D3DSWAPEFFECT_DISCARD;
	d3d->params.BackBufferFormat		= D3DFMT_A8R8G8B8;
	d3d->params.EnableAutoDepthStencil	= FALSE;
	d3d->params.PresentationInterval	= D3DPRESENT_INTERVAL_ONE; // TODO- vsync off if fps suffers?
	d3d->params.Flags					= 0;
	d3d->params.BackBufferWidth			= bmp->w;
	d3d->params.BackBufferHeight		= bmp->h;

	// Check device caps.
	if (FAILED(IDirect3D9_GetDeviceCaps(tigrD3D, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps)))
		tigrError(bmp, "Cannot query Direct3D 9 capabilities.\n");

	// Check vertex processing mode. Ideally I'd just always use software,
	// but some hardware only supports hardware mode (!)
	flags = D3DCREATE_PUREDEVICE | D3DCREATE_FPU_PRESERVE;
	if (caps.VertexProcessingCaps != 0)
		flags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		flags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	// Create the device.
	if (IDirect3D9_CreateDevice(tigrD3D, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, (HWND)bmp->handle,
		flags, &d3d->params, &d3d->dev))
		tigrError(bmp, "Cannot create Direct3D 9 device.\n");

	// Create upscaling shaders.
	if (FAILED(IDirect3DDevice9_CreateVertexShader(d3d->dev, (DWORD *)tigrUpscaleVSCode, &d3d->vs))
	 || FAILED(IDirect3DDevice9_CreatePixelShader(d3d->dev, (DWORD *)tigrUpscalePSCode, &d3d->ps)))
		tigrError(bmp, "Cannot create Direct3D 9 shaders.\n");
}

void tigrGAPIBegin(Tigr *bmp)
{
	HRESULT hr;
	TigrInternal *win = tigrInternal(bmp);
	D3D9Stuff *d3d = &win->d3d9;

	if (d3d->lost)
	{
		// Check if it's OK to resume work yet.
		if (IDirect3DDevice9_TestCooperativeLevel(d3d->dev) == D3DERR_DEVICELOST)
			return;

		hr = IDirect3DDevice9_Reset(d3d->dev, &d3d->params);
		if (hr == D3DERR_DEVICELOST)
			return;

		if (FAILED(hr)
		 || FAILED(IDirect3DDevice9_CreateTexture(d3d->dev, bmp->w, bmp->h, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8,
			D3DPOOL_SYSTEMMEM, &d3d->sysTex[0], NULL))
		 || FAILED(IDirect3DDevice9_CreateTexture(d3d->dev, bmp->w, bmp->h, 1, 0, D3DFMT_A8R8G8B8,
			D3DPOOL_DEFAULT, &d3d->vidTex[0], NULL))
		 || FAILED(IDirect3DDevice9_CreateTexture(d3d->dev, win->widgets->w, win->widgets->h, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8,
			D3DPOOL_SYSTEMMEM, &d3d->sysTex[1], NULL))
		 || FAILED(IDirect3DDevice9_CreateTexture(d3d->dev, win->widgets->w, win->widgets->h, 1, 0, D3DFMT_A8R8G8B8,
			D3DPOOL_DEFAULT, &d3d->vidTex[1], NULL)))
		{
			tigrError(bmp, "Error creating Direct3D 9 textures.\n");
		}

		d3d->lost = 0;
	}
}

void tigrGAPIEnd(Tigr *bmp)
{
	TigrInternal *win = tigrInternal(bmp);
	D3D9Stuff *d3d = &win->d3d9;
	if (!d3d->lost)
	{
		IDirect3DTexture9_Release(d3d->sysTex[0]);
		IDirect3DTexture9_Release(d3d->vidTex[0]);
		IDirect3DTexture9_Release(d3d->sysTex[1]);
		IDirect3DTexture9_Release(d3d->vidTex[1]);
		IDirect3DDevice9_Reset(d3d->dev, &d3d->params);
		d3d->lost = 1;
	}
}

void tigrGAPIDestroy(Tigr *bmp)
{
	TigrInternal *win = tigrInternal(bmp);
	D3D9Stuff *d3d = &win->d3d9;

	IDirect3DVertexShader9_Release(d3d->vs);
	IDirect3DPixelShader9_Release(d3d->ps);
	IDirect3DDevice9_Release(d3d->dev);
}

void tigrGAPIResize(Tigr *bmp, int width, int height)
{
	TigrInternal *win = tigrInternal(bmp);
	D3D9Stuff *d3d = &win->d3d9;

	tigrGAPIEnd(bmp);
	d3d->params.BackBufferWidth = width;
	d3d->params.BackBufferHeight = height;
	tigrGAPIBegin(bmp);
}

void tigrDxUpdate(IDirect3DDevice9 *dev, IDirect3DTexture9 *sysTex, IDirect3DTexture9 *vidTex, Tigr *bmp)
{
	D3DLOCKED_RECT rect;
	TPixel *src, *dest;
	int y;

	// Lock the system memory texture.
	IDirect3DTexture9_LockRect(sysTex, 0, &rect, NULL, D3DLOCK_DISCARD);

	// Copy our bitmap into it.
	src = bmp->pix;
	for (y=0;y<bmp->h;y++)
	{
		dest = (TPixel *)( (char *)rect.pBits + rect.Pitch*y );
		memcpy(dest, src, bmp->w*sizeof(TPixel));
		src += bmp->w;
	}

	IDirect3DTexture9_UnlockRect(sysTex, 0);

	// Update that into somewhere useful.
	IDirect3DDevice9_UpdateTexture(dev, (IDirect3DBaseTexture9 *)sysTex, (IDirect3DBaseTexture9 *)vidTex);
}

void tigrDxQuad(IDirect3DDevice9 *dev, IDirect3DTexture9 *tex, int ix0, int iy0, int ix1, int iy1)
{
	float tri[6][5];
	float x0 = (float)ix0;
	float y0 = (float)iy0;
	float x1 = (float)ix1;
	float y1 = (float)iy1;

	//          x               y               z              u              v
	tri[0][0] = x0; tri[0][1] = y0; tri[0][2] = 0; tri[0][3] = 0; tri[0][4] = 0;
	tri[1][0] = x1; tri[1][1] = y0; tri[1][2] = 0; tri[1][3] = 1; tri[1][4] = 0;
	tri[2][0] = x0; tri[2][1] = y1; tri[2][2] = 0; tri[2][3] = 0; tri[2][4] = 1;

	tri[3][0] = x1; tri[3][1] = y0; tri[3][2] = 0; tri[3][3] = 1; tri[3][4] = 0;
	tri[4][0] = x1; tri[4][1] = y1; tri[4][2] = 0; tri[4][3] = 1; tri[4][4] = 1;
	tri[5][0] = x0; tri[5][1] = y1; tri[5][2] = 0; tri[5][3] = 0; tri[5][4] = 1;

	IDirect3DDevice9_SetTexture(dev, 0, (IDirect3DBaseTexture9 *)tex);
	IDirect3DDevice9_DrawPrimitiveUP(dev, D3DPT_TRIANGLELIST, 2, &tri, 5*4);
}

void tigrGAPIPresent(Tigr *bmp, int dw, int dh)
{
	TigrInternal *win;
	D3D9Stuff *d3d;
	HWND hWnd;
	HRESULT hr;

	win = tigrInternal(bmp);
	d3d = &win->d3d9;
	hWnd = (HWND)bmp->handle;

	// Make sure we have a device.
	// If we don't, then sleep to simulate the vsync until it comes back.
	if (d3d->lost)
	{
		tigrGAPIBegin(bmp);
		if (d3d->lost) {
			Sleep(15);
			return;
		}
	}

	IDirect3DDevice9_BeginScene(d3d->dev);

	// Upload the pixel data.
	tigrDxUpdate(d3d->dev, d3d->sysTex[0], d3d->vidTex[0], bmp);
	tigrDxUpdate(d3d->dev, d3d->sysTex[1], d3d->vidTex[1], win->widgets);

	// Set up our upscale shader.
	IDirect3DDevice9_SetRenderState(d3d->dev, D3DRS_ALPHABLENDENABLE, FALSE);
	IDirect3DDevice9_SetVertexShader(d3d->dev, d3d->vs);
	IDirect3DDevice9_SetPixelShader(d3d->dev, d3d->ps);
	IDirect3DDevice9_SetFVF(d3d->dev, D3DFVF_XYZ|D3DFVF_TEX1);
	IDirect3DDevice9_SetSamplerState(d3d->dev, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
	IDirect3DDevice9_SetSamplerState(d3d->dev, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
	IDirect3DDevice9_SetSamplerState(d3d->dev, 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	IDirect3DDevice9_SetSamplerState(d3d->dev, 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

	// Let the shader know about the window size.
	{
		float consts[12];
		consts[0] = (float)dw;
		consts[1] = (float)dh;
		consts[2] = 1.0f / dw;
		consts[3] = 1.0f / dh;
		consts[4] = (float)bmp->w;
		consts[5] = (float)bmp->h;
		consts[6] = 1.0f / bmp->w;
		consts[7] = 1.0f / bmp->h;
		consts[8] = win->hblur ? 1.0f : 0.0f;
		consts[9] = win->vblur ? 1.0f : 0.0f;
		consts[10] = win->scanlines;
		consts[11] = win->contrast;
		IDirect3DDevice9_SetVertexShaderConstantF(d3d->dev, 0, consts, 3);
		IDirect3DDevice9_SetPixelShaderConstantF(d3d->dev, 0, consts, 3);
	}

	// We clear so that a) we fill the border, and b) to let the driver
	// know it can discard the contents.
	IDirect3DDevice9_Clear(d3d->dev, 0, NULL, D3DCLEAR_TARGET, 0, 0, 0);

	// Blit the final image to the screen.
	tigrDxQuad(d3d->dev, d3d->vidTex[0], win->pos[0], win->pos[1], win->pos[2], win->pos[3]);

	// Draw the widget overlay.
	IDirect3DDevice9_SetRenderState(d3d->dev, D3DRS_ALPHABLENDENABLE, TRUE);
	IDirect3DDevice9_SetRenderState(d3d->dev, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	IDirect3DDevice9_SetRenderState(d3d->dev, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	tigrDxQuad(d3d->dev, d3d->vidTex[1], 
		(int)(dw - win->widgets->w * win->widgetsScale), 0, 
		dw, (int)(win->widgets->h * win->widgetsScale));

	// Et fini.
	IDirect3DDevice9_EndScene(d3d->dev);
	hr = IDirect3DDevice9_Present(d3d->dev, NULL, NULL, hWnd, NULL );

	// See if we lost our device.
	if (hr == D3DERR_DEVICELOST)
		tigrGAPIEnd(bmp);
}

#endif

//////// End of inlined file: tigr_d3d9.c ////////

#endif // _WIN32

//////// Start of inlined file: tigr_upscale_gl_vs.h ////////

// Auto-generated by incbin.pl from tigr_upscale_gl_vs.glsl

const unsigned char tigr_upscale_gl_vs[] = {
	0x23,0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x33,0x33,0x30,0x20,0x63,0x6f,0x72,
	0x65,0x0d,0x0a,0x0d,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x20,0x28,0x6c,0x6f,0x63,
	0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x30,0x29,0x20,0x69,0x6e,0x20,0x76,0x65,
	0x63,0x32,0x20,0x70,0x6f,0x73,0x5f,0x69,0x6e,0x3b,0x0d,0x0a,0x6c,0x61,0x79,0x6f,
	0x75,0x74,0x20,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x31,
	0x29,0x20,0x69,0x6e,0x20,0x76,0x65,0x63,0x32,0x20,0x75,0x76,0x5f,0x69,0x6e,0x3b,
	0x0d,0x0a,0x0d,0x0a,0x6f,0x75,0x74,0x20,0x76,0x65,0x63,0x32,0x20,0x75,0x76,0x3b,
	0x0d,0x0a,0x0d,0x0a,0x75,0x6e,0x69,0x66,0x6f,0x72,0x6d,0x20,0x6d,0x61,0x74,0x34,
	0x20,0x6d,0x6f,0x64,0x65,0x6c,0x3b,0x0d,0x0a,0x75,0x6e,0x69,0x66,0x6f,0x72,0x6d,
	0x20,0x6d,0x61,0x74,0x34,0x20,0x70,0x72,0x6f,0x6a,0x65,0x63,0x74,0x69,0x6f,0x6e,
	0x3b,0x0d,0x0a,0x0d,0x0a,0x76,0x6f,0x69,0x64,0x20,0x6d,0x61,0x69,0x6e,0x28,0x29,
	0x0d,0x0a,0x7b,0x0d,0x0a,0x09,0x75,0x76,0x20,0x3d,0x20,0x75,0x76,0x5f,0x69,0x6e,
	0x3b,0x0d,0x0a,0x09,0x67,0x6c,0x5f,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x20,
	0x3d,0x20,0x70,0x72,0x6f,0x6a,0x65,0x63,0x74,0x69,0x6f,0x6e,0x20,0x2a,0x20,0x6d,
	0x6f,0x64,0x65,0x6c,0x20,0x2a,0x20,0x76,0x65,0x63,0x34,0x28,0x70,0x6f,0x73,0x5f,
	0x69,0x6e,0x2c,0x20,0x30,0x2e,0x30,0x2c,0x20,0x31,0x2e,0x30,0x29,0x3b,0x0d,0x0a,
	0x7d,0x0d,0x0a };

int tigr_upscale_gl_vs_size = (int)sizeof(tigr_upscale_gl_vs);


//////// End of inlined file: tigr_upscale_gl_vs.h ////////

//////// Start of inlined file: tigr_upscale_gl_fs.h ////////

// Auto-generated by incbin.pl from tigr_upscale_gl_fs.glsl

const unsigned char tigr_upscale_gl_fs[] = {
	0x23,0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x33,0x33,0x30,0x20,0x63,0x6f,0x72,
	0x65,0x0d,0x0a,0x0d,0x0a,0x69,0x6e,0x20,0x76,0x65,0x63,0x32,0x20,0x75,0x76,0x3b,
	0x0d,0x0a,0x0d,0x0a,0x6f,0x75,0x74,0x20,0x76,0x65,0x63,0x34,0x20,0x63,0x6f,0x6c,
	0x6f,0x72,0x3b,0x0d,0x0a,0x0d,0x0a,0x75,0x6e,0x69,0x66,0x6f,0x72,0x6d,0x20,0x73,
	0x61,0x6d,0x70,0x6c,0x65,0x72,0x32,0x44,0x20,0x69,0x6d,0x61,0x67,0x65,0x3b,0x0d,
	0x0a,0x75,0x6e,0x69,0x66,0x6f,0x72,0x6d,0x20,0x76,0x65,0x63,0x34,0x20,0x70,0x61,
	0x72,0x61,0x6d,0x65,0x74,0x65,0x72,0x73,0x3b,0x0d,0x0a,0x0d,0x0a,0x76,0x6f,0x69,
	0x64,0x20,0x6d,0x61,0x69,0x6e,0x28,0x29,0x0d,0x0a,0x7b,0x0d,0x0a,0x09,0x76,0x65,
	0x63,0x32,0x20,0x74,0x65,0x78,0x5f,0x73,0x69,0x7a,0x65,0x20,0x3d,0x20,0x74,0x65,
	0x78,0x74,0x75,0x72,0x65,0x53,0x69,0x7a,0x65,0x28,0x69,0x6d,0x61,0x67,0x65,0x2c,
	0x20,0x30,0x29,0x3b,0x0d,0x0a,0x09,0x76,0x65,0x63,0x32,0x20,0x75,0x76,0x5f,0x62,
	0x6c,0x75,0x72,0x20,0x3d,0x20,0x6d,0x69,0x78,0x28,0x66,0x6c,0x6f,0x6f,0x72,0x28,
	0x75,0x76,0x20,0x2a,0x20,0x74,0x65,0x78,0x5f,0x73,0x69,0x7a,0x65,0x29,0x20,0x2b,
	0x20,0x30,0x2e,0x35,0x2c,0x20,0x75,0x76,0x20,0x2a,0x20,0x74,0x65,0x78,0x5f,0x73,
	0x69,0x7a,0x65,0x2c,0x20,0x70,0x61,0x72,0x61,0x6d,0x65,0x74,0x65,0x72,0x73,0x2e,
	0x78,0x79,0x29,0x20,0x2f,0x20,0x74,0x65,0x78,0x5f,0x73,0x69,0x7a,0x65,0x3b,0x0d,
	0x0a,0x09,0x76,0x65,0x63,0x34,0x20,0x63,0x20,0x3d,0x20,0x74,0x65,0x78,0x74,0x75,
	0x72,0x65,0x28,0x69,0x6d,0x61,0x67,0x65,0x2c,0x20,0x75,0x76,0x5f,0x62,0x6c,0x75,
	0x72,0x29,0x3b,0x0d,0x0a,0x09,0x63,0x2e,0x72,0x67,0x62,0x20,0x2a,0x3d,0x20,0x6d,
	0x69,0x78,0x28,0x30,0x2e,0x35,0x2c,0x20,0x31,0x2e,0x30,0x20,0x2d,0x20,0x66,0x72,
	0x61,0x63,0x74,0x28,0x75,0x76,0x2e,0x79,0x20,0x2a,0x20,0x74,0x65,0x78,0x5f,0x73,
	0x69,0x7a,0x65,0x2e,0x79,0x29,0x2c,0x20,0x70,0x61,0x72,0x61,0x6d,0x65,0x74,0x65,
	0x72,0x73,0x2e,0x7a,0x29,0x20,0x2a,0x20,0x32,0x2e,0x30,0x3b,0x20,0x2f,0x2f,0x73,
	0x63,0x61,0x6e,0x6c,0x69,0x6e,0x65,0x0d,0x0a,0x09,0x63,0x20,0x3d,0x20,0x6d,0x69,
	0x78,0x28,0x76,0x65,0x63,0x34,0x28,0x30,0x2e,0x35,0x29,0x2c,0x20,0x63,0x2c,0x20,
	0x70,0x61,0x72,0x61,0x6d,0x65,0x74,0x65,0x72,0x73,0x2e,0x77,0x29,0x3b,0x20,0x2f,
	0x2f,0x63,0x6f,0x6e,0x74,0x72,0x61,0x73,0x74,0x20,0x0d,0x0a,0x09,0x63,0x6f,0x6c,
	0x6f,0x72,0x20,0x3d,0x20,0x63,0x3b,0x0d,0x0a,0x7d,0x0d,0x0a };

int tigr_upscale_gl_fs_size = (int)sizeof(tigr_upscale_gl_fs);


//////// End of inlined file: tigr_upscale_gl_fs.h ////////

//////// Start of inlined file: tigr_win.c ////////

//#include "tigr_internal.h"
#include <assert.h>

// not really windows stuff
TigrInternal *tigrInternal(Tigr *bmp)
{
	assert(bmp->handle);
	return (TigrInternal *)(bmp + 1);
}

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>

#define WIDGET_SCALE	3
#define WIDGET_FADE		16

int main(int argc, char *argv[]);

#ifndef TIGR_DO_NOT_PRESERVE_WINDOW_POSITION
HKEY tigrRegKey;
#endif

static wchar_t *unicode(const char *str)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, 0, 0);
	wchar_t *dest = (wchar_t *)malloc(sizeof(wchar_t) * len);
	MultiByteToWideChar(CP_UTF8, 0, str, -1, dest, len);
	return dest;
}

void tigrError(Tigr *bmp, const char *message, ...)
{
	char tmp[1024];

	va_list args;
	va_start(args, message);
	_vsnprintf(tmp, sizeof(tmp), message, args);
	tmp[sizeof(tmp)-1] = 0;
	va_end(args);

	MessageBoxW(bmp ? (HWND)bmp->handle : NULL, unicode(tmp), bmp ? tigrInternal(bmp)->wtitle : L"Error", MB_OK|MB_ICONERROR);
	exit(1);
}

void tigrEnterBorderlessWindowed(Tigr *bmp)
{
	// Enter borderless windowed mode.
	MONITORINFO mi = { sizeof(mi) };
	TigrInternal *win = tigrInternal(bmp);

	GetWindowRect((HWND)bmp->handle, &win->oldPos);

	GetMonitorInfo(MonitorFromWindow((HWND)bmp->handle, MONITOR_DEFAULTTONEAREST), &mi);
	win->dwStyle = WS_VISIBLE | WS_POPUP;
	SetWindowLong((HWND)bmp->handle, GWL_STYLE, win->dwStyle);
	SetWindowPos((HWND)bmp->handle, HWND_TOP,
		mi.rcMonitor.left,
		mi.rcMonitor.top,
		mi.rcMonitor.right - mi.rcMonitor.left,
		mi.rcMonitor.bottom - mi.rcMonitor.top,
		0);
}

void tigrLeaveBorderlessWindowed(Tigr *bmp)
{
	TigrInternal *win = tigrInternal(bmp);

	win->dwStyle = WS_VISIBLE | WS_OVERLAPPEDWINDOW;
	SetWindowLong((HWND)bmp->handle, GWL_STYLE, win->dwStyle);

	SetWindowPos((HWND)bmp->handle, NULL,
		win->oldPos.left,
		win->oldPos.top,
		win->oldPos.right - win->oldPos.left,
		win->oldPos.bottom - win->oldPos.top,
		0);
}

void tigrDxUpdateWidgets(Tigr *bmp, int dw, int dh)
{
	POINT pt;
	int i, x, clicked=0;
	char str[8];
	TPixel col;
	TPixel off = tigrRGB(255,255,255);
	TPixel on = tigrRGB(0,200,255);
	TigrInternal *win = tigrInternal(bmp);
	(void)dh;

	tigrClear(win->widgets, tigrRGBA(0,0,0,0));

	if (!(win->dwStyle & WS_POPUP))
	{
		win->widgetsWanted = 0;
		win->widgetAlpha = 0;
		return;
	}

	// See if we want to be showing widgets or not.
	GetCursorPos(&pt);
	ScreenToClient((HWND)bmp->handle, &pt);
	if (pt.y == 0)
		win->widgetsWanted = 1;
	if (pt.y > win->widgets->h*WIDGET_SCALE)
		win->widgetsWanted = 0;

	// Track the alpha.
	if (win->widgetsWanted)
		win->widgetAlpha = (win->widgetAlpha <= 255-WIDGET_FADE) ? win->widgetAlpha+WIDGET_FADE : 255;
	else
		win->widgetAlpha = (win->widgetAlpha >= WIDGET_FADE) ? win->widgetAlpha-WIDGET_FADE : 0;

	// Get relative coords.
	pt.x -= (dw - win->widgets->w*WIDGET_SCALE);
	pt.x /= WIDGET_SCALE;
	pt.y /= WIDGET_SCALE;

	tigrClear(win->widgets, tigrRGBA(0,0,0,win->widgetAlpha));

	// Render it.
	for (i=0;i<3;i++)
	{
		switch(i) {
			case 0: str[0] = '_'; str[1] = 0; break; // "_" (minimize)
			case 1: str[0] = 0xEF; str[1] = 0xBF; str[2] = 0xBD; str[3] = 0; break; // "[]" (maximize)
			case 2: str[0] = 0xC3; str[1] = 0x97; str[2] = 0; break; // "x" (close)
		}
		x = win->widgets->w + (i-3)*12;
		if (i == 2)
			off = tigrRGB(255,0,0);
		if (pt.x >= x && pt.x < x+10 && pt.y < win->widgets->h)
		{
			col = on;
			if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
				clicked |= 1<<i;
		} else {
			col = off;
		}
		col.a = win->widgetAlpha;
		tigrPrint(win->widgets, tfont, x, 2, col, str);
	}

	if (clicked & 1)
		ShowWindow((HWND)bmp->handle, SW_MINIMIZE);
	if (clicked & 2)
		tigrLeaveBorderlessWindowed(bmp);
	if (clicked & 4)
		SendMessage((HWND)bmp->handle, WM_CLOSE, 0, 0);
}

void tigrUpdate(Tigr *bmp)
{
	MSG msg;
	RECT rc;
	int dw, dh;
	TigrInternal *win = tigrInternal(bmp);

	if (!win->shown)
	{
		win->shown = 1;
		UpdateWindow((HWND)bmp->handle);
		ShowWindow((HWND)bmp->handle, SW_SHOW);
	}

	// Get the window size.
	GetClientRect((HWND)bmp->handle, &rc);
	dw = rc.right - rc.left;
	dh = rc.bottom - rc.top;

	// Update the widget overlay.
	tigrDxUpdateWidgets(bmp, dw, dh);

	tigrGAPIPresent(bmp, dw, dh);

	memcpy(win->prev, win->keys, 256);

	// Run the message pump.
	while (PeekMessage(&msg, (HWND)bmp->handle, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			break;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

static BOOL UnadjustWindowRectEx(LPRECT prc, DWORD dwStyle, BOOL fMenu, DWORD dwExStyle)
{
	BOOL fRc;
	RECT rc;
	SetRectEmpty(&rc);
	fRc = AdjustWindowRectEx(&rc, dwStyle, fMenu, dwExStyle);
	if (fRc) {
		prc->left -= rc.left;
		prc->top -= rc.top;
		prc->right -= rc.right;
		prc->bottom -= rc.bottom;
		}
	return fRc;
}

LRESULT CALLBACK tigrWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Tigr *bmp;
	TigrInternal *win = NULL;
	RECT rc;
	int dw, dh;

	GetClientRect(hWnd, &rc);
	dw = rc.right - rc.left;
	dh = rc.bottom - rc.top;

	bmp = (Tigr *)GetPropW(hWnd, L"Tigr");
	if (bmp)
		win = tigrInternal(bmp);

	switch (message)
	{
	case WM_PAINT:
		tigrGAPIPresent(bmp, dw, dh);
		ValidateRect(hWnd, NULL);
		break;
	case WM_CLOSE:
		if (win)
			win->closed = 1;
		break;
	case WM_GETMINMAXINFO:
		if (bmp)
		{
			MINMAXINFO *info = (MINMAXINFO *)lParam;
			RECT rc;
			rc.left = 0;
			rc.top = 0;
			if (win->flags & TIGR_AUTO)
			{
				rc.right = 32;
				rc.bottom = 32;
			} else {
				int minscale = tigrEnforceScale(1, win->flags);
				rc.right = bmp->w * minscale;
				rc.bottom = bmp->h * minscale;
			}
			AdjustWindowRectEx(&rc, win->dwStyle, FALSE, 0);
			info->ptMinTrackSize.x = rc.right - rc.left;
			info->ptMinTrackSize.y = rc.bottom - rc.top;
		}
		return 0;
	case WM_SIZING:
		if (win)
		{
			// Calculate scale-constrained sizes.
			RECT *rc = (RECT *)lParam;
			int dx, dy;
			UnadjustWindowRectEx(rc, win->dwStyle, FALSE, 0);
			dx = (rc->right - rc->left) % win->scale;
			dy = (rc->bottom - rc->top) % win->scale;
			switch (wParam) {
			case WMSZ_LEFT: rc->left += dx; break;
			case WMSZ_RIGHT: rc->right -= dx; break;
			case WMSZ_TOP: rc->top += dy; break;
			case WMSZ_TOPLEFT: rc->left += dx; rc->top += dy; break;
			case WMSZ_TOPRIGHT: rc->right -= dx; rc->top += dy; break;
			case WMSZ_BOTTOM: rc->bottom -= dy; break;
			case WMSZ_BOTTOMLEFT: rc->left += dx; rc->bottom -= dy; break;
			case WMSZ_BOTTOMRIGHT: rc->right -= dx; rc->bottom -= dy; break;
			}
			AdjustWindowRectEx(rc, win->dwStyle, FALSE, 0);
		}
		return TRUE;
	case WM_SIZE:
		if (win)
		{
			if (wParam != SIZE_MINIMIZED)
			{
				// Detect window size changes and update our bitmap accordingly.
				dw = LOWORD(lParam);
				dh = HIWORD(lParam);
				if (win->flags & TIGR_AUTO)
				{
					tigrResize(bmp, dw/win->scale, dh/win->scale);
				} else {
					win->scale = tigrEnforceScale(tigrCalcScale(bmp->w, bmp->h, dw, dh), win->flags);
				}
				tigrPosition(bmp, win->scale, dw, dh, win->pos);
				tigrGAPIResize(bmp, dw, dh);
			}

			// If someone tried to maximize us (e.g. via shortcut launch options),
			// prefer instead to be borderless.
			if (wParam == SIZE_MAXIMIZED)
			{
				ShowWindow((HWND)bmp->handle, SW_NORMAL);
				tigrEnterBorderlessWindowed(bmp);
			}
		}
		return 0;
	#ifndef TIGR_DO_NOT_PRESERVE_WINDOW_POSITION
	case WM_WINDOWPOSCHANGED:
		{
			// Save our position.
			WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
			GetWindowPlacement(hWnd, &wp);
			if (win->dwStyle & WS_POPUP)
				wp.showCmd = SW_MAXIMIZE;
			RegSetValueExW(tigrRegKey, win->wtitle, 0, REG_BINARY, (BYTE *)&wp, sizeof(wp));
			return DefWindowProcW(hWnd, message, wParam, lParam);
		}
	#endif
	case WM_ACTIVATE:
		if (win) {
			memset(win->keys, 0, 256);
			memset(win->prev, 0, 256);
			win->lastChar = 0;
		}
		return 0;
	case WM_CHAR:
		if (win) {
			if (wParam == '\r') wParam = '\n';
				win->lastChar = wParam;
		}
		return DefWindowProcW(hWnd, message, wParam, lParam);
	case WM_MENUCHAR:
		// Disable beep on Alt+Enter
		if (LOWORD(wParam) == VK_RETURN)
			return MNC_CLOSE<<16;
		return DefWindowProcW(hWnd, message, wParam, lParam);
	case WM_SYSKEYDOWN:
		if (win)
		{
			if (wParam == VK_RETURN)
			{
				// Alt+Enter
				if (win->dwStyle & WS_POPUP)
					tigrLeaveBorderlessWindowed(bmp);
				else
					tigrEnterBorderlessWindowed(bmp);
				return 0;
			}
		}
		// fall-thru
	case WM_KEYDOWN:
		if (win)
			win->keys[wParam] = 1;
		return DefWindowProcW(hWnd, message, wParam, lParam);
	case WM_SYSKEYUP:
		// fall-thru
	case WM_KEYUP:
		if (win)
			win->keys[wParam] = 0;
		return DefWindowProcW(hWnd, message, wParam, lParam);
	default:
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}
	return 0;
}

Tigr *tigrWindow(int w, int h, const char *title, int flags)
{
	WNDCLASSEXW wcex = {0};
	int maxW, maxH, scale;
	HWND hWnd;
	DWORD dwStyle;
	RECT rc;
	DWORD err;
	Tigr *bmp;
	TigrInternal *win;
	#ifndef TIGR_DO_NOT_PRESERVE_WINDOW_POSITION
	WINDOWPLACEMENT wp;
	DWORD wpsize = sizeof(wp);
	#endif
	
	wchar_t *wtitle = unicode(title);

	// Find our registry key.
	#ifndef TIGR_DO_NOT_PRESERVE_WINDOW_POSITION
	RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\TIGR", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &tigrRegKey, NULL);
	#endif

	// Register a window class.
	wcex.cbSize			= sizeof(WNDCLASSEXW);
	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc	= tigrWndProc;
	wcex.hInstance		= GetModuleHandle(NULL);
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.lpszClassName	= L"TIGR";
	RegisterClassExW(&wcex);

	if (flags & TIGR_AUTO)
	{
		// Always use a 1:1 pixel size.
		scale = 1;
	} else {
		// See how big we can make it and still fit on-screen.
		maxW = GetSystemMetrics(SM_CXSCREEN) * 3/4;
		maxH = GetSystemMetrics(SM_CYSCREEN) * 3/4;
		scale = tigrCalcScale(w, h, maxW, maxH);
	}

	scale = tigrEnforceScale(scale, flags);

	// Get the final window size.
	dwStyle = WS_OVERLAPPEDWINDOW;
	rc.left = 0; rc.top = 0; rc.right = w*scale; rc.bottom = h*scale;
	AdjustWindowRect(&rc, dwStyle, FALSE);

	// Make a window.
	hWnd = CreateWindowW(L"TIGR", wtitle, dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right-rc.left, rc.bottom-rc.top,
		NULL, NULL, wcex.hInstance, NULL);
	err = GetLastError();
	if (!hWnd)
		ExitProcess(1);

	// Wrap a bitmap around it.
	bmp = tigrBitmap2(w, h, sizeof(TigrInternal));
	bmp->handle = hWnd;

	// Set up the Windows parts.
	win = tigrInternal(bmp);
	win->dwStyle = dwStyle;
	win->wtitle = wtitle;
	win->shown = 0;
	win->closed = 0;
	#ifdef TIGR_GAPI_D3D9
	win->d3d9.lost = 1;
	#endif
	win->scale = scale;
	win->lastChar = 0;
	win->flags = flags;

	win->hblur = win->vblur = 0;
	win->scanlines = 0.0f;
	win->contrast = 1.0f;

	win->widgetsWanted = 0;
	win->widgetAlpha = 0;
	win->widgetsScale = WIDGET_SCALE;
	win->widgets = tigrBitmap(40, 14);

	SetPropW(hWnd, L"Tigr", bmp);

	tigrGAPICreate(bmp);
	tigrGAPIBegin(bmp);

	// Try and restore our window position.
	#ifndef TIGR_DO_NOT_PRESERVE_WINDOW_POSITION
	if (RegQueryValueExW(tigrRegKey, wtitle, NULL, NULL, (BYTE *)&wp, &wpsize) == ERROR_SUCCESS)
	{
		if (wp.showCmd == SW_MAXIMIZE)
			tigrEnterBorderlessWindowed(bmp);
		else
			SetWindowPlacement(hWnd, &wp);
	}
	#endif

	return bmp;
}

void tigrFree(Tigr *bmp)
{
	if (bmp->handle)
	{
		TigrInternal *win = tigrInternal(bmp);
		DestroyWindow((HWND)bmp->handle);
		tigrGAPIEnd(bmp);
		tigrGAPIDestroy(bmp);
		free(win->wtitle);
		tigrFree(win->widgets);
	}
	free(bmp->pix);
	free(bmp);
}

int tigrClosed(Tigr *bmp)
{
	TigrInternal *win = tigrInternal(bmp);
	int val = win->closed;
	win->closed = 0;
	return val;
}

float tigrTime()
{
	static int first = 1;
	static LARGE_INTEGER prev;

	LARGE_INTEGER cnt, freq;
	ULONGLONG diff;
	QueryPerformanceCounter(&cnt);
	QueryPerformanceFrequency(&freq);

	if (first)
	{
		first = 0;
		prev = cnt;
	}

	diff = cnt.QuadPart - prev.QuadPart;
	prev = cnt;
	return (float)(diff / (double)freq.QuadPart);
}

void tigrMouse(Tigr *bmp, int *x, int *y, int *buttons)
{
	POINT pt;
	TigrInternal *win;

	win = tigrInternal(bmp);
	GetCursorPos(&pt);
	ScreenToClient((HWND)bmp->handle, &pt);
	*x = (pt.x - win->pos[0]) / win->scale;
	*y = (pt.y - win->pos[1]) / win->scale;
	*buttons = 0;
	if (GetFocus() != bmp->handle)
		return;
	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) *buttons |= 1;
	if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) *buttons |= 2;
	if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) *buttons |= 4;
}

static int tigrWinVK(int key)
{
	if (key >= 'A' && key <= 'Z') return key;
	if (key >= '0' && key <= '9') return key;
	switch (key) {
	case TK_BACKSPACE: return VK_BACK;
	case TK_TAB: return VK_TAB;
	case TK_RETURN: return VK_RETURN;
	case TK_SHIFT: return VK_SHIFT;
	case TK_CONTROL: return VK_CONTROL;
	case TK_ALT: return VK_MENU;
	case TK_PAUSE: return VK_PAUSE;
	case TK_CAPSLOCK: return VK_CAPITAL;
	case TK_ESCAPE: return VK_ESCAPE;
	case TK_SPACE: return VK_SPACE;
	case TK_PAGEUP: return VK_PRIOR;
	case TK_PAGEDN: return VK_NEXT;
	case TK_END: return VK_END;
	case TK_HOME: return VK_HOME;
	case TK_LEFT: return VK_LEFT;
	case TK_UP: return VK_UP;
	case TK_RIGHT: return VK_RIGHT;
	case TK_DOWN: return VK_DOWN;
	case TK_INSERT: return VK_INSERT;
	case TK_DELETE: return VK_DELETE;
	case TK_LWIN: return VK_LWIN;
	case TK_RWIN: return VK_RWIN;
	//case TK_APPS: return VK_APPS; // this key doesn't exist on OS X
	case TK_PAD0: return VK_NUMPAD0;
	case TK_PAD1: return VK_NUMPAD1;
	case TK_PAD2: return VK_NUMPAD2;
	case TK_PAD3: return VK_NUMPAD3;
	case TK_PAD4: return VK_NUMPAD4;
	case TK_PAD5: return VK_NUMPAD5;
	case TK_PAD6: return VK_NUMPAD6;
	case TK_PAD7: return VK_NUMPAD7;
	case TK_PAD8: return VK_NUMPAD8;
	case TK_PAD9: return VK_NUMPAD9;
	case TK_PADMUL: return VK_MULTIPLY;
	case TK_PADADD: return VK_ADD;
	case TK_PADENTER: return VK_SEPARATOR;
	case TK_PADSUB: return VK_SUBTRACT;
	case TK_PADDOT: return VK_DECIMAL;
	case TK_PADDIV: return VK_DIVIDE;
	case TK_F1: return VK_F1;
	case TK_F2: return VK_F2;
	case TK_F3: return VK_F3;
	case TK_F4: return VK_F4;
	case TK_F5: return VK_F5;
	case TK_F6: return VK_F6;
	case TK_F7: return VK_F7;
	case TK_F8: return VK_F8;
	case TK_F9: return VK_F9;
	case TK_F10: return VK_F10;
	case TK_F11: return VK_F11;
	case TK_F12: return VK_F12;
	case TK_NUMLOCK: return VK_NUMLOCK;
	case TK_SCROLL: return VK_SCROLL;
	case TK_LSHIFT: return VK_LSHIFT;
	case TK_RSHIFT: return VK_RSHIFT;
	case TK_LCONTROL: return VK_LCONTROL;
	case TK_RCONTROL: return VK_RCONTROL;
	case TK_LALT: return VK_LMENU;
	case TK_RALT: return VK_RMENU;
	case TK_SEMICOLON: return VK_OEM_1;
	case TK_EQUALS: return VK_OEM_PLUS;
	case TK_COMMA: return VK_OEM_COMMA;
	case TK_MINUS: return VK_OEM_MINUS;
	case TK_DOT: return VK_OEM_PERIOD;
	case TK_SLASH: return VK_OEM_2;
	case TK_BACKTICK: return VK_OEM_3;
	case TK_LSQUARE: return VK_OEM_4;
	case TK_BACKSLASH: return VK_OEM_5;
	case TK_RSQUARE: return VK_OEM_6;
	case TK_TICK: return VK_OEM_7;
	}
	return 0;
}

int tigrKeyDown(Tigr *bmp, int key)
{
	TigrInternal *win;
	int k = tigrWinVK(key);
	if (GetFocus() != bmp->handle)
		return 0;
	win = tigrInternal(bmp);
	return win->keys[k] && !win->prev[k];
}

int tigrKeyHeld(Tigr *bmp, int key)
{
	TigrInternal *win;
	int k = tigrWinVK(key);
	if (GetFocus() != bmp->handle)
		return 0;
	win = tigrInternal(bmp);
	return win->keys[k];
}

int tigrReadChar(Tigr *bmp)
{
	TigrInternal *win = tigrInternal(bmp);
	int c = win->lastChar;
	win->lastChar = 0;
	return c;
}

// We supply our own WinMain and just chain through to the user's
// real entry point.
#ifdef UNICODE
int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
#else
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
{
	int n, argc;
	LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
	char **argv = (char **)calloc(argc+1, sizeof(int));

	(void)hInstance; (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;

	for (n=0;n<argc;n++)
	{
		int len = WideCharToMultiByte(CP_UTF8, 0, wargv[n], -1, 0, 0, NULL, NULL);
		argv[n] = (char *)malloc(len);
		WideCharToMultiByte(CP_UTF8, 0, wargv[n], -1, argv[n], len, NULL, NULL);
	}
	return main(argc, argv);
}
#endif

//////// End of inlined file: tigr_win.c ////////

//////// Start of inlined file: tigr_osx.c ////////

// this one is based on https://github.com/jimon/osx_app_in_plain_c

//#include "tigr_internal.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#ifdef TARGET_OS_MAC

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <math.h>
#include <assert.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <objc/NSObjCRuntime.h>
#include <mach/mach_time.h>

// maybe this is available somewhere in objc runtime?
#if __LP64__ || (TARGET_OS_EMBEDDED && !TARGET_OS_IPHONE) || TARGET_OS_WIN32 || NS_BUILD_32_LIKE_64
#define NSIntegerEncoding "q"
#define NSUIntegerEncoding "L"
#else
#define NSIntegerEncoding "i"
#define NSUIntegerEncoding "I"
#endif

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#else
// this is how they are defined originally
#include <CoreGraphics/CGBase.h>
#include <CoreGraphics/CGGeometry.h>
typedef CGPoint NSPoint;
typedef CGSize NSSize;
typedef CGRect NSRect;

extern id NSApp;
extern id const NSDefaultRunLoopMode;
#endif

#if defined(__OBJC__) && __has_feature(objc_arc)
//#define ARC_AVAILABLE
#error "Can't compile as objective-c code just yet! (see autorelease pool todo below)"
#endif

// ABI is a bit different between platforms
#ifdef __arm64__
#define abi_objc_msgSend_stret objc_msgSend
#else
#define abi_objc_msgSend_stret objc_msgSend_stret
#endif
#ifdef __i386__
#define abi_objc_msgSend_fpret objc_msgSend_fpret
#else
#define abi_objc_msgSend_fpret objc_msgSend
#endif

#define objc_msgSend_id				((id (*)(id, SEL))objc_msgSend)
#define objc_msgSend_void			((void (*)(id, SEL))objc_msgSend)
#define objc_msgSend_void_id		((void (*)(id, SEL, id))objc_msgSend)
#define objc_msgSend_void_bool		((void (*)(id, SEL, BOOL))objc_msgSend)
#define objc_msgSend_id_const_char	((id (*)(id, SEL, const char*))objc_msgSend)

bool terminated = false;

// we gonna construct objective-c class by hand in runtime, so wow, so hacker!
NSUInteger applicationShouldTerminate(id self, SEL _sel, id sender)
{
	terminated = true;
	return 0;
}

void windowWillClose(id self, SEL _sel, id notification)
{
	NSUInteger value = true;
	object_setInstanceVariable(self, "closed", (void*)value);
}

void windowDidBecomeKey(id self, SEL _sel, id notification)
{
	TigrInternal * win;
	Tigr * bmp = 0;
	object_getInstanceVariable(self, "tigrHandle", (void**)&bmp);
	win = bmp ? tigrInternal(bmp) : NULL;

	if(win)
	{
		memset(win->keys, 0, 256);
		memset(win->prev, 0, 256);
		win->lastChar = 0;
		win->mouseButtons = 0;
	}
}

bool _tigrCocoaIsWindowClosed(id window)
{
	id wdg = objc_msgSend_id(window, sel_registerName("delegate"));
	if(!wdg)
		return false;
	NSUInteger value = 0;
	object_getInstanceVariable(wdg, "closed", (void**)&value);
	return value ? true : false;
}

static bool tigrOSXInited = false;
static id autoreleasePool = NULL;

void _tigrCleanupOSX()
{
	#ifdef ARC_AVAILABLE
	// TODO autorelease pool
	#else
	objc_msgSend_void(autoreleasePool, sel_registerName("drain"));
	#endif
}

void tigrInitOSX()
{
	if(tigrOSXInited)
		return;

	#ifdef ARC_AVAILABLE
	// TODO and what do we do now? it's a bit too tricky to use @autoreleasepool here
	#error TODO this code should be compiled as C for now
	#else
	//would be nice to use objc_autoreleasePoolPush instead, but it's not publically available in the headers
	id poolAlloc = objc_msgSend_id((id)objc_getClass("NSAutoreleasePool"), sel_registerName("alloc"));
	autoreleasePool = objc_msgSend_id(poolAlloc, sel_registerName("init"));
	atexit(&_tigrCleanupOSX);
	#endif

	objc_msgSend_id((id)objc_getClass("NSApplication"), sel_registerName("sharedApplication"));
	((void (*)(id, SEL, NSInteger))objc_msgSend)(NSApp, sel_registerName("setActivationPolicy:"), 0);

	Class appDelegateClass = objc_allocateClassPair((Class)objc_getClass("NSObject"), "AppDelegate", 0);
	bool resultAddProtoc = class_addProtocol(appDelegateClass, objc_getProtocol("NSApplicationDelegate"));
	assert(resultAddProtoc);
	bool resultAddMethod = class_addMethod(appDelegateClass, sel_registerName("applicationShouldTerminate:"), (IMP)applicationShouldTerminate, NSUIntegerEncoding "@:@");
	assert(resultAddMethod);
	id dgAlloc = objc_msgSend_id((id)appDelegateClass, sel_registerName("alloc"));
	id dg = objc_msgSend_id(dgAlloc, sel_registerName("init"));
	#ifndef ARC_AVAILABLE
	objc_msgSend_void(dg, sel_registerName("autorelease"));
	#endif

	objc_msgSend_void_id(NSApp, sel_registerName("setDelegate:"), dg);
	objc_msgSend_void(NSApp, sel_registerName("finishLaunching"));

	id menubarAlloc = objc_msgSend_id((id)objc_getClass("NSMenu"), sel_registerName("alloc"));
	id menubar = objc_msgSend_id(menubarAlloc, sel_registerName("init"));
	#ifndef ARC_AVAILABLE
	objc_msgSend_void(menubar, sel_registerName("autorelease"));
	#endif

	id appMenuItemAlloc = objc_msgSend_id((id)objc_getClass("NSMenuItem"), sel_registerName("alloc"));
	id appMenuItem = objc_msgSend_id(appMenuItemAlloc, sel_registerName("init"));
	#ifndef ARC_AVAILABLE
	objc_msgSend_void(appMenuItem, sel_registerName("autorelease"));
	#endif

	objc_msgSend_void_id(menubar, sel_registerName("addItem:"), appMenuItem);
	((id (*)(id, SEL, id))objc_msgSend)(NSApp, sel_registerName("setMainMenu:"), menubar);

	id appMenuAlloc = objc_msgSend_id((id)objc_getClass("NSMenu"), sel_registerName("alloc"));
	id appMenu = objc_msgSend_id(appMenuAlloc, sel_registerName("init"));
	#ifndef ARC_AVAILABLE
	objc_msgSend_void(appMenu, sel_registerName("autorelease"));
	#endif

	id processInfo = objc_msgSend_id((id)objc_getClass("NSProcessInfo"), sel_registerName("processInfo"));
	id appName = objc_msgSend_id(processInfo, sel_registerName("processName"));

	id quitTitlePrefixString = objc_msgSend_id_const_char((id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), "Quit ");
	id quitTitle = ((id (*)(id, SEL, id))objc_msgSend)(quitTitlePrefixString, sel_registerName("stringByAppendingString:"), appName);

	id quitMenuItemKey = objc_msgSend_id_const_char((id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), "q");
	id quitMenuItemAlloc = objc_msgSend_id((id)objc_getClass("NSMenuItem"), sel_registerName("alloc"));
	id quitMenuItem = ((id (*)(id, SEL, id, SEL, id))objc_msgSend)(quitMenuItemAlloc, sel_registerName("initWithTitle:action:keyEquivalent:"), quitTitle, sel_registerName("terminate:"), quitMenuItemKey);
	#ifndef ARC_AVAILABLE
	objc_msgSend_void(quitMenuItem, sel_registerName("autorelease"));
	#endif

	objc_msgSend_void_id(appMenu, sel_registerName("addItem:"), quitMenuItem);
	objc_msgSend_void_id(appMenuItem, sel_registerName("setSubmenu:"), appMenu);

	tigrOSXInited = true;
}

void tigrError(Tigr *bmp, const char *message, ...)
{
	char tmp[1024];

	va_list args;
	va_start(args, message);
	vsnprintf(tmp, sizeof(tmp), message, args);
	tmp[sizeof(tmp)-1] = 0;
	va_end(args);

	CFStringRef header = CFStringCreateWithCString(NULL, "Error", kCFStringEncodingUTF8);
	CFStringRef msg = CFStringCreateWithCString(NULL, tmp, kCFStringEncodingUTF8);
	CFUserNotificationDisplayNotice(0.0, kCFUserNotificationStopAlertLevel, NULL, NULL, NULL, header, msg, NULL);
	CFRelease(header);
	CFRelease(msg);
	exit(1);
}

NSSize _tigrCocoaWindowSize(id window)
{
	id contentView = objc_msgSend_id(window, sel_registerName("contentView"));
	NSRect rect = ((NSRect (*)(id, SEL))abi_objc_msgSend_stret)(contentView, sel_registerName("frame"));
	rect = ((NSRect (*)(id, SEL, NSRect))abi_objc_msgSend_stret)(contentView, sel_registerName("convertRectToBacking:"), rect);

	return rect.size;
}

TigrInternal * _tigrInternalCocoa(id window)
{
	if(!window)
		return NULL;

	id wdg = objc_msgSend_id(window, sel_registerName("delegate"));
	if(!wdg)
		return NULL;

	Tigr * bmp = 0;
	object_getInstanceVariable(wdg, "tigrHandle", (void**)&bmp);
	return bmp ? tigrInternal(bmp) : NULL;
}

Tigr *tigrWindow(int w, int h, const char *title, int flags)
{
	int scale;
	Tigr *bmp;
	TigrInternal *win;

	tigrInitOSX();

	if (flags & TIGR_AUTO)
	{
		// Always use a 1:1 pixel size.
		scale = 1;
	} else {
		// See how big we can make it and still fit on-screen.
		CGRect mainMonitor = CGDisplayBounds(CGMainDisplayID());
		int maxW = CGRectGetHeight(mainMonitor) * 3/4;
		int maxH = CGRectGetWidth(mainMonitor) * 3/4;
		scale = tigrCalcScale(w, h, maxW, maxH);
	}

	scale = tigrEnforceScale(scale, flags);

	NSRect rect = {{0, 0}, {w * scale, h * scale}};
	id windowAlloc = objc_msgSend_id((id)objc_getClass("NSWindow"), sel_registerName("alloc"));
	id window = ((id (*)(id, SEL, NSRect, NSUInteger, NSUInteger, BOOL))objc_msgSend)(windowAlloc, sel_registerName("initWithContentRect:styleMask:backing:defer:"), rect, 15, 2, NO);
	#ifndef ARC_AVAILABLE
	objc_msgSend_void(window, sel_registerName("autorelease"));
	#endif

	// when we are not using ARC, than window will be added to autorelease pool
	// so if we close it by hand (pressing red button), we don't want it to be released for us
	// so it will be released by autorelease pool later
	objc_msgSend_void_bool(window, sel_registerName("setReleasedWhenClosed:"), NO);

	Class WindowDelegateClass = objc_allocateClassPair((Class)objc_getClass("NSObject"), "WindowDelegate", 0);
	bool resultAddProtoc = class_addProtocol(WindowDelegateClass, objc_getProtocol("NSWindowDelegate"));
	assert(resultAddProtoc);
	bool resultAddIvar = class_addIvar(WindowDelegateClass, "closed", sizeof(NSUInteger), rint(log2(sizeof(NSUInteger))), NSUIntegerEncoding);
	assert(resultAddIvar);
	resultAddIvar = class_addIvar(WindowDelegateClass, "tigrHandle", sizeof(void*), rint(log2(sizeof(void*))), "ˆv");
	assert(resultAddIvar);
	bool resultAddMethod = class_addMethod(WindowDelegateClass, sel_registerName("windowWillClose:"), (IMP)windowWillClose,  "v@:@");
	assert(resultAddMethod);
	resultAddMethod = class_addMethod(WindowDelegateClass, sel_registerName("windowDidBecomeKey:"), (IMP)windowDidBecomeKey,  "v@:@");
	assert(resultAddMethod);
	id wdgAlloc = objc_msgSend_id((id)WindowDelegateClass, sel_registerName("alloc"));
	id wdg = objc_msgSend_id(wdgAlloc, sel_registerName("init"));
	#ifndef ARC_AVAILABLE
	objc_msgSend_void(wdg, sel_registerName("autorelease"));
	#endif

	objc_msgSend_void_id(window, sel_registerName("setDelegate:"), wdg);

	id contentView = objc_msgSend_id(window, sel_registerName("contentView"));

	if(flags & TIGR_RETINA)
		objc_msgSend_void_bool(contentView, sel_registerName("setWantsBestResolutionOpenGLSurface:"), YES);

	NSPoint point = {20, 20};
	((void (*)(id, SEL, NSPoint))objc_msgSend)(window, sel_registerName("cascadeTopLeftFromPoint:"), point);

	id titleString = objc_msgSend_id_const_char((id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), title);
	objc_msgSend_void_id(window, sel_registerName("setTitle:"), titleString);

	uint32_t glAttributes[] =
	{
		8, 24,			//	NSOpenGLPFAColorSize, 24,
		11, 8,			//	NSOpenGLPFAAlphaSize, 8,
		5,				//	NSOpenGLPFADoubleBuffer,
		73,				//	NSOpenGLPFAAccelerated,
		//72,			//	NSOpenGLPFANoRecovery,
		//55, 1,		//	NSOpenGLPFASampleBuffers, 1,
		//56, 4,		//	NSOpenGLPFASamples, 4,
		99, 0x3200,		//	NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
		0
	};

	id pixelFormatAlloc = objc_msgSend_id((id)objc_getClass("NSOpenGLPixelFormat"), sel_registerName("alloc"));
	id pixelFormat = ((id (*)(id, SEL, const uint32_t*))objc_msgSend)(pixelFormatAlloc, sel_registerName("initWithAttributes:"), glAttributes);
	#ifndef ARC_AVAILABLE
	objc_msgSend_void(pixelFormat, sel_registerName("autorelease"));
	#endif

	id openGLContextAlloc = objc_msgSend_id((id)objc_getClass("NSOpenGLContext"), sel_registerName("alloc"));
	id openGLContext = ((id (*)(id, SEL, id, id))objc_msgSend)(openGLContextAlloc, sel_registerName("initWithFormat:shareContext:"), pixelFormat, nil);
	#ifndef ARC_AVAILABLE
	objc_msgSend_void(openGLContext, sel_registerName("autorelease"));
	#endif

	objc_msgSend_void_id(openGLContext, sel_registerName("setView:"), contentView);
	objc_msgSend_void_id(window, sel_registerName("makeKeyAndOrderFront:"), window);
	objc_msgSend_void_bool(window, sel_registerName("setAcceptsMouseMovedEvents:"), YES);

	id blackColor = objc_msgSend_id((id)objc_getClass("NSColor"), sel_registerName("blackColor"));
	objc_msgSend_void_id(window, sel_registerName("setBackgroundColor:"), blackColor);

	// TODO do we really need this?
	objc_msgSend_void_bool(NSApp, sel_registerName("activateIgnoringOtherApps:"), YES);

	// Wrap a bitmap around it.
	NSSize windowSize = _tigrCocoaWindowSize(window);
	bmp = tigrBitmap2(windowSize.width / scale, windowSize.height / scale, sizeof(TigrInternal));
	bmp->handle = window;

	// Set the handle
	object_setInstanceVariable(wdg, "tigrHandle", (void*)bmp);

	// Set up the Windows parts.
	win = tigrInternal(bmp);
	win->glContext = openGLContext;
	win->shown = 0;
	win->closed = 0;
	win->scale = scale;
	win->lastChar = 0;
	win->flags = flags;
	win->hblur = win->vblur = 0;
	win->scanlines = 0.0f;
	win->contrast = 1.0f;
	win->widgetsWanted = 0;
	win->widgetAlpha = 0;
	win->widgetsScale = 0;
	win->widgets = tigrBitmap(40, 14);
	win->gl.gl_legacy = 0;
	win->mouseButtons = 0;

	tigrPosition(bmp, win->scale, bmp->w, bmp->h, win->pos);

	objc_msgSend_void(openGLContext, sel_registerName("makeCurrentContext"));
	tigrGAPICreate(bmp);
	tigrGAPIBegin(bmp);
	tigrGAPIResize(bmp, bmp->w, bmp->h);

	return bmp;
}

void tigrFree(Tigr *bmp)
{
	if(bmp->handle)
	{
		TigrInternal * win = tigrInternal(bmp);
		objc_msgSend_void((id)win->glContext, sel_registerName("makeCurrentContext"));
		tigrGAPIEnd(bmp);
		tigrGAPIDestroy(bmp);
		tigrFree(win->widgets);

		id window = (id)bmp->handle;
		if(!_tigrCocoaIsWindowClosed(window) && !terminated)
			objc_msgSend_void(window, sel_registerName("close"));
	}
	free(bmp->pix);
	free(bmp);
}

uint8_t _tigrKeyFromOSX(uint16_t key)
{
	// from Carbon HIToolbox/Events.h
	enum
	{
		kVK_ANSI_A                    = 0x00,
		kVK_ANSI_S                    = 0x01,
		kVK_ANSI_D                    = 0x02,
		kVK_ANSI_F                    = 0x03,
		kVK_ANSI_H                    = 0x04,
		kVK_ANSI_G                    = 0x05,
		kVK_ANSI_Z                    = 0x06,
		kVK_ANSI_X                    = 0x07,
		kVK_ANSI_C                    = 0x08,
		kVK_ANSI_V                    = 0x09,
		kVK_ANSI_B                    = 0x0B,
		kVK_ANSI_Q                    = 0x0C,
		kVK_ANSI_W                    = 0x0D,
		kVK_ANSI_E                    = 0x0E,
		kVK_ANSI_R                    = 0x0F,
		kVK_ANSI_Y                    = 0x10,
		kVK_ANSI_T                    = 0x11,
		kVK_ANSI_1                    = 0x12,
		kVK_ANSI_2                    = 0x13,
		kVK_ANSI_3                    = 0x14,
		kVK_ANSI_4                    = 0x15,
		kVK_ANSI_6                    = 0x16,
		kVK_ANSI_5                    = 0x17,
		kVK_ANSI_Equal                = 0x18,
		kVK_ANSI_9                    = 0x19,
		kVK_ANSI_7                    = 0x1A,
		kVK_ANSI_Minus                = 0x1B,
		kVK_ANSI_8                    = 0x1C,
		kVK_ANSI_0                    = 0x1D,
		kVK_ANSI_RightBracket         = 0x1E,
		kVK_ANSI_O                    = 0x1F,
		kVK_ANSI_U                    = 0x20,
		kVK_ANSI_LeftBracket          = 0x21,
		kVK_ANSI_I                    = 0x22,
		kVK_ANSI_P                    = 0x23,
		kVK_ANSI_L                    = 0x25,
		kVK_ANSI_J                    = 0x26,
		kVK_ANSI_Quote                = 0x27,
		kVK_ANSI_K                    = 0x28,
		kVK_ANSI_Semicolon            = 0x29,
		kVK_ANSI_Backslash            = 0x2A,
		kVK_ANSI_Comma                = 0x2B,
		kVK_ANSI_Slash                = 0x2C,
		kVK_ANSI_N                    = 0x2D,
		kVK_ANSI_M                    = 0x2E,
		kVK_ANSI_Period               = 0x2F,
		kVK_ANSI_Grave                = 0x32,
		kVK_ANSI_KeypadDecimal        = 0x41,
		kVK_ANSI_KeypadMultiply       = 0x43,
		kVK_ANSI_KeypadPlus           = 0x45,
		kVK_ANSI_KeypadClear          = 0x47,
		kVK_ANSI_KeypadDivide         = 0x4B,
		kVK_ANSI_KeypadEnter          = 0x4C,
		kVK_ANSI_KeypadMinus          = 0x4E,
		kVK_ANSI_KeypadEquals         = 0x51,
		kVK_ANSI_Keypad0              = 0x52,
		kVK_ANSI_Keypad1              = 0x53,
		kVK_ANSI_Keypad2              = 0x54,
		kVK_ANSI_Keypad3              = 0x55,
		kVK_ANSI_Keypad4              = 0x56,
		kVK_ANSI_Keypad5              = 0x57,
		kVK_ANSI_Keypad6              = 0x58,
		kVK_ANSI_Keypad7              = 0x59,
		kVK_ANSI_Keypad8              = 0x5B,
		kVK_ANSI_Keypad9              = 0x5C,
		kVK_Return                    = 0x24,
		kVK_Tab                       = 0x30,
		kVK_Space                     = 0x31,
		kVK_Delete                    = 0x33,
		kVK_Escape                    = 0x35,
		kVK_Command                   = 0x37,
		kVK_Shift                     = 0x38,
		kVK_CapsLock                  = 0x39,
		kVK_Option                    = 0x3A,
		kVK_Control                   = 0x3B,
		kVK_RightShift                = 0x3C,
		kVK_RightOption               = 0x3D,
		kVK_RightControl              = 0x3E,
		kVK_Function                  = 0x3F,
		kVK_F17                       = 0x40,
		kVK_VolumeUp                  = 0x48,
		kVK_VolumeDown                = 0x49,
		kVK_Mute                      = 0x4A,
		kVK_F18                       = 0x4F,
		kVK_F19                       = 0x50,
		kVK_F20                       = 0x5A,
		kVK_F5                        = 0x60,
		kVK_F6                        = 0x61,
		kVK_F7                        = 0x62,
		kVK_F3                        = 0x63,
		kVK_F8                        = 0x64,
		kVK_F9                        = 0x65,
		kVK_F11                       = 0x67,
		kVK_F13                       = 0x69,
		kVK_F16                       = 0x6A,
		kVK_F14                       = 0x6B,
		kVK_F10                       = 0x6D,
		kVK_F12                       = 0x6F,
		kVK_F15                       = 0x71,
		kVK_Help                      = 0x72,
		kVK_Home                      = 0x73,
		kVK_PageUp                    = 0x74,
		kVK_ForwardDelete             = 0x75,
		kVK_F4                        = 0x76,
		kVK_End                       = 0x77,
		kVK_F2                        = 0x78,
		kVK_PageDown                  = 0x79,
		kVK_F1                        = 0x7A,
		kVK_LeftArrow                 = 0x7B,
		kVK_RightArrow                = 0x7C,
		kVK_DownArrow                 = 0x7D,
		kVK_UpArrow                   = 0x7E
	};

	switch(key)
	{
	case kVK_ANSI_Q: return 'Q';
	case kVK_ANSI_W: return 'W';
	case kVK_ANSI_E: return 'E';
	case kVK_ANSI_R: return 'R';
	case kVK_ANSI_T: return 'T';
	case kVK_ANSI_Y: return 'Y';
	case kVK_ANSI_U: return 'U';
	case kVK_ANSI_I: return 'I';
	case kVK_ANSI_O: return 'O';
	case kVK_ANSI_P: return 'P';
	case kVK_ANSI_A: return 'A';
	case kVK_ANSI_S: return 'S';
	case kVK_ANSI_D: return 'D';
	case kVK_ANSI_F: return 'F';
	case kVK_ANSI_G: return 'G';
	case kVK_ANSI_H: return 'H';
	case kVK_ANSI_J: return 'J';
	case kVK_ANSI_K: return 'K';
	case kVK_ANSI_L: return 'L';
	case kVK_ANSI_Z: return 'Z';
	case kVK_ANSI_X: return 'X';
	case kVK_ANSI_C: return 'C';
	case kVK_ANSI_V: return 'V';
	case kVK_ANSI_B: return 'B';
	case kVK_ANSI_N: return 'N';
	case kVK_ANSI_M: return 'M';
	case kVK_ANSI_0: return '0';
	case kVK_ANSI_1: return '1';
	case kVK_ANSI_2: return '2';
	case kVK_ANSI_3: return '3';
	case kVK_ANSI_4: return '4';
	case kVK_ANSI_5: return '5';
	case kVK_ANSI_6: return '6';
	case kVK_ANSI_7: return '7';
	case kVK_ANSI_8: return '8';
	case kVK_ANSI_9: return '9';
	case kVK_ANSI_Keypad0: return TK_PAD0;
	case kVK_ANSI_Keypad1: return TK_PAD1;
	case kVK_ANSI_Keypad2: return TK_PAD2;
	case kVK_ANSI_Keypad3: return TK_PAD3;
	case kVK_ANSI_Keypad4: return TK_PAD4;
	case kVK_ANSI_Keypad5: return TK_PAD5;
	case kVK_ANSI_Keypad6: return TK_PAD6;
	case kVK_ANSI_Keypad7: return TK_PAD7;
	case kVK_ANSI_Keypad8: return TK_PAD8;
	case kVK_ANSI_Keypad9: return TK_PAD9;
	case kVK_ANSI_KeypadMultiply: return TK_PADMUL;
	case kVK_ANSI_KeypadPlus: return TK_PADADD;
	case kVK_ANSI_KeypadEnter: return TK_PADENTER;
	case kVK_ANSI_KeypadMinus: return TK_PADSUB;
	case kVK_ANSI_KeypadDecimal: return TK_PADDOT;
	case kVK_ANSI_KeypadDivide: return TK_PADDIV;
	case kVK_F1: return TK_F1;
	case kVK_F2: return TK_F2;
	case kVK_F3: return TK_F3;
	case kVK_F4: return TK_F4;
	case kVK_F5: return TK_F5;
	case kVK_F6: return TK_F6;
	case kVK_F7: return TK_F7;
	case kVK_F8: return TK_F8;
	case kVK_F9: return TK_F9;
	case kVK_F10: return TK_F10;
	case kVK_F11: return TK_F11;
	case kVK_F12: return TK_F12;
	case kVK_Shift: return TK_LSHIFT;
	case kVK_Control: return TK_LCONTROL;
	case kVK_Option: return TK_LALT;
	case kVK_CapsLock: return TK_CAPSLOCK;
	case kVK_Command: return TK_LWIN;
	case kVK_Command - 1: return TK_RWIN;
	case kVK_RightShift: return TK_RSHIFT;
	case kVK_RightControl: return TK_RCONTROL;
	case kVK_RightOption: return TK_RALT;
	case kVK_Delete: return TK_BACKSPACE;
	case kVK_Tab: return TK_TAB;
	case kVK_Return: return TK_RETURN;
	case kVK_Escape: return TK_ESCAPE;
	case kVK_Space: return TK_SPACE;
	case kVK_PageUp: return TK_PAGEUP;
	case kVK_PageDown: return TK_PAGEDN;
	case kVK_End: return TK_END;
	case kVK_Home: return TK_HOME;
	case kVK_LeftArrow: return TK_LEFT;
	case kVK_UpArrow: return TK_UP;
	case kVK_RightArrow: return TK_RIGHT;
	case kVK_DownArrow: return TK_DOWN;
	case kVK_Help: return TK_INSERT;
	case kVK_ForwardDelete: return TK_DELETE;
	case kVK_F14: return TK_SCROLL;
	case kVK_F15: return TK_PAUSE;
	case kVK_ANSI_KeypadClear: return TK_NUMLOCK;
	case kVK_ANSI_Semicolon: return TK_SEMICOLON;
	case kVK_ANSI_Equal: return TK_EQUALS;
	case kVK_ANSI_Comma: return TK_COMMA;
	case kVK_ANSI_Minus: return TK_MINUS;
	case kVK_ANSI_Slash: return TK_SLASH;
	case kVK_ANSI_Backslash: return TK_BACKSLASH;
	case kVK_ANSI_Grave: return TK_BACKTICK;
	case kVK_ANSI_Quote: return TK_TICK;
	case kVK_ANSI_LeftBracket: return TK_LSQUARE;
	case kVK_ANSI_RightBracket: return TK_RSQUARE;
	case kVK_ANSI_Period: return TK_DOT;
	default: return 0;
	}
}

void _tigrOnCocoaEvent(id event, id window)
{
	if(!event)
		return;

	TigrInternal * win = _tigrInternalCocoa(window);
	if(!win) // just pipe the event
	{
		objc_msgSend_void_id(NSApp, sel_registerName("sendEvent:"), event);
		return;
	}

	NSUInteger eventType = ((NSUInteger (*)(id, SEL))objc_msgSend)(event, sel_registerName("type"));
	switch(eventType)
	{
	case 1: // NSLeftMouseDown
		win->mouseButtons |= 1;
		break;
	case 2: // NSLeftMouseUp
		win->mouseButtons &= ~1;
		break;
	case 3: // NSRightMouseDown
		win->mouseButtons |= 2;
		break;
	case 4: // NSRightMouseUp
		win->mouseButtons &= ~2;
		break;
	case 25: // NSOtherMouseDown
	{
		// number == 2 is a middle button
		NSInteger number = ((NSInteger (*)(id, SEL))objc_msgSend)(event, sel_registerName("buttonNumber"));
		if(number == 2)
			win->mouseButtons |= 4;
		break;
	}
	case 26: // NSOtherMouseUp
	{
		NSInteger number = ((NSInteger (*)(id, SEL))objc_msgSend)(event, sel_registerName("buttonNumber"));
		if(number == 2)
			win->mouseButtons &= ~4;
		break;
	}
	//case 22: // NSScrollWheel
	//{
	//	CGFloat deltaX = ((CGFloat (*)(id, SEL))abi_objc_msgSend_fpret)(event, sel_registerName("scrollingDeltaX"));
	//	CGFloat deltaY = ((CGFloat (*)(id, SEL))abi_objc_msgSend_fpret)(event, sel_registerName("scrollingDeltaY"));
	//	BOOL precisionScrolling = ((BOOL (*)(id, SEL))objc_msgSend)(event, sel_registerName("hasPreciseScrollingDeltas"));
	//
	//	if(precisionScrolling)
	//	{
	//		deltaX *= 0.1f; // similar to glfw
	//		deltaY *= 0.1f;
	//	}
	//
	//	if(fabs(deltaX) > 0.0f || fabs(deltaY) > 0.0f)
	//		printf("mouse scroll wheel delta %f %f\n", deltaX, deltaY);
	//	break;
	//}
	case 12: // NSFlagsChanged
	{
		NSUInteger modifiers = ((NSUInteger (*)(id, SEL))objc_msgSend)(event, sel_registerName("modifierFlags"));

		// based on NSEventModifierFlags and NSDeviceIndependentModifierFlagsMask
		struct
		{
			union
			{
				struct
				{
					uint8_t alpha_shift:1;
					uint8_t shift:1;
					uint8_t control:1;
					uint8_t alternate:1;
					uint8_t command:1;
					uint8_t numeric_pad:1;
					uint8_t help:1;
					uint8_t function:1;
				};
				uint8_t mask;
			};
		} keys;

		keys.mask = (modifiers & 0xffff0000UL) >> 16;

		// TODO L,R variation of keys?
		win->keys[TK_CONTROL] = keys.alpha_shift;
		win->keys[TK_SHIFT] = keys.shift;
		win->keys[TK_CONTROL] = keys.control;
		win->keys[TK_ALT] = keys.alternate;
		win->keys[TK_LWIN] = keys.command;
		win->keys[TK_RWIN] = keys.command;
		break;
	}
	case 10: // NSKeyDown
	{
		id inputText = objc_msgSend_id(event, sel_registerName("characters"));
		const char * inputTextUTF8 = ((const char* (*)(id, SEL))objc_msgSend)(inputText, sel_registerName("UTF8String"));

		tigrDecodeUTF8(inputTextUTF8, &win->lastChar);

		uint16_t keyCode = ((unsigned short (*)(id, SEL))objc_msgSend)(event, sel_registerName("keyCode"));
		win->keys[_tigrKeyFromOSX(keyCode)] = 1;
		break;
	}
	case 11: // NSKeyUp
	{
		uint16_t keyCode = ((unsigned short (*)(id, SEL))objc_msgSend)(event, sel_registerName("keyCode"));
		win->keys[_tigrKeyFromOSX(keyCode)] = 0;
		break;
	}
	default:
		break;
	}

	objc_msgSend_void_id(NSApp, sel_registerName("sendEvent:"), event);
}

void tigrUpdate(Tigr *bmp)
{
	TigrInternal *win;
	id openGLContext;
	id window;
	win = tigrInternal(bmp);
	window = (id)bmp->handle;
	openGLContext = (id)win->glContext;

	id keyWindow = objc_msgSend_id(NSApp, sel_registerName("keyWindow"));

	if(keyWindow == window)
		memcpy(win->prev, win->keys, 256);

	id distantPast = objc_msgSend_id((id)objc_getClass("NSDate"), sel_registerName("distantPast"));
	id event = ((id (*)(id, SEL, NSUInteger, id, id, BOOL))objc_msgSend)(NSApp, sel_registerName("nextEventMatchingMask:untilDate:inMode:dequeue:"), NSUIntegerMax, distantPast, NSDefaultRunLoopMode, YES);
	_tigrOnCocoaEvent(event, keyWindow);
	if(terminated || _tigrCocoaIsWindowClosed(window))
		return;

	// do runloop stuff
	objc_msgSend_void(NSApp, sel_registerName("updateWindows"));
	objc_msgSend_void(openGLContext, sel_registerName("update"));
	objc_msgSend_void(openGLContext, sel_registerName("makeCurrentContext"));

	NSSize windowSize = _tigrCocoaWindowSize(window);

	if (win->flags & TIGR_AUTO)
		tigrResize(bmp, windowSize.width / win->scale, windowSize.height / win->scale);
	else
		win->scale = tigrEnforceScale(tigrCalcScale(bmp->w, bmp->h, windowSize.width, windowSize.height), win->flags);

	tigrPosition(bmp, win->scale, windowSize.width, windowSize.height, win->pos);
	tigrGAPIResize(bmp, windowSize.width, windowSize.height);
	tigrGAPIPresent(bmp, windowSize.width, windowSize.height);
	objc_msgSend_void(openGLContext, sel_registerName("flushBuffer"));
}

int tigrClosed(Tigr *bmp)
{
	return (terminated || _tigrCocoaIsWindowClosed((id)bmp->handle)) ? 1 : 0;
}

void tigrMouse(Tigr *bmp, int *x, int *y, int *buttons)
{
	TigrInternal *win;
	id window;
	win = tigrInternal(bmp);
	window = (id)bmp->handle;

	id windowContentView = objc_msgSend_id(window, sel_registerName("contentView"));
	NSRect adjustFrame = ((NSRect (*)(id, SEL))abi_objc_msgSend_stret)(windowContentView, sel_registerName("frame"));

	// NSPoint is small enough to fit a register, so no need for objc_msgSend_stret
	NSPoint p = ((NSPoint (*)(id, SEL))objc_msgSend)(window, sel_registerName("mouseLocationOutsideOfEventStream"));

	// map input to content view rect
	if(p.x < 0) p.x = 0;
	else if(p.x > adjustFrame.size.width) p.x = adjustFrame.size.width;
	if(p.y < 0) p.y = 0;
	else if(p.y > adjustFrame.size.height) p.y = adjustFrame.size.height;

	// map input to pixels
	NSRect r = {p.x, p.y, 0, 0};
	r = ((NSRect (*)(id, SEL, NSRect))abi_objc_msgSend_stret)(windowContentView, sel_registerName("convertRectToBacking:"), r);
	p = r.origin;

	p.x = (p.x - win->pos[0]) / win->scale;
	p.y = (adjustFrame.size.height - p.y - win->pos[1]) / win->scale;

	if(x)
		*x = p.x;
	if(y)
		*y = p.y;

	if(buttons)
	{
		id keyWindow = objc_msgSend_id(NSApp, sel_registerName("keyWindow"));
		*buttons = keyWindow != bmp->handle ? 0 : win->mouseButtons;
	}
}

int tigrKeyDown(Tigr *bmp, int key)
{
	TigrInternal *win;
	assert(key < 256);
	id keyWindow = objc_msgSend_id(NSApp, sel_registerName("keyWindow"));
	if(keyWindow != bmp->handle)
		return 0;
	win = tigrInternal(bmp);
	return win->keys[key] && !win->prev[key];
}

int tigrKeyHeld(Tigr *bmp, int key)
{
	TigrInternal *win;
	assert(key < 256);
	id keyWindow = objc_msgSend_id(NSApp, sel_registerName("keyWindow"));
	if(keyWindow != bmp->handle)
		return 0;
	win = tigrInternal(bmp);
	return win->keys[key];
}

int tigrReadChar(Tigr *bmp)
{
	TigrInternal *win = tigrInternal(bmp);
	int c = win->lastChar;
	win->lastChar = 0;
	return c;
}

float tigrTime()
{
	static uint64_t time = 0;
	static mach_timebase_info_data_t timebaseInfo;

	if(timebaseInfo.denom == 0)
	{
		mach_timebase_info(&timebaseInfo);
		time = mach_absolute_time();
		return 0.0f;
	}

	uint64_t current_time = mach_absolute_time();
	double elapsed = (double)(current_time - time) * timebaseInfo.numer / (timebaseInfo.denom * 1000000000.0);
	time = current_time;
	return (float)elapsed;
}

#endif
#endif

//////// End of inlined file: tigr_osx.c ////////

//////// Start of inlined file: tigr_gl.c ////////

//#include "tigr_internal.h"
#include <stdio.h> // TODO can we remove this and printf's later?

#ifdef TIGR_GAPI_GL
#ifndef __APPLE__
// please provide you own glext.h, you can download latest at https://www.opengl.org/registry/api/GL/glext.h
#include <glext.h>
#endif
#ifdef _WIN32
// please provide you own wglext.h, you can download latest at https://www.opengl.org/registry/api/GL/wglext.h
#include <wglext.h>
#endif

extern const unsigned char tigr_upscale_gl_vs[], tigr_upscale_gl_fs[];
extern int tigr_upscale_gl_vs_size, tigr_upscale_gl_fs_size;

#ifdef _WIN32
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormat;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribs;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLGENBUFFERSARBPROC glGenBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLUNIFORM4FPROC glUniform4f;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
PFNGLACTIVETEXTUREPROC glActiveTexture;
int tigrGL11Init(Tigr *bmp)
{
	int pixel_format;
	TigrInternal *win = tigrInternal(bmp);
	GLStuff *gl= &win->gl;
	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SWAP_EXCHANGE,
		PFD_TYPE_RGBA,
		32, // color bits
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0,
		24, // depth
		8,  // stencil
		0,
		PFD_MAIN_PLANE, // is it ignored ?
		0,
		0, 0, 0
	};
	if(!(gl->dc = GetDC((HWND)bmp->handle))) {tigrError(bmp, "Cannot create OpenGL device context.\n"); return -1;}
	if(!(pixel_format = ChoosePixelFormat(gl->dc, &pfd))) {tigrError(bmp, "Cannot choose OpenGL pixel format.\n"); return -1;}
	if(!SetPixelFormat(gl->dc, pixel_format, &pfd)) {tigrError(bmp, "Cannot set OpenGL pixel format.\n"); return -1;}
	if(!(gl->hglrc = wglCreateContext(gl->dc))) {tigrError(bmp, "Cannot create OpenGL context.\n"); return -1;}
	if(!wglMakeCurrent(gl->dc, gl->hglrc)) {tigrError(bmp, "Cannot activate OpenGL context.\n"); return -1;}
	gl->gl_legacy = 1;
	return 0;
}
int tigrGL33Init(Tigr *bmp)
{
	int pixel_format;
	UINT num_formats;
	TigrInternal *win = tigrInternal(bmp);
	GLStuff *gl= &win->gl;

	wglChoosePixelFormat = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	wglCreateContextAttribs = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
	glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
	glGenBuffers = (PFNGLGENBUFFERSARBPROC)wglGetProcAddress("glGenBuffers");
	glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
	glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
	glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
	glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
	glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
	glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
	glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
	glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
	glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
	glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
	glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
	glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
	glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
	glUniform4f = (PFNGLUNIFORM4FPROC)wglGetProcAddress("glUniform4f");
	glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv");
	glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");

	if(!wglChoosePixelFormat || !wglCreateContextAttribs) {tigrError(bmp, "Cannot create OpenGL context.\n"); return -1;}
	const int attribList[] =
	{
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB, 32,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		0
	};
	int attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		0
	};
	if(!wglChoosePixelFormat(gl->dc, attribList, NULL, 1, &pixel_format, &num_formats)) {tigrError(bmp, "Cannot choose OpenGL pixel format.\n"); return -1;}
	if(!(gl->hglrc = wglCreateContextAttribs(gl->dc, gl->hglrc, attribs))) {tigrError(bmp, "Cannot create OpenGL context attribs.\n"); return -1;}
	if(!wglMakeCurrent(gl->dc, gl->hglrc)) {tigrError(bmp, "Cannot activate OpenGL context.\n"); return -1;}
	gl->gl_legacy = 0;
	return 0;
}
#endif

void tigrCheckGLError(const char *state)
{
	GLenum err = glGetError();
	if(err != GL_NO_ERROR)
		printf("got gl error %x when was doing %s\n", err, state);
}

void tigrCheckShaderErrors(GLuint object)
{
	GLint success;
	GLchar info[2048];
	glGetShaderiv(object, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(object, sizeof(info), NULL, info);
		printf("shader compile error : %s\n", info);
	}
}

void tigrCheckProgramErrors(GLuint object)
{
	GLint success;
	GLchar info[2048];
	glGetProgramiv(object, GL_LINK_STATUS, &success);
	if(!success)
	{
		glGetProgramInfoLog(object, sizeof(info), NULL, info);
		printf("shader link error : %s\n", info);
	}
}

void tigrGAPICreate(Tigr *bmp)
{
	GLuint vs, fs;
	TigrInternal *win = tigrInternal(bmp);
	GLStuff *gl= &win->gl;
	GLuint VBO;
	GLfloat vertices[] = {
		// pos      uv
		0.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 1.0f, 0.0f
	};

	#ifdef _WIN32
	if(tigrGL11Init(bmp))
		return;
	tigrGL33Init(bmp);
	#endif

	//printf("ogl version %s\n", glGetString(GL_VERSION));

	if(!gl->gl_legacy)
	{
		// create vao
		glGenVertexArrays(1, &gl->vao);
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glBindVertexArray(gl->vao);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), NULL);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), NULL);

		// create program
		vs = glCreateShader(GL_VERTEX_SHADER);
		const char *vs_source = (const char*)&tigr_upscale_gl_vs;
		glShaderSource(vs, 1, &vs_source, &tigr_upscale_gl_vs_size);
		glCompileShader(vs);
		tigrCheckShaderErrors(vs);
		fs = glCreateShader(GL_FRAGMENT_SHADER);
		const char *fs_source = (const char*)&tigr_upscale_gl_fs;
		glShaderSource(fs, 1, &fs_source, &tigr_upscale_gl_fs_size);
		glCompileShader(fs);
		tigrCheckShaderErrors(fs);
		gl->program = glCreateProgram();
		glAttachShader(gl->program, vs);
		glAttachShader(gl->program, fs);
		glLinkProgram(gl->program);
		tigrCheckProgramErrors(gl->program);
		glDeleteShader(vs);
		glDeleteShader(fs);
		gl->uniform_projection = glGetUniformLocation(gl->program, "projection");
		gl->uniform_model = glGetUniformLocation(gl->program, "model");
		gl->uniform_parameters = glGetUniformLocation(gl->program, "parameters");
	}

	// create textures
	if(gl->gl_legacy)
		glEnable(GL_TEXTURE_2D);
	glGenTextures(2, gl->tex);
	for(int i = 0; i < 2; ++i) {
		glBindTexture(GL_TEXTURE_2D, gl->tex[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl->gl_legacy ? GL_NEAREST : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl->gl_legacy ? GL_NEAREST : GL_LINEAR);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	}

	tigrCheckGLError("initialization");
}

void tigrGAPIBegin(Tigr *bmp)
{
}

void tigrGAPIEnd(Tigr *bmp)
{
}

void tigrGAPIDestroy(Tigr *bmp)
{
	TigrInternal *win = tigrInternal(bmp);
	GLStuff *gl= &win->gl;

	if(!gl->gl_legacy)
	{
		glDeleteTextures(2, gl->tex);
		glDeleteProgram(gl->program);
	}

	tigrCheckGLError("destroy");

	#ifdef _WIN32
	if(!wglMakeCurrent(NULL, NULL)) {tigrError(bmp, "Cannot deactivate OpenGL context.\n"); return;}
	if(gl->hglrc && !wglDeleteContext(gl->hglrc)) {tigrError(bmp, "Cannot delete OpenGL context.\n"); return;}
	gl->hglrc = NULL;

	if(gl->dc && !ReleaseDC((HWND)bmp->handle, gl->dc)) {tigrError(bmp, "Cannot release OpenGL device context.\n"); return;}
	gl->dc = NULL;
	#endif
}

void tigrGAPIResize(Tigr *bmp, int width, int height)
{
	// no-op
	(void)bmp;
	(void)width;
	(void)height;
}

void tigrGAPIDraw(int legacy, GLuint uniform_model, GLuint tex, Tigr *bmp, int x1, int y1, int x2, int y2)
{
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, bmp->w, bmp->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, bmp->pix);

	if(!legacy)
	{
		float sx = (float)(x2 - x1);
		float sy = (float)(y2 - y1);
		float tx = (float)x1;
		float ty = (float)y1;

		float model[16] =
		{
			  sx, 0.0f, 0.0f, 0.0f,
			0.0f,   sy, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			  tx,   ty, 0.0f, 1.0f
		};

		glUniformMatrix4fv(uniform_model, 1, GL_FALSE, model);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	else
	{
		#ifndef __APPLE__
		glBegin(GL_QUADS);
		glTexCoord2f(1.0f, 0.0f); glVertex2i(x2, y1);
		glTexCoord2f(0.0f, 0.0f); glVertex2i(x1, y1);
		glTexCoord2f(0.0f, 1.0f); glVertex2i(x1, y2);
		glTexCoord2f(1.0f, 1.0f); glVertex2i(x2, y2);
		glEnd();
		#else
		assert(0);
		#endif
	}
}

void tigrGAPIPresent(Tigr *bmp, int w, int h)
{
	TigrInternal *win = tigrInternal(bmp);
	GLStuff *gl= &win->gl;

#ifdef _WIN32
	wglMakeCurrent(gl->dc, gl->hglrc);
#endif
	glViewport(0, 0, w, h);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	if(!gl->gl_legacy)
	{
		float projection[16] =
		{
			 2.0f / w,  0.0f    , 0.0f, 0.0f,
			 0.0f    , -2.0f / h, 0.0f, 0.0f,
			 0.0f    ,  0.0f    , 1.0f, 0.0f,
			-1.0f    ,  1.0f    , 0.0f, 1.0f
		};

		glActiveTexture(GL_TEXTURE0);
		glBindVertexArray(gl->vao);
		glUseProgram(gl->program);
		glUniformMatrix4fv(gl->uniform_projection, 1, GL_FALSE, projection);
		glUniform4f(gl->uniform_parameters, win->hblur ? 1.0f : 0.0f, win->vblur ? 1.0f : 0.0f, win->scanlines, win->contrast);
	}
	else
	{
		#ifndef __APPLE__
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, w, h, 0, -1.0f, 1.0f);
		glEnable(GL_TEXTURE_2D);
		#else
		assert(0);
		#endif
	}

	glDisable(GL_BLEND);
	tigrGAPIDraw(gl->gl_legacy, gl->uniform_model, gl->tex[0], bmp, win->pos[0], win->pos[1], win->pos[2], win->pos[3]);

	if(win->widgetsScale > 0)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		tigrGAPIDraw(gl->gl_legacy, gl->uniform_model, gl->tex[1], win->widgets, 
			(int)(w - win->widgets->w * win->widgetsScale), 0, 
			w, (int)(win->widgets->h * win->widgetsScale));
	}

	tigrCheckGLError("present");

	#ifdef _WIN32
	if(!SwapBuffers(gl->dc)) {tigrError(bmp, "Cannot swap OpenGL buffers.\n"); return;}
	#endif
}

#endif

//////// End of inlined file: tigr_gl.c ////////


//////// End of inlined file: tigr_amalgamated.c ////////

