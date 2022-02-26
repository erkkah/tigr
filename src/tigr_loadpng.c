#include "tigr_internal.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct {
    const unsigned char *p, *end;
} PNG;

static unsigned get32(const unsigned char* v) {
    return (v[0] << 24) | (v[1] << 16) | (v[2] << 8) | v[3];
}

static const unsigned char* find(PNG* png, const char* chunk, unsigned minlen) {
    const unsigned char* start;
    while (png->p < png->end) {
        unsigned len = get32(png->p + 0);
        start = png->p;
        png->p += len + 12;
        if (memcmp(start + 4, chunk, 4) == 0 && len >= minlen && png->p <= png->end)
            return start + 8;
    }

    return NULL;
}

static unsigned char paeth(unsigned char a, unsigned char b, unsigned char c) {
    int p = a + b - c;
    int pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);
    return (pa <= pb && pa <= pc) ? a : (pb <= pc) ? b : c;
}

static int rowBytes(int w, int bipp) {
    int rowBits = w * bipp;
    return rowBits / 8 + ((rowBits % 8) ? 1 : 0);
}

static int unfilter(int w, int h, int bipp, unsigned char* raw) {
    int len = rowBytes(w, bipp);
    int bpp = rowBytes(1, bipp);
    int x, y;
    unsigned char* first = (unsigned char*)malloc(len + 1);
    memset(first, 0, len + 1);
    unsigned char* prev = first;
    for (y = 0; y < h; y++, prev = raw, raw += len) {
#define LOOP(A, B)            \
    for (x = 0; x < bpp; x++) \
        raw[x] += A;          \
    for (; x < len; x++)      \
        raw[x] += B;          \
    break
        switch (*raw++) {
            case 0:
                break;
            case 1:
                LOOP(0, raw[x - bpp]);
            case 2:
                LOOP(prev[x], prev[x]);
            case 3:
                LOOP(prev[x] / 2, (raw[x - bpp] + prev[x]) / 2);
            case 4:
                LOOP(prev[x], paeth(raw[x - bpp], prev[x], prev[x - bpp]));
            default:
                return 0;
        }
#undef LOOP
    }
    free(first);
    return 1;
}

static void convert(int bypp, int w, int h, const unsigned char* src, TPixel* dest, const unsigned char* trns) {
    int x, y;
    for (y = 0; y < h; y++) {
        src++;  // skip filter byte
        for (x = 0; x < w; x++, src += bypp) {
            switch (bypp) {
                case 1: {
                    unsigned char c = src[0];
                    if (trns && c == *trns) {
                        *dest++ = tigrRGBA(c, c, c, 0);
                        break;
                    } else {
                        *dest++ = tigrRGB(c, c, c);
                        break;
                    }
                }
                case 2:
                    *dest++ = tigrRGBA(src[0], src[0], src[0], src[1]);
                    break;
                case 3: {
                    unsigned char r = src[0];
                    unsigned char g = src[1];
                    unsigned char b = src[2];
                    if (trns && trns[1] == r && trns[3] == g && trns[5] == b) {
                        *dest++ = tigrRGBA(r, g, b, 0);
                        break;
                    } else {
                        *dest++ = tigrRGB(r, g, b);
                        break;
                    }
                }
                case 4:
                    *dest++ = tigrRGBA(src[0], src[1], src[2], src[3]);
                    break;
            }
        }
    }
}

static void depalette(int w,
                      int h,
                      unsigned char* src,
                      TPixel* dest,
                      int bipp,
                      const unsigned char* plte,
                      const unsigned char* trns,
                      int trnsSize) {
    int x, y, c;
    unsigned char alpha;
    int mask, len;

    switch (bipp) {
        case 4:
            mask = 15;
            len = 1;
            break;
        case 2:
            mask = 3;
            len = 3;
            break;
        case 1:
            mask = 1;
            len = 7;
    }

    for (y = 0; y < h; y++) {
        src++;  // skip filter byte
        for (x = 0; x < w; x++) {
            if (bipp == 8) {
                c = *src++;
            } else {
                int pos = x & len;
                c = (src[0] >> ((len - pos) * bipp)) & mask;
                if (pos == len) {
                    src++;
                }
            }
            alpha = 255;
            if (c < trnsSize) {
                alpha = trns[c];
            }
            *dest++ = tigrRGBA(plte[c * 3 + 0], plte[c * 3 + 1], plte[c * 3 + 2], alpha);
        }
    }
}

