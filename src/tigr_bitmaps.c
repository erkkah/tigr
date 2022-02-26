#include "tigr_internal.h"
#include <stdlib.h>
#include <string.h>

// Expands 0-255 into 0-256
#define EXPAND(X) ((X) + ((X) > 0))

#define CLIP0(CX, X, X2, W) \
    if (X < CX) {           \
        int D = CX - X;     \
        W -= D;             \
        X2 += D;            \
        X += D;             \
    }
#define CLIP1(X, DW, W) \
    if (X + W > DW)     \
        W = DW - X;
#define CLIP()                  \
    CLIP0(dst->cx, dx, sx, w);  \
    CLIP0(dst->cy, dy, sy, h);  \
    CLIP0(0, sx, dx, w);        \
    CLIP0(0, sy, dy, h);        \
    CLIP1(dx, dst->cx + cw, w); \
    CLIP1(dy, dst->cy + ch, h); \
    CLIP1(sx, src->w, w);       \
    CLIP1(sy, src->h, h);       \
    if (w <= 0 || h <= 0)       \
    return

Tigr* tigrBitmap2(int w, int h, int extra) {
    Tigr* tigr = (Tigr*)calloc(1, sizeof(Tigr) + extra);
    tigr->w = w;
    tigr->h = h;
    tigr->cw = -1;
    tigr->ch = -1;
    tigr->pix = (TPixel*)calloc(w * h, sizeof(TPixel));
    tigr->blitMode = TIGR_BLEND_ALPHA;
    return tigr;
}

Tigr* tigrBitmap(int w, int h) {
    return tigrBitmap2(w, h, 0);
}

void tigrResize(Tigr* bmp, int w, int h) {
    if (bmp->w == w && bmp->h == h) {
        return;
    }

    int y, cw, ch;
    TPixel* newpix = (TPixel*)calloc(w * h, sizeof(TPixel));
    cw = (w < bmp->w) ? w : bmp->w;
    ch = (h < bmp->h) ? h : bmp->h;

    // Copy any old data across.
    for (y = 0; y < ch; y++)
        memcpy(newpix + y * w, bmp->pix + y * bmp->w, cw * sizeof(TPixel));

    free(bmp->pix);
    bmp->pix = newpix;
    bmp->w = w;
    bmp->h = h;
}

int tigrCalcScale(int bmpW, int bmpH, int areaW, int areaH) {
    // We want it as big as possible in the window, but still
    // maintaining the correct aspect ratio, and always
    // having an integer pixel size.
    int scale = 0;
    for (;;) {
        scale++;
        if (bmpW * scale > areaW || bmpH * scale > areaH) {
            scale--;
            break;
        }
    }
    return (scale > 1) ? scale : 1;
}

int tigrEnforceScale(int scale, int flags) {
    if ((flags & TIGR_4X) && scale < 4)
        scale = 4;
    if ((flags & TIGR_3X) && scale < 3)
        scale = 3;
    if ((flags & TIGR_2X) && scale < 2)
        scale = 2;
    return scale;
}

void tigrPosition(Tigr* bmp, int scale, int windowW, int windowH, int out[4]) {
    // Center the image on screen at this scale.
    out[0] = (windowW - bmp->w * scale) / 2;
    out[1] = (windowH - bmp->h * scale) / 2;
    out[2] = out[0] + bmp->w * scale;
    out[3] = out[1] + bmp->h * scale;
}

void tigrClear(Tigr* bmp, TPixel color) {
    int count = bmp->w * bmp->h;
    int n;
    for (n = 0; n < count; n++)
        bmp->pix[n] = color;
}

void tigrFill(Tigr* bmp, int x, int y, int w, int h, TPixel color) {
    TPixel* td;
    int dt, i;

    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w > bmp->w) {
        w = bmp->w - x;
    }
    if (y + h > bmp->h) {
        h = bmp->h - y;
    }
    if (w <= 0 || h <= 0)
        return;

    td = &bmp->pix[y * bmp->w + x];
    dt = bmp->w;
    do {
        for (i = 0; i < w; i++)
            td[i] = color;
        td += dt;
    } while (--h);
}

