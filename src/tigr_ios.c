#include "tigr_internal.h"

#ifdef __IOS__

#include <CoreGraphics/CoreGraphics.h>
#include <objc/message.h>
#include <objc/objc.h>
#include <objc/runtime.h>
#include <dispatch/dispatch.h>
#include <os/log.h>
#include <time.h>

#if defined(__OBJC__) && __has_feature(objc_arc)
#error "Can't compile as objective-c code!"
#endif

#define objc_msgSend_id ((id(*)(id, SEL))objc_msgSend)
#define objc_msgSend_void ((void (*)(id, SEL))objc_msgSend)
#define objc_msgSend_void_id ((void (*)(id, SEL, id))objc_msgSend)
#define objc_msgSend_void_bool ((void (*)(id, SEL, BOOL))objc_msgSend)

#define objc_msgSendSuper_t(RET, ...) ((RET(*)(struct objc_super*, SEL, ##__VA_ARGS__))objc_msgSendSuper)
#define objc_msgSend_t(RET, ...) ((RET(*)(id, SEL, ##__VA_ARGS__))objc_msgSend)
#define sel(NAME) sel_registerName(NAME)
#define class(NAME) ((id)objc_getClass(NAME))
#define makeClass(NAME, SUPER) \
    objc_allocateClassPair((Class)objc_getClass(SUPER), NAME, 0)

// Check here to get the signature right: https://nshipster.com/type-encodings/
#define addMethod(CLASS, NAME, IMPL, SIGNATURE) \
    if (!class_addMethod(CLASS, sel(NAME), (IMP) (IMPL), (SIGNATURE))) assert(false)

#define objc_alloc(CLASS) objc_msgSend_id(class(CLASS), sel("alloc"))


extern id UIApplication;
static int NSQualityOfServiceUserInteractive = 0x21;

id makeNSString(const char* str) {
    return objc_msgSend_t(id, const char*)
        (class("NSString"), sel("stringWithUTF8String:"), str);
}

id joinNSStrings(id a, id b) {
    return objc_msgSend_t(id, id)
        (a, sel("stringByAppendingString:"), b);
}

const char* UTF8StringFromNSString(id a) {
    return objc_msgSend_t(const char*)(a, sel("UTF8String"));
}

typedef struct {
    TigrTouchPoint points[MAX_TOUCH_POINTS];
    int numPoints;
} InputState;


/// Global state
static struct {
    InputState inputState;
    id appDelegate;
    id context;
    id frameCondition;
    Tigr* currentWindow;
    int screenW;
    int screenH;
    double scaleFactor;
    double lastTime;
    int closed;
    int renderReadFd;
    int mainWriteFd;
} gState = {
    .inputState = {
        .numPoints = 0,
    },
    .appDelegate = 0,
    .context = 0,
    .frameCondition = 0,
    .currentWindow = 0,
    .screenW = 0,
    .screenH = 0,
    .scaleFactor = 1,
    .lastTime = 0,
    .closed = 0,
    .renderReadFd = 0,
    .mainWriteFd = 0,
};

typedef enum {
    SET_INPUT
} TigrMessage;

typedef struct TigrMessageData {
    TigrMessage message;
    InputState inputState;
} TigrMessageData;

static id autoreleasePool = NULL;

static void _pauseRendering(void* appDelegate) {
    id vc = objc_msgSend_id(appDelegate, sel("viewController"));
    objc_msgSend_void_bool((id) vc, sel("setPaused:"), YES);
}

static void _resumeRendering(void* appDelegate) {
    id vc = objc_msgSend_id(appDelegate, sel("viewController"));
    objc_msgSend_void_bool((id) vc, sel("setPaused:"), NO);
}

void callOnMainThread(void(*fn)(void*), void* data) {
    dispatch_sync_f(dispatch_get_main_queue(), NULL, fn);
}

void writeToRenderThread(const TigrMessageData* message) {
    if (write(gState.mainWriteFd, message, sizeof(TigrMessageData)) != sizeof(TigrMessageData)) {
        os_log_error(OS_LOG_DEFAULT, "Failed to write message to render thread: %s", strerror(errno));
    }
}

int readFromMainThread(TigrMessageData* message) {
    int result = read(gState.renderReadFd, message, sizeof(TigrMessageData));
    if (result == EAGAIN) {
        return 0;
    }
    if (result != sizeof(TigrMessageData)) {
        os_log_error(OS_LOG_DEFAULT, "Failed to read message from main thread: %s", strerror(errno));
        return 0;
    }
    return 1;
}

