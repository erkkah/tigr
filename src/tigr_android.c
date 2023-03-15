#ifndef TIGR_HEADLESS

#include "tigr_internal.h"

#ifdef __ANDROID__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <android/log.h>
#include <android/native_window.h>
#include <android/input.h>

#ifndef NDEBUG
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "tigr", __VA_ARGS__))
#else
#define LOGD(...) ((void)0)
#endif
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "tigr", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "tigr", __VA_ARGS__))

typedef struct {
    TigrTouchPoint points[MAX_TOUCH_POINTS];
    int numPoints;
} InputState;

/// Global state
static struct {
    ANativeWindow* window;
    InputState inputState;
    EGLDisplay display;
    EGLSurface surface;
    EGLint screenW;
    EGLint screenH;
    EGLConfig config;
    double lastTime;
    int closed;
} gState = {
    .window = 0,
    .inputState = {
        .numPoints = 0,
    },
    .display = EGL_NO_DISPLAY,
    .surface = EGL_NO_SURFACE,
    .screenW = 0,
    .screenH = 0,
    .config = 0,
    .lastTime = 0,
    .closed = 0,
};

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
        tigrError(NULL, "Unable to initialize EGLConfig");
    }

    free(supportedConfigs);

    return config;
}

/// Android interface, called from main thread

void tigr_android_create() {
    gState.closed = 0;
    gState.window = 0;
    gState.inputState.numPoints = 0;
    gState.surface = EGL_NO_SURFACE;
    gState.lastTime = 0;

    if (gState.display == EGL_NO_DISPLAY) {
        gState.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        EGLBoolean status = eglInitialize(gState.display, NULL, NULL);
        if (!status) {
            tigrError(NULL, "Failed to init EGL");
        }
        gState.config = getGLConfig(gState.display);
    }
}

void tigr_android_destroy() {
    eglTerminate(gState.display);
    gState.display = EGL_NO_DISPLAY;
}

/// Internals ///

static void logEglError() {
    int error = eglGetError();
    switch (error) {
        case EGL_BAD_DISPLAY:
            LOGE("EGL error: Bad display");
            break;
        case EGL_NOT_INITIALIZED:
            LOGE("EGL error: Not initialized");
            break;
        case EGL_BAD_NATIVE_WINDOW:
            LOGE("EGL error: Bad native window");
            break;
        case EGL_BAD_ALLOC:
            LOGE("EGL error: Bad alloc");
            break;
        case EGL_BAD_MATCH:
            LOGE("EGL error: Bad match");
            break;
        default:
            LOGE("EGL error: %d", error);
    }
}

static void setupOpenGL() {
    LOGD("setupOpenGL");
    assert(gState.surface == EGL_NO_SURFACE);
    assert(gState.window != 0);

    gState.surface = eglCreateWindowSurface(gState.display, gState.config, gState.window, NULL);
    if (gState.surface == EGL_NO_SURFACE) {
        logEglError();
    }
    assert(gState.surface != EGL_NO_SURFACE);
    eglQuerySurface(gState.display, gState.surface, EGL_WIDTH, &gState.screenW);
    eglQuerySurface(gState.display, gState.surface, EGL_HEIGHT, &gState.screenH);
    LOGD("Screen is %d x %d", gState.screenW, gState.screenH);
}

