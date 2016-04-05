#include "tigr_internal.h"
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