static void acquireContext() {
    callOnMainThread(_pauseRendering, gState.appDelegate);
    objc_msgSend_void_id(class("EAGLContext"), sel("setCurrentContext:"), gState.context);
}

static void releaseContext() {
    objc_msgSend_void_id(class("EAGLContext"), sel("setCurrentContext:"), NULL);
    callOnMainThread(_resumeRendering, gState.appDelegate);
}

void viewWillTransitionToSize(id self, SEL _sel, CGSize size, id transitionCoordinator) {
    // No animation, just set them
    gState.screenW = size.width * gState.scaleFactor;
    gState.screenH = size.height * gState.scaleFactor;
    struct objc_super super = {
        self,
        objc_getClass("GLKViewController"),
    };
    objc_msgSendSuper_t(void, CGSize, id)(&super, _sel, size, transitionCoordinator);
}

BOOL didFinishLaunchingWithOptions(id self, SEL _sel, id application, id options) {
    gState.appDelegate = self;

    os_log_info(OS_LOG_DEFAULT, "didFinishLaunchingWithOptions!");

    id screen = objc_msgSend_id(class("UIScreen"), sel("mainScreen"));
    CGRect bounds = objc_msgSend_t(CGRect)(screen, sel("bounds"));
    CGSize size = bounds.size;

    id window = objc_alloc("UIWindow");
    window = objc_msgSend_t(id, CGRect)(window, sel("initWithFrame:"), bounds);

    Class ViewController = makeClass("TigrViewController", "GLKViewController");
    addMethod(ViewController, "viewWillTransitionToSize:withTransitionCoordinator:", viewWillTransitionToSize, "v@:{CGSize}@");
    id vc = objc_msgSend_t(id)((id)ViewController, sel("alloc"));
    vc = objc_msgSend_id(vc, sel("init"));
    int framesPerSecond = objc_msgSend_t(int)(vc, sel("framesPerSecond"));
    os_log_info(OS_LOG_DEFAULT, "Frames per second: (%d)", framesPerSecond);

    id context = objc_alloc("EAGLContext");
    static int kEAGLRenderingAPIOpenGLES3 = 3;
    context = objc_msgSend_t(id, int)(context, sel("initWithAPI:"), kEAGLRenderingAPIOpenGLES3);
    gState.context = context;

    id view = objc_alloc("GLKView");
    view = objc_msgSend_t(id, CGRect, id)(view, sel("initWithFrame:context:"), bounds, context);
    objc_msgSend_t(void, BOOL)(view, sel("setMultipleTouchEnabled:"), YES);

    objc_msgSend_t(void, id)(vc, sel("setView:"), view);
    objc_msgSend_t(void, id)(view, sel("setDelegate:"), self);
    objc_msgSend_t(void, id)(window, sel("setRootViewController:"), vc);
    objc_msgSend_t(void)(window, sel("makeKeyAndVisible"));

    gState.scaleFactor = objc_msgSend_t(double)(view, sel("contentScaleFactor"));
    gState.screenW = size.width * gState.scaleFactor;
    gState.screenH = size.height * gState.scaleFactor;

    gState.frameCondition = objc_msgSend_t(id)(objc_alloc("NSCondition"), sel("init"));

    id renderThread = objc_msgSend_t(id, id, SEL, id)
        (objc_alloc("NSThread"), sel("initWithTarget:selector:object:"), self, sel("renderMain"), NULL);
    objc_msgSend_t(void, int)(renderThread, sel("setQualityOfService:"), NSQualityOfServiceUserInteractive);
    objc_msgSend_void(renderThread, sel("start"));

    return YES;
}

void waitForFrame() {
    objc_msgSend_void(gState.frameCondition, sel("wait"));
}

void drawInRect(id _self, SEL _sel, id view, CGRect rect) {
    if (gState.currentWindow != 0) {
        tigrGAPIPresent(gState.currentWindow, gState.screenW, gState.screenH);
    }
    objc_msgSend_void(gState.frameCondition, sel("signal"));
}

extern void tigrMain();

void renderMain(id _self, SEL _sel) {
    tigrMain();
}

enum {
    UITouchPhaseBegan,
    UITouchPhaseMoved,
    UITouchPhaseStationary,
    UITouchPhaseEnded,
    UITouchPhaseCancelled,
};

