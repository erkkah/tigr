#include "tigr_internal.h"

#ifdef __IOS__

#include <os/log.h>

/// Global state
static struct {
    int screenW;
    int screenH;
    double lastTime;
    int closed;
} gState = {
    .screenW = 0,
    .screenH = 0,
    .lastTime = 0,
    .closed = 0,
};


void tigrIOSInit(int w, int h) {
    gState.screenW = w;
    gState.screenH = h;
}

extern void ios_acquire_context();
extern void ios_release_context();

Tigr* tigrWindow(int w, int h, const char* title, int flags) {    
    int scale = 1;
    if (flags & TIGR_AUTO) {
        // Always use a 1:1 pixel size.
        scale = 1;
    } else {
        // See how big we can make it and still fit on-screen.
        scale = tigrCalcScale(w, h, gState.screenW, gState.screenH);
        os_log_info(OS_LOG_DEFAULT, "Scale: %d", scale);
    }

    scale = tigrEnforceScale(scale, flags);
    Tigr* bmp = tigrBitmap2(w, h, sizeof(TigrInternal));
    bmp->handle = 4711;
    TigrInternal* win = tigrInternal(bmp);
    win->shown = 0;
    win->closed = 0;
    win->scale = scale;

    win->lastChar = 0;
    win->flags = flags;
    win->p1 = win->p2 = win->p3 = 0;
	win->p4 = 1;
    win->widgetsWanted = 0;
    win->widgetAlpha = 0;
    win->widgetsScale = 0;
    win->widgets = 0;
    win->gl.gl_legacy = 0;

    tigrPosition(bmp, win->scale, bmp->w, bmp->h, win->pos);
    ios_acquire_context();
    tigrGAPICreate(bmp);
    ios_release_context();

    return bmp;
}

extern void ios_swap();
Tigr* currentWindow = 0;

void tigrUpdate(Tigr* bmp) {
    TigrInternal* win = tigrInternal(bmp);

    if (win->flags & TIGR_AUTO) {
        tigrResize(bmp, gState.screenW / win->scale, gState.screenH / win->scale);
    } else {
        win->scale = tigrEnforceScale(tigrCalcScale(bmp->w, bmp->h, gState.screenW, gState.screenH), win->flags);
    }

    tigrPosition(bmp, win->scale, gState.screenW, gState.screenH, win->pos);
    currentWindow = bmp;
    ios_swap();
}

int tigrClosed(Tigr* bmp) {
    return 0;
}

void tigrError(Tigr* bmp, const char* message, ...) {
    char tmp[1024];

    va_list args;
    va_start(args, message);
    vsnprintf(tmp, sizeof(tmp), message, args);
    tmp[sizeof(tmp) - 1] = 0;
    va_end(args);

    os_log_error(OS_LOG_DEFAULT, "tigr fatal error: %s\n", tmp);

    //exit(1);
}

void tigrFree(Tigr* bmp) {
    if (bmp->handle) {
        TigrInternal* win = tigrInternal(bmp);
    }
    free(bmp->pix);
    free(bmp);
}

int tigrGAPIBegin(Tigr* bmp) {
    (void)bmp;
    return 0;
}

int tigrGAPIEnd(Tigr* bmp) {
    (void)bmp;
    return 0;
}

#endif // __IOS__