static void tearDownOpenGL() {
    LOGD("tearDownOpenGL");
    if (gState.display != EGL_NO_DISPLAY) {
        eglMakeCurrent(gState.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (gState.surface != EGL_NO_SURFACE) {
            LOGD("eglDestroySurface");
            if (!eglDestroySurface(gState.display, gState.surface)) {
                logEglError();
            }
            gState.surface = EGL_NO_SURFACE;
        }
    }
}

static int tigrKeyFromAndroidKey(int key) {
    switch (key) {
        case AKEYCODE_Q:
            return 'Q';
        case AKEYCODE_W:
            return 'W';
        case AKEYCODE_E:
            return 'E';
        case AKEYCODE_R:
            return 'R';
        case AKEYCODE_T:
            return 'T';
        case AKEYCODE_Y:
            return 'Y';
        case AKEYCODE_U:
            return 'U';
        case AKEYCODE_I:
            return 'I';
        case AKEYCODE_O:
            return 'O';
        case AKEYCODE_P:
            return 'P';

        case AKEYCODE_A:
            return 'A';
        case AKEYCODE_S:
            return 'S';
        case AKEYCODE_D:
            return 'D';
        case AKEYCODE_F:
            return 'F';
        case AKEYCODE_G:
            return 'G';
        case AKEYCODE_H:
            return 'H';
        case AKEYCODE_J:
            return 'J';
        case AKEYCODE_K:
            return 'K';
        case AKEYCODE_L:
            return 'L';

        case AKEYCODE_Z:
            return 'Z';
        case AKEYCODE_X:
            return 'X';
        case AKEYCODE_C:
            return 'C';
        case AKEYCODE_V:
            return 'V';
        case AKEYCODE_B:
            return 'B';
        case AKEYCODE_N:
            return 'N';
        case AKEYCODE_M:
            return 'M';

        case AKEYCODE_0:
            return '0';
        case AKEYCODE_1:
            return '1';
        case AKEYCODE_2:
            return '2';
        case AKEYCODE_3:
            return '3';
        case AKEYCODE_4:
            return '4';
        case AKEYCODE_5:
            return '5';
        case AKEYCODE_6:
            return '6';
        case AKEYCODE_7:
            return '7';
        case AKEYCODE_8:
            return '8';
        case AKEYCODE_9:
            return '9';

        case AKEYCODE_NUMPAD_0:
            return TK_PAD0;
        case AKEYCODE_NUMPAD_1:
            return TK_PAD1;
        case AKEYCODE_NUMPAD_2:
            return TK_PAD2;
        case AKEYCODE_NUMPAD_3:
            return TK_PAD3;
        case AKEYCODE_NUMPAD_4:
            return TK_PAD4;
        case AKEYCODE_NUMPAD_5:
            return TK_PAD5;
        case AKEYCODE_NUMPAD_6:
            return TK_PAD6;
        case AKEYCODE_NUMPAD_7:
            return TK_PAD7;
        case AKEYCODE_NUMPAD_8:
            return TK_PAD8;
        case AKEYCODE_NUMPAD_9:
            return TK_PAD9;

        case AKEYCODE_NUMPAD_MULTIPLY:
            return TK_PADMUL;
        case AKEYCODE_NUMPAD_DIVIDE:
            return TK_PADDIV;
        case AKEYCODE_NUMPAD_ADD:
            return TK_PADADD;
        case AKEYCODE_NUMPAD_SUBTRACT:
            return TK_PADSUB;
        case AKEYCODE_NUMPAD_ENTER:
            return TK_PADENTER;
        case AKEYCODE_NUMPAD_DOT:
            return TK_PADDOT;

        case AKEYCODE_F1:
            return TK_F1;
        case AKEYCODE_F2:
            return TK_F2;
        case AKEYCODE_F3:
            return TK_F3;
        case AKEYCODE_F4:
            return TK_F4;
        case AKEYCODE_F5:
            return TK_F5;
        case AKEYCODE_F6:
            return TK_F6;
        case AKEYCODE_F7:
            return TK_F7;
        case AKEYCODE_F8:
            return TK_F8;
        case AKEYCODE_F9:
            return TK_F9;
        case AKEYCODE_F10:
            return TK_F10;
        case AKEYCODE_F11:
            return TK_F11;
        case AKEYCODE_F12:
            return TK_F12;

        case AKEYCODE_SHIFT_LEFT:
            return TK_LSHIFT;
        case AKEYCODE_SHIFT_RIGHT:
            return TK_RSHIFT;
        case AKEYCODE_CTRL_LEFT:
            return TK_LCONTROL;
        case AKEYCODE_CTRL_RIGHT:
            return TK_RCONTROL;
        case AKEYCODE_ALT_LEFT:
            return TK_LALT;
        case AKEYCODE_ALT_RIGHT:
            return TK_RALT;
        case AKEYCODE_META_LEFT:
            return TK_LWIN;
        case AKEYCODE_META_RIGHT:
            return TK_RWIN;

        case AKEYCODE_DEL:
            return TK_BACKSPACE;
        case AKEYCODE_TAB:
            return TK_TAB;
        case AKEYCODE_ENTER:
            return TK_RETURN;
        case AKEYCODE_CAPS_LOCK:
            return TK_CAPSLOCK;
        case AKEYCODE_ESCAPE:
        case AKEYCODE_BACK:
            return TK_ESCAPE;
        case AKEYCODE_SPACE:
            return TK_SPACE;

        case AKEYCODE_PAGE_UP:
            return TK_PAGEUP;
        case AKEYCODE_PAGE_DOWN:
            return TK_PAGEDN;
        case AKEYCODE_MOVE_END:
            return TK_END;
        case AKEYCODE_MOVE_HOME:
            return TK_HOME;
        case AKEYCODE_DPAD_LEFT:
            return TK_LEFT;
        case AKEYCODE_DPAD_RIGHT:
            return TK_RIGHT;
        case AKEYCODE_DPAD_UP:
            return TK_UP;
        case AKEYCODE_DPAD_DOWN:
            return TK_DOWN;

        case AKEYCODE_INSERT:
            return TK_INSERT;
        case AKEYCODE_FORWARD_DEL:
            return TK_DELETE;
        case AKEYCODE_NUM_LOCK:
            return TK_NUMLOCK;
        case AKEYCODE_SCROLL_LOCK:
            return TK_SCROLL;

        case AKEYCODE_SEMICOLON:
            return TK_SEMICOLON;
        case AKEYCODE_EQUALS:
            return TK_EQUALS;
        case AKEYCODE_COMMA:
            return TK_COMMA;
        case AKEYCODE_MINUS:
            return TK_MINUS;
        case AKEYCODE_PERIOD:
            return TK_DOT;
        case AKEYCODE_SLASH:
            return TK_SLASH;
        case AKEYCODE_BACKSLASH:
            return TK_BACKSLASH;
        case AKEYCODE_GRAVE:
            return TK_BACKTICK;
        case AKEYCODE_APOSTROPHE:
            return TK_TICK;
        case AKEYCODE_LEFT_BRACKET:
            return TK_LSQUARE;
        case AKEYCODE_RIGHT_BRACKET:
            return TK_RSQUARE;

        default:
            return 0;
    }
}

static int processInputEvent(AInputEvent* event, TigrInternal* win) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event);
        int32_t actionCode = action & AMOTION_EVENT_ACTION_MASK;

        size_t touchPoints = AMotionEvent_getPointerCount(event);
        size_t releasedIndex = -1;
        if (actionCode == AMOTION_EVENT_ACTION_POINTER_UP) {
            releasedIndex =
                (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        }
        size_t targetPointCount = 0;

        for (size_t i = 0; i < touchPoints && i < MAX_TOUCH_POINTS; i++) {
            if (i == releasedIndex) {
                continue;
            }
            gState.inputState.points[targetPointCount].x = AMotionEvent_getX(event, i);
            gState.inputState.points[targetPointCount].y = AMotionEvent_getY(event, i);
            targetPointCount++;
        }

        if (actionCode == AMOTION_EVENT_ACTION_UP || actionCode == AMOTION_EVENT_ACTION_CANCEL) {
            gState.inputState.numPoints = 0;
        } else {
            gState.inputState.numPoints = targetPointCount;
        }
        return 1;
    } else if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
        if (!win) {
            return 1;
        }
        int32_t deviceID = AInputEvent_getDeviceId(event);
        // KeyCharacterMap#VIRTUAL_KEYBOARD == -1
        if (deviceID != -1) {
            return 1;
        }
        int32_t action = AKeyEvent_getAction(event);
        int32_t keyCode = AKeyEvent_getKeyCode(event);
        int key = tigrKeyFromAndroidKey(keyCode);
        // We pass the character in the scancode field from the Java side
        int32_t unicodeChar = AKeyEvent_getScanCode(event);

        if (action == AKEY_EVENT_ACTION_DOWN) {
            win->keys[key] = 1;
            win->lastChar = unicodeChar;
        } else if (action == AKEY_EVENT_ACTION_UP) {
            win->released[key] = 1;
        }
        return 1;
    }
    return 0;
}

