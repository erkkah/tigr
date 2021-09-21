#include "tigr_internal.h"

#ifdef __IOS__

#include <CoreGraphics/CoreGraphics.h>
#include <objc/message.h>
#include <objc/objc.h>
#include <objc/runtime.h>
#include <dispatch/dispatch.h>
#include <os/log.h>

#if defined(__OBJC__) && __has_feature(objc_arc)
#error "Can't compile as objective-c code!"
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

#define objc_msgSend_id ((id(*)(id, SEL))objc_msgSend)
#define objc_msgSend_void ((void (*)(id, SEL))objc_msgSend)
#define objc_msgSend_void_id ((void (*)(id, SEL, id))objc_msgSend)
#define objc_msgSend_void_bool ((void (*)(id, SEL, BOOL))objc_msgSend)
#define objc_msgSend_id_const_char ((id(*)(id, SEL, const char*))objc_msgSend)

#define objc_msgSend_t(RET, ...) ((RET(*)(id, SEL, ##__VA_ARGS__))objc_msgSend)

#define objc_alloc(CLASS) objc_msgSend_id((id)objc_getClass(CLASS), sel_registerName("alloc"))

extern id UIApplication;

/// Global state
static struct {
    id appDelegate;
    id context;
    id frameCondition;
    Tigr* currentWindow;
    int screenW;
    int screenH;
    double lastTime;
    int closed;
} gState = {
    .appDelegate = 0,
    .context = 0,
    .frameCondition = 0,
    .currentWindow = 0,
    .screenW = 0,
    .screenH = 0,
    .lastTime = 0,
    .closed = 0,
};

static id autoreleasePool = NULL;

static void _pauseRendering(void* appDelegate) {
    id vc = objc_msgSend_id(appDelegate, sel_registerName("viewController"));
    objc_msgSend_void_bool((id) vc, sel_registerName("setPaused:"), YES);
}

static void _resumeRendering(void* appDelegate) {
    id vc = objc_msgSend_id(appDelegate, sel_registerName("viewController"));
    objc_msgSend_void_bool((id) vc, sel_registerName("setPaused:"), NO);
}

void callOnMainThread(void(*fn)(void*), void* data) {
    dispatch_sync_f(dispatch_get_main_queue(), NULL, fn);
}

static void acquireContext() {
    callOnMainThread(_pauseRendering, gState.appDelegate);
    objc_msgSend_void_id((id)objc_getClass("EAGLContext"), sel_registerName("setCurrentContext:"), gState.context);
}

static void releaseContext() {
    objc_msgSend_void_id((id)objc_getClass("EAGLContext"), sel_registerName("setCurrentContext:"), NULL);
    callOnMainThread(_resumeRendering, gState.appDelegate);
}

BOOL didFinishLaunchingWithOptions(id self, SEL _sel, id application, id options) {
    gState.appDelegate = self;

    os_log_info(OS_LOG_DEFAULT, "didFinishLaunchingWithOptions!");

    id screen = objc_msgSend_id((id)objc_getClass("UIScreen"), sel_registerName("mainScreen"));
    CGRect bounds = objc_msgSend_t(CGRect)(screen, sel_registerName("bounds"));
    CGSize size = bounds.size;

    id window = objc_alloc("UIWindow");
    window = objc_msgSend_t(id, CGRect)(window, sel_registerName("initWithFrame:"), bounds);

    id vc = objc_alloc("GLKViewController");
    vc = objc_msgSend_id(vc, sel_registerName("init"));

    id context = objc_alloc("EAGLContext");
    static int kEAGLRenderingAPIOpenGLES3 = 3;
    context = objc_msgSend_t(id, int)(context, sel_registerName("initWithAPI:"), kEAGLRenderingAPIOpenGLES3);
    gState.context = context;

    id view = objc_alloc("GLKView");
    view = objc_msgSend_t(id, CGRect, id)(view, sel_registerName("initWithFrame:context:"), bounds, context);

    objc_msgSend_t(void, id)(vc, sel_registerName("setView:"), view);
    objc_msgSend_t(void, id)(view, sel_registerName("setDelegate:"), self);
    objc_msgSend_t(void, id)(window, sel_registerName("setRootViewController:"), vc);
    objc_msgSend_t(void)(window, sel_registerName("makeKeyAndVisible"));

    double scaleFactor = objc_msgSend_t(double)(view, sel_registerName("contentScaleFactor"));
    gState.screenW = size.width * scaleFactor;
    gState.screenH = size.height * scaleFactor;
    os_log_info(OS_LOG_DEFAULT, "Size: %f, %f, %f", size.width, size.height, scaleFactor);

    gState.frameCondition = objc_msgSend_t(id)(objc_alloc("NSCondition"), sel_registerName("init"));

    id renderThread = objc_msgSend_t(id, id, SEL, id)
        (objc_alloc("NSThread"), sel_registerName("initWithTarget:selector:object:"), self, sel_registerName("renderMain"), NULL);
    objc_msgSend_void(renderThread, sel_registerName("start"));

    return YES;
}

void waitForFrame() {
    objc_msgSend_void(gState.frameCondition, sel_registerName("wait"));
}

void drawInRect(id _self, SEL _sel, id view, CGRect rect) {
    if (gState.currentWindow != 0) {
        tigrGAPIPresent(gState.currentWindow, gState.screenW, gState.screenH);
    }
    objc_msgSend_void(gState.frameCondition, sel_registerName("signal"));
}

extern void tigrMain();

void renderMain(id _self, SEL _sel) {
    tigrMain();
}

void tigrInitIOS() {
    static bool inited = false;
    if (inited) {
        return;
    }
    inited = true;

    id application = objc_msgSend_id((id)objc_getClass("UIApplication"), sel_registerName("sharedApplication"));
    Class delegateClass = objc_allocateClassPair((Class)objc_getClass("UIResponder"), "TigrAppDelegate", 0);
    objc_registerClassPair(delegateClass);
    //bool result = class_addProtocol(delegateClass, objc_getProtocol("UIApplicationDelegate"));
    //assert(result);
    //Class GLKViewDelegate = objc_getClass("GLKViewDelegate");
    //result = class_addProtocol(delegateClass, objc_getProtocol("GLKViewDelegate"));
    //assert(result);

    bool result = class_addMethod(delegateClass, sel_registerName("application:didFinishLaunchingWithOptions:"), (IMP) didFinishLaunchingWithOptions, "c@:@@");
    assert(result);
    result = class_addMethod(delegateClass, sel_registerName("glkView:drawInRect:"), (IMP) drawInRect, "v@:@{CGRect}");
    assert(result);
    result = class_addMethod(delegateClass, sel_registerName("renderMain"), (IMP) renderMain, "v@:");
    assert(result);


    // https://nshipster.com/type-encodings/
    // https://stackoverflow.com/questions/7819092/how-can-i-add-properties-to-an-object-at-runtime


    /*
    id delegate = objc_msgSend_id((id)delegateClass, sel_registerName("alloc"));
    delegate = objc_msgSend_id(delegate, sel_registerName("init"));
    objc_msgSend_void_id(application, sel_registerName("setDelegate:"), delegate);
    gState.appDelegate = delegate;
    */
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
    acquireContext();
    tigrGAPICreate(bmp);
    releaseContext();

    return bmp;
}

void tigrUpdate(Tigr* bmp) {
    TigrInternal* win = tigrInternal(bmp);

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