void tigrLine(Tigr* bmp, int x0, int y0, int x1, int y1, TPixel color) {
    int sx, sy, dx, dy, err, e2;
    dx = abs(x1 - x0);
    dy = abs(y1 - y0);
    if (x0 < x1)
        sx = 1;
    else
        sx = -1;
    if (y0 < y1)
        sy = 1;
    else
        sy = -1;
    err = dx - dy;

    do {
        tigrPlot(bmp, x0, y0, color);
        e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    } while (x0 != x1 || y0 != y1);
}

void tigrFillRect(Tigr* bmp, int x, int y, int w, int h, TPixel color) {
    x += 1;
    y += 1;
    w -= 2;
    h -= 2;

    int cx = bmp->cx;
    int cy = bmp->cy;
    int cw = bmp->cw >= 0 ? bmp->cw : bmp->w;
    int ch = bmp->ch >= 0 ? bmp->ch : bmp->h;

    if (x < cx) {
        w += (x - cx);
        x = cx;
    }
    if (y < cy) {
        h += (y - cy);
        y = cy;
    }
    if (x + w > cx + cw) {
        w -= (x + w) - (cx + cw);
    }
    if (y + h > cy + ch) {
        h -= (y + h) - (cy + ch);
    }
    if (w <= 0 || h <= 0)
        return;

    TPixel* td = &bmp->pix[y * bmp->w + x];
    int dt = bmp->w;
    int xa = EXPAND(color.a);
    int a = xa * xa;

    do {
        for (int i = 0; i < w; i++) {
            td[i].r += (unsigned char)((color.r - td[i].r) * a >> 16);
            td[i].g += (unsigned char)((color.g - td[i].g) * a >> 16);
            td[i].b += (unsigned char)((color.b - td[i].b) * a >> 16);
            td[i].a += (bmp->blitMode) * (unsigned char)((color.a - td[i].a) * a >> 16);
        }
        td += dt;
    } while (--h);
}

void tigrRect(Tigr* bmp, int x, int y, int w, int h, TPixel color) {
    int x1, y1;
    if (w <= 0 || h <= 0)
        return;

    x1 = x + w - 1;
    y1 = y + h - 1;
    tigrLine(bmp, x, y, x1, y, color);
    tigrLine(bmp, x1, y, x1, y1, color);
    tigrLine(bmp, x1, y1, x, y1, color);
    tigrLine(bmp, x, y1, x, y, color);
}

void tigrFillCircle(Tigr* bmp, int x0, int y0, int r, TPixel color) {
    if (r <= 0) {
        return;
    }

    int E = 1 - r;
    int dx = 0;
    int dy = -2 * r;
    int x = 0;
    int y = r;

    tigrLine(bmp, x0 - r + 1, y0, x0 + r, y0, color);

    while (x < y - 1) {
        x++;

        if (E >= 0) {
            y--;
            dy += 2;
            E += dy;
            tigrLine(bmp, x0 - x + 1, y0 + y, x0 + x, y0 + y, color);
            tigrLine(bmp, x0 - x + 1, y0 - y, x0 + x, y0 - y, color);
        }

        dx += 2;
        E += dx + 1;

        if (x != y) {
            tigrLine(bmp, x0 - y + 1, y0 + x, x0 + y, y0 + x, color);
            tigrLine(bmp, x0 - y + 1, y0 - x, x0 + y, y0 - x, color);
        }
    }
}

void tigrCircle(Tigr* bmp, int x0, int y0, int r, TPixel color) {
    int E = 1 - r;
    int dx = 0;
    int dy = -2 * r;
    int x = 0;
    int y = r;

    tigrPlot(bmp, x0, y0 + r, color);
    tigrPlot(bmp, x0, y0 - r, color);
    tigrPlot(bmp, x0 + r, y0, color);
    tigrPlot(bmp, x0 - r, y0, color);

    while (x < y - 1) {
        x++;

        if (E >= 0) {
            y--;
            dy += 2;
            E += dy;
        }

        dx += 2;
        E += dx + 1;

        tigrPlot(bmp, x0 + x, y0 + y, color);
        tigrPlot(bmp, x0 - x, y0 + y, color);
        tigrPlot(bmp, x0 + x, y0 - y, color);
        tigrPlot(bmp, x0 - x, y0 - y, color);

        if (x != y) {
            tigrPlot(bmp, x0 + y, y0 + x, color);
            tigrPlot(bmp, x0 - y, y0 + x, color);
            tigrPlot(bmp, x0 + y, y0 - x, color);
            tigrPlot(bmp, x0 - y, y0 - x, color);
        }
    }
}