void touches(id self, SEL sel, id touches, id event) {
    id enumerator = objc_msgSend_t(id)(touches, sel("objectEnumerator"));
    id touch = 0;
    InputState input = {
        .numPoints = 0,
    };
    while ((touch = objc_msgSend_t(id)(enumerator, sel("nextObject")))) {
        CGPoint location = objc_msgSend_t(CGPoint, id)(touch, sel("locationInView:"), NULL);
        int phase = objc_msgSend_t(int)(touch, sel("phase"));
        switch (phase) {
            case UITouchPhaseBegan:
            case UITouchPhaseMoved:
            case UITouchPhaseStationary:
            input.points[input.numPoints].x = location.x * gState.scaleFactor;
            input.points[input.numPoints].y = location.y * gState.scaleFactor;
            input.numPoints++;
            break;
        }
        if (input.numPoints >= MAX_TOUCH_POINTS) {
            break;
        }
        os_log_info(OS_LOG_DEFAULT, "Touches: (%f, %f)", location.x, location.y);
    }
    TigrMessageData message = {
        .message = SET_INPUT,
        .inputState = input,
    };
    writeToRenderThread(&message);
}

void touchesEnded(id self, SEL sel, id touches, id event) {
    id enumerator = objc_msgSend_t(id)(touches, sel("objectEnumerator"));
    id touch = 0;
    while ((touch = objc_msgSend_t(id)(enumerator, sel("nextObject")))) {
        CGPoint location = objc_msgSend_t(CGPoint, id)(touch, sel("locationInView:"), NULL);
        os_log_info(OS_LOG_DEFAULT, "Touches ended/cancelled: (%f, %f)", location.x, location.y);
    }
}

void tigrInitIOS() {
    static bool inited = false;
    if (inited) {
        return;
    }
    inited = true;

    id application = objc_msgSend_id(class("UIApplication"), sel("sharedApplication"));
    Class delegateClass = makeClass("TigrAppDelegate", "UIResponder");
    addMethod(delegateClass, "application:didFinishLaunchingWithOptions:", didFinishLaunchingWithOptions, "c@:@@");
    addMethod(delegateClass, "touchesBegan:withEvent:", touches, "v@:@@");
    addMethod(delegateClass, "touchesMoved:withEvent:", touches, "v@:@@");
    addMethod(delegateClass, "touchesEnded:withEvent:", touches, "v@:@@");
    addMethod(delegateClass, "touchesCancelled:withEvent:", touches, "v@:@@");
    addMethod(delegateClass, "glkView:drawInRect:", drawInRect, "v@:@{CGRect}");
    addMethod(delegateClass, "renderMain", renderMain, "v@:");
    objc_registerClassPair(delegateClass);

    int fds[2];
    if (pipe(fds) != 0) {
        exit(42);
    }
    int flags = fcntl(fds[0], F_GETFL, 0);
    fcntl(fds[0], F_SETFL, flags | O_NONBLOCK);

    gState.renderReadFd = fds[0];
    gState.mainWriteFd = fds[1];
}

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
    bmp->handle = (void*)4711;
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
    acquireContext();
    tigrGAPICreate(bmp);
    releaseContext();

    return bmp;
}

void processEvents(TigrInternal* win) {
    TigrMessageData data;

    while (readFromMainThread(&data)) {
        switch(data.message) {
            case SET_INPUT:
                gState.inputState = data.inputState;
        }
    }
}

static int toWindowX(TigrInternal* win, int x) {
    return (x - win->pos[0]) / win->scale;
}

static int toWindowY(TigrInternal* win, int y) {
    return (y - win->pos[1]) / win->scale;
}

void tigrUpdate(Tigr* bmp) {
    TigrInternal* win = tigrInternal(bmp);

    processEvents(win);

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
    gState.currentWindow = bmp;
    waitForFrame();
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

extern void* _tigrReadFile(const char* fileName, int* length);

void* tigrReadFile(const char* fileName, int* length) {
    id mainBundle = objc_msgSend_id(class("NSBundle"), sel("mainBundle"));
    id resourcePath = objc_msgSend_id(mainBundle, sel("resourcePath"));
    resourcePath = joinNSStrings(resourcePath, makeNSString("/"));
    resourcePath = joinNSStrings(resourcePath, makeNSString(fileName));
    return _tigrReadFile(UTF8StringFromNSString(resourcePath), length);
}

#endif // __IOS__
