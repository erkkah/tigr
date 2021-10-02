#include "tigr_internal.h"

#ifdef __IOS__

#include <CoreGraphics/CoreGraphics.h>
#include <objc/message.h>
#include <objc/objc.h>
#include <objc/runtime.h>
#include <dispatch/dispatch.h>
#include <os/log.h>
#include <time.h>

#include "tigr_objc.h"

extern id UIApplication;
static int NSQualityOfServiceUserInteractive = 0x21;

typedef struct {
    TigrTouchPoint points[MAX_TOUCH_POINTS];
    int numPoints;
} InputState;


/// Global state
static struct {
    InputState inputState;
    id viewController;
    id context;
    id frameCondition;
    int screenW;
    int screenH;
    double scaleFactor;
    double timeSinceLastDraw;
    int closed;
    int renderReadFd;
    int mainWriteFd;
} gState = {
    .inputState = {
        .numPoints = 0,
    },
    .viewController = 0,
    .context = 0,
    .frameCondition = 0,
    .screenW = 0,
    .screenH = 0,
    .scaleFactor = 1,
    .timeSinceLastDraw = 0,
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

void writeToRenderThread(const TigrMessageData* message) {
    if (write(gState.mainWriteFd, message, sizeof(TigrMessageData)) != sizeof(TigrMessageData)) {
        os_log_error(OS_LOG_DEFAULT, "Failed to write message to render thread: %{public}s", strerror(errno));
    }
}

int readFromMainThread(TigrMessageData* message) {
    int result = read(gState.renderReadFd, message, sizeof(TigrMessageData));
    if (result == -1 && errno == EAGAIN) {
        return 0;
    }
    if (result != sizeof(TigrMessageData)) {
        os_log_error(OS_LOG_DEFAULT, "Failed to read message from main thread: %{public}s", strerror(errno));
        return 0;
    }
    return 1;
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

BOOL prefersStatusBarHidden(id self, SEL _sel) {
    return true;
}

enum RenderState {
    SWAPPED = 5150,
    RENDERED
};

BOOL didFinishLaunchingWithOptions(id self, SEL _sel, id application, id options) {
    id screen = objc_msgSend_id(class("UIScreen"), sel("mainScreen"));
    CGRect bounds = objc_msgSend_t(CGRect)(screen, sel("bounds"));
    CGSize size = bounds.size;

    id window = objc_alloc("UIWindow");
    window = objc_msgSend_t(id, CGRect)(window, sel("initWithFrame:"), bounds);

    Class ViewController = makeClass("TigrViewController", "GLKViewController");
    addMethod(ViewController, "viewWillTransitionToSize:withTransitionCoordinator:", viewWillTransitionToSize, "v@:{CGSize}@");
    addMethod(ViewController, "prefersStatusBarHidden", prefersStatusBarHidden, "c@:");
    id vc = objc_msgSend_t(id)((id)ViewController, sel("alloc"));
    vc = objc_msgSend_id(vc, sel("init"));
    gState.viewController = vc;
    objc_msgSend_t(void, int)(vc, sel("setPreferredFramesPerSecond:"), 60);
    int framesPerSecond = objc_msgSend_t(int)(vc, sel("framesPerSecond"));

    id context = objc_alloc("EAGLContext");
    static int kEAGLRenderingAPIOpenGLES3 = 3;
    context = objc_msgSend_t(id, int)(context, sel("initWithAPI:"), kEAGLRenderingAPIOpenGLES3);
    gState.context = context;

    id view = objc_alloc("GLKView");
    view = objc_msgSend_t(id, CGRect, id)(view, sel("initWithFrame:context:"), bounds, context);
    objc_msgSend_t(void, BOOL)(view, sel("setMultipleTouchEnabled:"), YES);
    objc_msgSend_t(void, id)(view, sel("setDelegate:"), self);
    objc_msgSend_t(void, id)(vc, sel("setView:"), view);
    objc_msgSend_t(void, id)(vc, sel("setDelegate:"), self);
    objc_msgSend_t(void, id)(window, sel("setRootViewController:"), vc);
    objc_msgSend_t(void)(window, sel("makeKeyAndVisible"));

    gState.scaleFactor = objc_msgSend_t(double)(view, sel("contentScaleFactor"));
    gState.screenW = size.width * gState.scaleFactor;
    gState.screenH = size.height * gState.scaleFactor;

    gState.frameCondition = objc_msgSend_t(id, int)(objc_alloc("NSConditionLock"), sel("initWithCondition:"), RENDERED);
    objc_msgSend_t(void, int)(gState.frameCondition, sel("lockWhenCondition:"), RENDERED);

    id renderThread = objc_msgSend_t(id, id, SEL, id)
        (objc_alloc("NSThread"), sel("initWithTarget:selector:object:"), self, sel("renderMain"), NULL);
    objc_msgSend_t(void, int)(renderThread, sel("setQualityOfService:"), NSQualityOfServiceUserInteractive);
    objc_msgSend_t(void, id)(renderThread, sel("setName:"), makeNSString("Tigr Render Thread"));
    objc_msgSend_void(renderThread, sel("start"));

    return YES;
}

void waitForFrame() {
    objc_msgSend_t(void, id)(class("EAGLContext"), sel("setCurrentContext:"), 0);
    objc_msgSend_t(void, int)(gState.frameCondition, sel("unlockWithCondition:"), RENDERED);
    objc_msgSend_t(void, int)(gState.frameCondition, sel("lockWhenCondition:"), SWAPPED);
    objc_msgSend_t(void, id)(class("EAGLContext"), sel("setCurrentContext:"), gState.context);
}

void drawInRect(id _self, SEL _sel, id view, CGRect rect) {
    gState.timeSinceLastDraw = objc_msgSend_t(double)(gState.viewController, sel("timeSinceLastDraw"));
    objc_msgSend_t(void, int)(gState.frameCondition, sel("unlockWithCondition:"), SWAPPED);
    objc_msgSend_t(void, int)(gState.frameCondition, sel("lockWhenCondition:"), RENDERED);
    objc_msgSend_t(void, id)(class("EAGLContext"), sel("setCurrentContext:"), gState.context);
}

extern void tigrMain();

void renderMain(id _self, SEL _sel) {
    objc_msgSend_t(void, int)(gState.frameCondition, sel("lockWhenCondition:"), SWAPPED);
    objc_msgSend_t(void, id)(class("EAGLContext"), sel("setCurrentContext:"), gState.context);
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
    id allTouches = objc_msgSend_t(id)(event, sel("allTouches"));
    id enumerator = objc_msgSend_t(id)(allTouches, sel("objectEnumerator"));
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
    }
    TigrMessageData message = {
        .message = SET_INPUT,
        .inputState = input,
    };
    writeToRenderThread(&message);
}

Class tigrAppDelegate() {
    static Class delegateClass = 0;
    if (delegateClass != 0) {
        return delegateClass;
    }

    id application = objc_msgSend_id(class("UIApplication"), sel("sharedApplication"));
    delegateClass = makeClass("TigrAppDelegate", "UIResponder");
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
        tigrError(0, "Failed to create message pipe");
    }
    int flags = fcntl(fds[0], F_GETFL, 0);
    fcntl(fds[0], F_SETFL, flags | O_NONBLOCK);

    gState.renderReadFd = fds[0];
    gState.mainWriteFd = fds[1];
    return delegateClass;
}

Tigr* tigrWindow(int w, int h, const char* title, int flags) {
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
    tigrGAPICreate(bmp);

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
    tigrGAPIPresent(bmp, gState.screenW, gState.screenH);
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

    os_log_error(OS_LOG_DEFAULT, "tigr fatal error: %{public}s\n", tmp);

    exit(1);
}

float tigrTime() {
    return (float)gState.timeSinceLastDraw;
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