TPixel tigrGet(Tigr* bmp, int x, int y) {
    TPixel empty = { 0, 0, 0, 0 };
    if (x >= 0 && y >= 0 && x < bmp->w && y < bmp->h)
        return bmp->pix[y * bmp->w + x];
    return empty;
}

void tigrPlot(Tigr* bmp, int x, int y, TPixel pix) {
    int xa, i, a;

    int cx = bmp->cx;
    int cy = bmp->cy;
    int cw = bmp->cw >= 0 ? bmp->cw : bmp->w;
    int ch = bmp->ch >= 0 ? bmp->ch : bmp->h;

    if (x >= cx && y >= cy && x < cx + cw && y < cy + ch) {
        xa = EXPAND(pix.a);
        a = xa * xa;
        i = y * bmp->w + x;

        bmp->pix[i].r += (unsigned char)((pix.r - bmp->pix[i].r) * a >> 16);
        bmp->pix[i].g += (unsigned char)((pix.g - bmp->pix[i].g) * a >> 16);
        bmp->pix[i].b += (unsigned char)((pix.b - bmp->pix[i].b) * a >> 16);
        bmp->pix[i].a += (bmp->blitMode) * (unsigned char)((pix.a - bmp->pix[i].a) * a >> 16);
    }
}

void tigrClip(Tigr* bmp, int cx, int cy, int cw, int ch) {
    bmp->cx = cx;
    bmp->cy = cy;
    bmp->cw = cw;
    bmp->ch = ch;
}

void tigrBlit(Tigr* dst, Tigr* src, int dx, int dy, int sx, int sy, int w, int h) {
    int cw = dst->cw >= 0 ? dst->cw : dst->w;
    int ch = dst->ch >= 0 ? dst->ch : dst->h;

    CLIP();

    TPixel* ts = &src->pix[sy * src->w + sx];
    TPixel* td = &dst->pix[dy * dst->w + dx];
    int st = src->w;
    int dt = dst->w;
    do {
        memcpy(td, ts, w * sizeof(TPixel));
        ts += st;
        td += dt;
    } while (--h);
}

void tigrBlitTint(Tigr* dst, Tigr* src, int dx, int dy, int sx, int sy, int w, int h, TPixel tint) {
    int cw = dst->cw >= 0 ? dst->cw : dst->w;
    int ch = dst->ch >= 0 ? dst->ch : dst->h;

    CLIP();

    int xr = EXPAND(tint.r);
    int xg = EXPAND(tint.g);
    int xb = EXPAND(tint.b);
    int xa = EXPAND(tint.a);

    TPixel* ts = &src->pix[sy * src->w + sx];
    TPixel* td = &dst->pix[dy * dst->w + dx];
    int st = src->w;
    int dt = dst->w;
    do {
        for (int x = 0; x < w; x++) {
            unsigned r = (xr * ts[x].r) >> 8;
            unsigned g = (xg * ts[x].g) >> 8;
            unsigned b = (xb * ts[x].b) >> 8;
            unsigned a = xa * EXPAND(ts[x].a);
            td[x].r += (unsigned char)((r - td[x].r) * a >> 16);
            td[x].g += (unsigned char)((g - td[x].g) * a >> 16);
            td[x].b += (unsigned char)((b - td[x].b) * a >> 16);
            td[x].a += (dst->blitMode) * (unsigned char)((ts[x].a - td[x].a) * a >> 16);
        }
        ts += st;
        td += dt;
    } while (--h);
}

void tigrBlitAlpha(Tigr* dst, Tigr* src, int dx, int dy, int sx, int sy, int w, int h, float alpha) {
    alpha = (alpha < 0) ? 0 : (alpha > 1 ? 1 : alpha);
    tigrBlitTint(dst, src, dx, dy, sx, sy, w, h, tigrRGBA(0xff, 0xff, 0xff, (unsigned char)(alpha * 255)));
}

void tigrBlitMode(Tigr* dst, int mode) {
    dst->blitMode = mode;
}

#undef CLIP0
#undef CLIP1
#undef CLIP