#define FAIL()          \
    {                   \
        errno = EINVAL; \
        goto err;       \
    }
#define CHECK(X) \
    if (!(X))    \
    FAIL()

static int outsize(Tigr* bmp, int bipp) {
    return (rowBytes(bmp->w, bipp) + 1) * bmp->h;
}

static Tigr* tigrLoadPng(PNG* png) {
    const unsigned char *ihdr, *idat, *plte, *trns, *first;
    int trnsSize = 0;
    int depth, ctype, bipp;
    int datalen = 0;
    unsigned char *data = NULL, *out;
    Tigr* bmp = NULL;

    CHECK(memcmp(png->p, "\211PNG\r\n\032\n", 8) == 0);  // PNG signature
    png->p += 8;
    first = png->p;

    // Read IHDR
    ihdr = find(png, "IHDR", 13);
    CHECK(ihdr);
    depth = ihdr[8];
    ctype = ihdr[9];
    switch (ctype) {
        case 0:
            bipp = depth;
            break;  // greyscale
        case 2:
            bipp = 3 * depth;
            break;  // RGB
        case 3:
            bipp = depth;
            break;  // paletted
        case 4:
            bipp = 2 * depth;
            break;  // grey+alpha
        case 6:
            bipp = 4 * depth;
            break;  // RGBA
        default:
            FAIL();
    }

    // Allocate bitmap (+1 width to save room for stupid PNG filter bytes)
    bmp = tigrBitmap(get32(ihdr + 0) + 1, get32(ihdr + 4));
    CHECK(bmp);
    bmp->w--;

    // We support 8-bit color components and 1, 2, 4 and 8 bit palette formats.
    // No interlacing, or wacky filter types.
    CHECK((depth != 16) && ihdr[10] == 0 && ihdr[11] == 0 && ihdr[12] == 0);

    // Join IDAT chunks.
    for (idat = find(png, "IDAT", 0); idat; idat = find(png, "IDAT", 0)) {
        unsigned len = get32(idat - 8);
        data = (unsigned char*)realloc(data, datalen + len);
        if (!data)
            break;

        memcpy(data + datalen, idat, len);
        datalen += len;
    }

    // Find palette.
    png->p = first;
    plte = find(png, "PLTE", 0);

    // Find transparency info.
    png->p = first;
    trns = find(png, "tRNS", 0);
    if (trns) {
        trnsSize = get32(trns - 8);
    }

    CHECK(data && datalen >= 6);
    CHECK((data[0] & 0x0f) == 0x08     // compression method (RFC 1950)
          && (data[0] & 0xf0) <= 0x70  // window size
          && (data[1] & 0x20) == 0);   // preset dictionary present

    out = (unsigned char*)bmp->pix + outsize(bmp, 32) - outsize(bmp, bipp);
    CHECK(tigrInflate(out, outsize(bmp, bipp), data + 2, datalen - 6));
    CHECK(unfilter(bmp->w, bmp->h, bipp, out));

    if (ctype == 3) {
        CHECK(plte);
        depalette(bmp->w, bmp->h, out, bmp->pix, bipp, plte, trns, trnsSize);
    } else {
        CHECK(bipp % 8 == 0);
        convert(bipp / 8, bmp->w, bmp->h, out, bmp->pix, trns);
    }

    free(data);
    return bmp;

err:
    if (data)
        free(data);
    if (bmp)
        tigrFree(bmp);
    return NULL;
}

#undef CHECK
#undef FAIL

Tigr* tigrLoadImageMem(const void* data, int length) {
    PNG png;
    png.p = (unsigned char*)data;
    png.end = (unsigned char*)data + length;
    return tigrLoadPng(&png);
}

Tigr* tigrLoadImage(const char* fileName) {
    int len;
    void* data;
    Tigr* bmp;

    data = tigrReadFile(fileName, &len);
    if (!data)
        return NULL;

    bmp = tigrLoadImageMem(data, len);
    free(data);
    return bmp;
}
