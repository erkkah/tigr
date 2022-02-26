#include "tigr_internal.h"
#include "tigr_font.h"
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>

#ifdef _MSC_VER
#define vsnprintf _vsnprintf
#endif

TigrFont tigrStockFont;
TigrFont* tfont = &tigrStockFont;

// Converts 8-bit codepage entries into Unicode code points.
static int cp1252[] = {
    0x20ac, 0xfffd, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x0160, 0x2039, 0x0152,
    0xfffd, 0x017d, 0xfffd, 0xfffd, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x02dc, 0x2122,
    0x0161, 0x203a, 0x0153, 0xfffd, 0x017e, 0x0178, 0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6,
    0x00a7, 0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af, 0x00b0, 0x00b1, 0x00b2, 0x00b3,
    0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf, 0x00c0,
    0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd,
    0x00ce, 0x00cf, 0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7, 0x00d8, 0x00d9, 0x00da,
    0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df, 0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
    0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef, 0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4,
    0x00f5, 0x00f6, 0x00f7, 0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff,
};
static int border(Tigr* bmp, int x, int y) {
    TPixel top = tigrGet(bmp, 0, 0);
    TPixel c = tigrGet(bmp, x, y);
    return (c.r == top.r && c.g == top.g && c.b == top.b) || x >= bmp->w || y >= bmp->h;
}

static void scan(Tigr* bmp, int* x, int* y, int* rowh) {
    while (*y < bmp->h) {
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

int tigrLoadGlyphs(TigrFont* font, int codepage) {
    int i, x = 0, y = 0, w, h, rowh = 1;
    TigrGlyph* g;
    switch (codepage) {
        case 0:
            font->numGlyphs = 128 - 32;
            break;
        case 1252:
            font->numGlyphs = 256 - 32;
            break;
    }

    font->glyphs = (TigrGlyph*)calloc(font->numGlyphs, sizeof(TigrGlyph));
    for (i = 32; i < font->numGlyphs + 32; i++) {
        // Find the next glyph.
        scan(font->bitmap, &x, &y, &rowh);
        if (y >= font->bitmap->h) {
            errno = EINVAL;
            return 0;
        }

        // Scan the width and height
        w = h = 0;
        while (!border(font->bitmap, x + w, y))
            w++;
        while (!border(font->bitmap, x, y + h))
            h++;

        // Look up the Unicode code point.
        g = &font->glyphs[i - 32];
        if (i < 128)
            g->code = i;  // ASCII
        else if (codepage == 1252)
            g->code = cp1252[i - 128];
        else {
            errno = EINVAL;
            return 0;
        }

        g->x = x;
        g->y = y;
        g->w = w;
        g->h = h;
        x += w;
        if (h != font->glyphs[0].h) {
            errno = EINVAL;
            return 0;
        }

        if (h > rowh)
            rowh = h;
    }

    // Sort by code point.
    for (i = 1; i < font->numGlyphs; i++) {
        int j = i;
        TigrGlyph g = font->glyphs[i];
        while (j > 0 && font->glyphs[j - 1].code > g.code) {
            font->glyphs[j] = font->glyphs[j - 1];
            j--;
        }
        font->glyphs[j] = g;
    }

    return 1;
}

TigrFont* tigrLoadFont(Tigr* bitmap, int codepage) {
    TigrFont* font = (TigrFont*)calloc(1, sizeof(TigrFont));
    font->bitmap = bitmap;
    if (!tigrLoadGlyphs(font, codepage)) {
        tigrFreeFont(font);
        return NULL;
    }
    return font;
}

void tigrFreeFont(TigrFont* font) {
    tigrFree(font->bitmap);
    free(font->glyphs);
    free(font);
}

static TigrGlyph* get(TigrFont* font, int code) {
    unsigned lo = 0, hi = font->numGlyphs;
    while (lo < hi) {
        unsigned guess = (lo + hi) / 2;
        if (code < font->glyphs[guess].code)
            hi = guess;
        else
            lo = guess + 1;
    }

    if (lo == 0 || font->glyphs[lo - 1].code != code)
        return &font->glyphs['?' - 32];
    else
        return &font->glyphs[lo - 1];
}

void tigrSetupFont(TigrFont* font) {
    // Load the stock font if needed.
    if (font == tfont && !tfont->bitmap) {
        tfont->bitmap = tigrLoadImageMem(tigr_font, tigr_font_size);
        tigrLoadGlyphs(tfont, 1252);
    }
}

void tigrPrint(Tigr* dest, TigrFont* font, int x, int y, TPixel color, const char* text, ...) {
    char tmp[1024];
    TigrGlyph* g;
    va_list args;
    const char* p;
    int start = x, c;

    tigrSetupFont(font);

    // Expand the formatting string.
    va_start(args, text);
    vsnprintf(tmp, sizeof(tmp), text, args);
    tmp[sizeof(tmp) - 1] = 0;
    va_end(args);

    // Print each glyph.
    p = tmp;
    while (*p) {
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

int tigrTextWidth(TigrFont* font, const char* text) {
    int x = 0, w = 0, c;
    tigrSetupFont(font);

    while (*text) {
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

int tigrTextHeight(TigrFont* font, const char* text) {
    int rowh, h, c;
    tigrSetupFont(font);

    h = rowh = get(font, 0)->h;
    while (*text) {
        text = tigrDecodeUTF8(text, &c);
        if (c == '\n' && *text)
            h += rowh;
    }
    return h;
}
