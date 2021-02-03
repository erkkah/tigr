#include "tigr_internal.h"

#ifdef __ANDROID__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <android/log.h>
#include <android/native_window.h>

#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "tigr", __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "tigr", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "tigr", __VA_ARGS__))

typedef struct {
    int touchX;
    int touchY;
    int pointers;
} InputState;

static ANativeWindow* window = 0;
static int windowInstance = 0;
static InputState inputState = { 0, 0, 0 };
static EGLDisplay display = EGL_NO_DISPLAY;
static EGLSurface surface = EGL_NO_SURFACE;
static EGLint screenW = 0;
static EGLint screenH = 0;
static EGLConfig config = 0;
static const EGLint contextAttribs[] = { EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 0, EGL_NONE };

static EGLConfig getGLConfig(EGLDisplay display) {
    EGLConfig config = 0;

    const EGLint attribs[] = { EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
                               EGL_NONE };
    EGLint numConfigs;

    eglChooseConfig(display, attribs, NULL, 0, &numConfigs);
    EGLConfig* supportedConfigs = (EGLConfig*)malloc(sizeof(EGLConfig) * numConfigs);
    eglChooseConfig(display, attribs, supportedConfigs, numConfigs, &numConfigs);

    int i = 0;
    for (; i < numConfigs; i++) {
        EGLConfig* cfg = supportedConfigs[i];
        EGLint r, g, b, d;
        if (eglGetConfigAttrib(display, cfg, EGL_RED_SIZE, &r) &&
            eglGetConfigAttrib(display, cfg, EGL_GREEN_SIZE, &g) &&
            eglGetConfigAttrib(display, cfg, EGL_BLUE_SIZE, &b) &&
            eglGetConfigAttrib(display, cfg, EGL_DEPTH_SIZE, &d) && r == 8 && g == 8 && b == 8 && d == 0) {
            config = supportedConfigs[i];
            break;
        }
    }

    if (i == numConfigs) {
        config = supportedConfigs[0];
    }

    if (config == NULL) {
        LOGE("Unable to initialize EGLConfig");
    }

    free(supportedConfigs);

    return config;
}

static void setupOpenGL() {
    assert(surface == EGL_NO_SURFACE);
    assert(window != 0);

    if (display == EGL_NO_DISPLAY) {
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        EGLBoolean status = eglInitialize(display, NULL, NULL);
        if (!status) {
            tigrError(NULL, "Failed to init EGL");
        }
        config = getGLConfig(display);
    }

    surface = eglCreateWindowSurface(display, config, window, NULL);
    eglQuerySurface(display, surface, EGL_WIDTH, &screenW);
    eglQuerySurface(display, surface, EGL_HEIGHT, &screenH);
    LOGD("Screen is %d x %d", screenW, screenH);
}

static void tearDownOpenGL() {
    if (display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (surface != EGL_NO_SURFACE) {
            eglDestroySurface(display, surface);
            surface = EGL_NO_SURFACE;
        }
    }
}

static int processInputEvent(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event);
        int32_t actionCode = action & AMOTION_EVENT_ACTION_MASK;

        inputState.touchX = AMotionEvent_getX(event, 0);
        inputState.touchY = AMotionEvent_getY(event, 0);

        if (actionCode == AMOTION_EVENT_ACTION_DOWN) {
            inputState.pointers |= 1;
        } else if (actionCode == AMOTION_EVENT_ACTION_UP) {
            inputState.pointers &= ~1;
        }
        return 1;
    }
    return 0;
}

static int handleEvent(AndroidEvent event, void* donePtr) {
    int* done = donePtr;

    switch (event.type) {
        case AE_WINDOW_CREATED:
            window = event.window;
            windowInstance++;
            setupOpenGL();
            break;

        case AE_WINDOW_DESTROYED:
            tearDownOpenGL();
            window = 0;
            break;

        case AE_INPUT:
            return processInputEvent(event.inputEvent);

        case AE_ACTIVITY_DESTROYED:
            eglTerminate(display);
            display = EGL_NO_DISPLAY;
            *done = 1;

        default:
            LOGE("Unhandled event type: %d", event.type);
            return 0;
    }

    return 1;
}

static int processEvents() {
    int done = 0;

    while (android_pollEvent(handleEvent, &done)) {
        if (done) {
            return 0;
        }
    }

    return 1;
}

static Tigr* refreshWindow(Tigr* bmp) {
    TigrInternal* win = tigrInternal(bmp);
    if (win->instance == windowInstance) {
        return bmp;
    }

    win->instance = windowInstance;

    int scale = 1;
    if (win->flags & TIGR_AUTO) {
        // Always use a 1:1 pixel size.
        scale = 1;
    } else {
        // See how big we can make it and still fit on-screen.
        scale = tigrCalcScale(bmp->w, bmp->h, screenW, screenH);
    }

    win->scale = tigrEnforceScale(scale, win->flags);

    return bmp;
}