static int handleEvent(AndroidEvent event, void* userData) {
    switch (event.type) {
        case AE_WINDOW_CREATED:
            gState.window = event.window;
            setupOpenGL();
            break;

        case AE_WINDOW_DESTROYED:
            tearDownOpenGL();
            gState.window = 0;
            break;

        case AE_INPUT:
            return processInputEvent(event.inputEvent, (TigrInternal*)userData);

        case AE_RESUME:
            gState.lastTime = event.time;
            break;

        case AE_CLOSE:
            gState.closed = 1;
            break;

        default:
            LOGE("Unhandled event type: %d", event.type);
            return 0;
    }

    return 1;
}

static int processEvents(TigrInternal* win) {
    if (gState.closed) {
        return 0;
    }

    while (android_pollEvent(handleEvent, win)) {
        if (gState.closed) {
            return 0;
        }
    }

    return 1;
}

static Tigr* refreshWindow(Tigr* bmp) {
    if (bmp->handle == gState.window) {
        return bmp;
    }

    bmp->handle = gState.window;
    if (gState.window == 0) {
        return 0;
    }

    TigrInternal* win = tigrInternal(bmp);

    int scale = 1;
    if (win->flags & TIGR_AUTO) {
        // Always use a 1:1 pixel size.
        scale = 1;
    } else {
        // See how big we can make it and still fit on-screen.
        scale = tigrCalcScale(bmp->w, bmp->h, gState.screenW, gState.screenH);
    }

    win->scale = tigrEnforceScale(scale, win->flags);

    return bmp;
}

/// TIGR interface implementation, called from render thread ///

Tigr* tigrWindow(int w, int h, const char* title, int flags) {
    while (gState.window == NULL) {
        if (!processEvents(0)) {
            return NULL;
        }
    }

    EGLContext context = eglCreateContext(gState.display, gState.config, NULL, contextAttribs);

    int scale = 1;
    if (flags & TIGR_AUTO) {
        // Always use a 1:1 pixel size.
        scale = 1;
    } else {
        // See how big we can make it and still fit on-screen.
        scale = tigrCalcScale(w, h, gState.screenW, gState.screenH);
    }

    scale = tigrEnforceScale(scale, flags);

    Tigr* bmp = tigrBitmap2(w, h, sizeof(TigrInternal));
    bmp->handle = (void*)gState.window;

    TigrInternal* win = tigrInternal(bmp);
    win->context = context;

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

    memset(win->keys, 0, 256);
    memset(win->prev, 0, 256);
    memset(win->released, 0, 256);

    tigrPosition(bmp, win->scale, bmp->w, bmp->h, win->pos);

    if (eglMakeCurrent(gState.display, gState.surface, gState.surface, context) == EGL_FALSE) {
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
    assert(gState.display != EGL_NO_DISPLAY);
    assert(gState.surface != EGL_NO_SURFACE);

    TigrInternal* win = tigrInternal(bmp);
    if (eglMakeCurrent(gState.display, gState.surface, gState.surface, win->context) == EGL_FALSE) {
        return -1;
    }
    return 0;
}

int tigrGAPIEnd(Tigr* bmp) {
    (void)bmp;
    eglMakeCurrent(gState.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
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

static int toWindowX(TigrInternal* win, int x) {
    return (x - win->pos[0]) / win->scale;
}

static int toWindowY(TigrInternal* win, int y) {
    return (y - win->pos[1]) / win->scale;
}

void tigrUpdate(Tigr* bmp) {
    TigrInternal* win = tigrInternal(bmp);
    memcpy(win->prev, win->keys, 256);
    for (int i = 0; i < 256; i++) {
        win->keys[i] ^= win->released[i];
        win->released[i] = 0;
    }

    if (!processEvents(win)) {
        win->closed = 1;
        return;
    }

    tigrUpdateModifiers(win);

    if (gState.window == 0) {
        return;
    }

    bmp = refreshWindow(bmp);
    if (bmp == 0) {
        return;
    }

    win->numTouchPoints = gState.inputState.numPoints;
    for (int i = 0; i < win->numTouchPoints; i++) {
        win->touchPoints[i].x = toWindowX(win, gState.inputState.points[i].x);
        win->touchPoints[i].y = toWindowY(win, gState.inputState.points[i].y);
    }

    win->mouseButtons = win->numTouchPoints;
    if (win->mouseButtons > 0) {
        win->mouseX = win->touchPoints[0].x;
        win->mouseY = win->touchPoints[0].y;
    }

    if (win->flags & TIGR_AUTO) {
        tigrResize(bmp, gState.screenW / win->scale, gState.screenH / win->scale);
    } else {
        win->scale = tigrEnforceScale(tigrCalcScale(bmp->w, bmp->h, gState.screenW, gState.screenH), win->flags);
    }

    tigrPosition(bmp, win->scale, gState.screenW, gState.screenH, win->pos);
    tigrGAPIBegin(bmp);
    tigrGAPIPresent(bmp, gState.screenW, gState.screenH);
    android_swap(gState.display, gState.surface);
    tigrGAPIEnd(bmp);
}

void tigrFree(Tigr* bmp) {
    if (bmp->handle) {
        TigrInternal* win = tigrInternal(bmp);

        eglMakeCurrent(gState.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (win->context != EGL_NO_CONTEXT) {
            // Win closed means app windows has closed, and the call would fail.
            if (!win->closed) {
                tigrGAPIDestroy(bmp);
            }
            eglDestroyContext(gState.display, win->context);
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
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    double now = (double)ts.tv_sec + (ts.tv_nsec / 1000000000.0);
    double elapsed = gState.lastTime == 0 ? 0 : now - gState.lastTime;
    gState.lastTime = now;

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

int tigrTouch(Tigr* bmp, TigrTouchPoint* points, int maxPoints) {
    TigrInternal* win = tigrInternal(bmp);
    for (int i = 0; i < maxPoints && i < win->numTouchPoints; i++) {
        points[i] = win->touchPoints[i];
    }
    return maxPoints < win->numTouchPoints ? maxPoints : win->numTouchPoints;
}

void* tigrReadFile(const char* fileName, int* length) {
    if (length != 0) {
        *length = 0;
    }

    void* asset = android_loadAsset(fileName, length);
    return asset;
}

#endif  // __ANDROID__
#endif // #ifndef TIGR_HEADLESS