Tigr* tigrWindow(int w, int h, const char* title, int flags) {
    while (window == NULL) {
        if (!processEvents()) {
            return NULL;
        }
    }

    EGLContext context = eglCreateContext(display, config, NULL, contextAttribs);

    int scale = 1;
    if (flags & TIGR_AUTO) {
        // Always use a 1:1 pixel size.
        scale = 1;
    } else {
        // See how big we can make it and still fit on-screen.
        scale = tigrCalcScale(w, h, screenW, screenH);
    }

    scale = tigrEnforceScale(scale, flags);

    Tigr* bmp = tigrBitmap2(w, h, sizeof(TigrInternal));
    bmp->handle = (void*)window;

    TigrInternal* win = tigrInternal(bmp);
    win->instance = windowInstance;
    win->context = context;

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
    win->widgets = 0;
    win->gl.gl_legacy = 0;

    tigrPosition(bmp, win->scale, bmp->w, bmp->h, win->pos);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOGE("Unable to eglMakeCurrent");
        return 0;
    }

    tigrGAPICreate(bmp);

    return bmp;
}

int tigrClosed(Tigr* bmp) {
    TigrInternal* win = tigrInternal(bmp);
    return win->closed;
}

int tigrGAPIBegin(Tigr* bmp) {
    TigrInternal* win = tigrInternal(bmp);
    if (eglMakeCurrent(display, surface, surface, win->context) == EGL_FALSE) {
        return -1;
    }
    return 0;
}

int tigrGAPIEnd(Tigr* bmp) {
    (void)bmp;
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    return 0;
}

int tigrKeyDown(Tigr* bmp, int key) {
    TigrInternal* win;
    assert(key < 256);
    win = tigrInternal(bmp);
    return win->keys[key] && !win->prev[key];
}

int tigrKeyHeld(Tigr* bmp, int key) {
    TigrInternal* win;
    assert(key < 256);
    win = tigrInternal(bmp);
    return win->keys[key];
}

int tigrReadChar(Tigr* bmp) {
    TigrInternal* win = tigrInternal(bmp);
    int c = win->lastChar;
    win->lastChar = 0;
    return c;
}

static void tigrUpdateModifiers(TigrInternal* win) {
    win->keys[TK_SHIFT] = win->keys[TK_LSHIFT] || win->keys[TK_RSHIFT];
    win->keys[TK_CONTROL] = win->keys[TK_LCONTROL] || win->keys[TK_RCONTROL];
    win->keys[TK_ALT] = win->keys[TK_LALT] || win->keys[TK_RALT];
}

void tigrUpdate(Tigr* bmp) {
    TigrInternal* win = tigrInternal(bmp);
    memcpy(win->prev, win->keys, 256);

    if (!processEvents()) {
        win->closed = 1;
        return;
    }

    if (window == 0) {
        return;
    }

    bmp = refreshWindow(bmp);

    win->mouseX = (inputState.touchX - win->pos[0]) / win->scale;
    win->mouseY = (inputState.touchY - win->pos[1]) / win->scale;
    win->mouseButtons = inputState.pointers;

    if (win->flags & TIGR_AUTO) {
        tigrResize(bmp, screenW / win->scale, screenH / win->scale);
    } else {
        win->scale = tigrEnforceScale(tigrCalcScale(bmp->w, bmp->h, screenW, screenH), win->flags);
    }

    tigrPosition(bmp, win->scale, screenW, screenH, win->pos);
    tigrGAPIBegin(bmp);
    tigrGAPIPresent(bmp, screenW, screenH);
    android_swap(display, surface);
    tigrGAPIEnd(bmp);
}

void tigrFree(Tigr* bmp) {
    if (bmp->handle) {
        EGLDisplay display = display;
        TigrInternal* win = tigrInternal(bmp);

        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (win->context != EGL_NO_CONTEXT) {
            // Win closed means app windows has closed, and the call would fail.
            if (!win->closed) {
                tigrGAPIDestroy(bmp);
            }
            eglDestroyContext(display, win->context);
        }

        win->context = EGL_NO_CONTEXT;
    }
    free(bmp->pix);
    free(bmp);
}

void tigrError(Tigr* bmp, const char* message, ...) {
    char tmp[1024];

    va_list args;
    va_start(args, message);
    vsnprintf(tmp, sizeof(tmp), message, args);
    tmp[sizeof(tmp) - 1] = 0;
    va_end(args);

    LOGE("tigr fatal error: %s\n", tmp);

    exit(1);
}

float tigrTime() {
    static double lastTime = 0;

    struct timeval tv;
    gettimeofday(&tv, NULL);

    double now = (double)tv.tv_sec + (tv.tv_usec / 1000000.0);
    double elapsed = lastTime == 0 ? 0 : now - lastTime;
    lastTime = now;

    return (float)elapsed;
}

void tigrMouse(Tigr* bmp, int* x, int* y, int* buttons) {
    TigrInternal* win = tigrInternal(bmp);
    if (x) {
        *x = win->mouseX;
    }
    if (y) {
        *y = win->mouseY;
    }
    if (buttons) {
        *buttons = win->mouseButtons;
    }
}

#endif  // __ANDROID__
