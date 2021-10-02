// this one is based on https://github.com/jimon/osx_app_in_plain_c

#include "tigr_internal.h"
#include "tigr_objc.h"

#if __MACOS__

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <mach/mach_time.h>
#include <objc/NSObjCRuntime.h>
#include <objc/message.h>
#include <objc/objc.h>
#include <objc/runtime.h>

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#else
// this is how they are defined originally
#include <CoreGraphics/CGBase.h>
#include <CoreGraphics/CGGeometry.h>
typedef CGPoint NSPoint;
typedef CGSize NSSize;
typedef CGRect NSRect;

enum {
    NSKeyDown = 10,
    NSKeyDownMask = 1 << NSKeyDown,
    NSKeyUp = 11,
    NSKeyUpMask = 1 << NSKeyUp,
    NSAllEventMask = NSUIntegerMax,
};

extern id NSApp;
extern id const NSDefaultRunLoopMode;

#define NSApplicationActivationPolicyRegular 0
#endif

bool terminated = false;

TigrInternal* _tigrInternalCocoa(id window) {
    if (!window)
        return NULL;

    id wdg = objc_msgSend_id(window, sel("delegate"));
    if (!wdg)
        return NULL;

    Tigr* bmp = 0;
    object_getInstanceVariable(wdg, "tigrHandle", (void**)&bmp);
    return bmp ? tigrInternal(bmp) : NULL;
}

// we gonna construct objective-c class by hand in runtime, so wow, so hacker!
NSUInteger applicationShouldTerminate(id self, SEL _sel, id sender) {
    terminated = true;
    return 0;
}

void windowWillClose(id self, SEL _sel, id notification) {
    NSUInteger value = true;
    object_setInstanceVariable(self, "closed", (void*)value);
}

void windowDidBecomeKey(id self, SEL _sel, id notification) {
    TigrInternal* win;
    Tigr* bmp = 0;
    object_getInstanceVariable(self, "tigrHandle", (void**)&bmp);
    win = bmp ? tigrInternal(bmp) : NULL;

    if (win) {
        memset(win->keys, 0, 256);
        memset(win->prev, 0, 256);
        win->lastChar = 0;
        win->mouseButtons = 0;
    }
}

void mouseEntered(id self, SEL _sel, id event) {
    id window = objc_msgSend_id(event, sel("window"));
    TigrInternal* win = _tigrInternalCocoa(window);
    win->mouseInView = 1;
    if (win->flags & TIGR_NOCURSOR) {
        objc_msgSend_id(class("NSCursor"), sel("hide"));
    }
}

void mouseExited(id self, SEL _sel, id event) {
    id window = objc_msgSend_id(event, sel("window"));
    TigrInternal* win = _tigrInternalCocoa(window);
    win->mouseInView = 0;
    if (win->flags & TIGR_NOCURSOR) {
        objc_msgSend_id(class("NSCursor"), sel("unhide"));
    }
}

bool _tigrCocoaIsWindowClosed(id window) {
    id wdg = objc_msgSend_id(window, sel("delegate"));
    if (!wdg)
        return false;
    NSUInteger value = 0;
    object_getInstanceVariable(wdg, "closed", (void**)&value);
    return value ? true : false;
}

static bool tigrOSXInited = false;
static id autoreleasePool = NULL;

#ifdef DEBUG
static void _showPools(const char* context) {
    fprintf(stderr, "NSAutoreleasePool@%s:\n", context);
    objc_msgSend(class("NSAutoreleasePool"), sel("showPools"));
}
#define showPools(x) _showPools((x))
#else
#define showPools(x)
#endif

static id pushPool() {
    id pool = objc_msgSend_id(class("NSAutoreleasePool"), sel("alloc"));
    return objc_msgSend_id(pool, sel("init"));
}

static void popPool(id pool) {
    objc_msgSend_void(pool, sel("drain"));
}

void _tigrCleanupOSX() {
    showPools("cleanup");
    popPool(autoreleasePool);
}

void tigrInitOSX() {
    if (tigrOSXInited)
        return;

    atexit(&_tigrCleanupOSX);

    autoreleasePool = pushPool();

    showPools("init start");

    objc_msgSend_id(class("NSApplication"), sel("sharedApplication"));
    objc_msgSend_t(void, NSInteger)(NSApp, sel("setActivationPolicy:"), NSApplicationActivationPolicyRegular);

    Class appDelegateClass = makeClass("AppDelegate", "NSObject");
    addMethod(appDelegateClass, "applicationShouldTerminate", applicationShouldTerminate, NSUIntegerEncoding "@:@");
    id dgAlloc = objc_msgSend_id((id)appDelegateClass, sel("alloc"));
    id dg = objc_msgSend_id(dgAlloc, sel("init"));

    objc_msgSend_void_id(NSApp, sel("setDelegate:"), dg);
    objc_msgSend_void(NSApp, sel("finishLaunching"));

    id menuBar = objc_alloc("NSMenu");
    menuBar = objc_msgSend_id(menuBar, sel("init"));

    id appMenuItem = objc_alloc("NSMenuItem");
    appMenuItem = objc_msgSend_id(appMenuItem, sel("init"));

    objc_msgSend_void_id(menuBar, sel("addItem:"), appMenuItem);
    objc_msgSend_t(id, id)(NSApp, sel("setMainMenu:"), menuBar);

    id processInfo = objc_msgSend_id(class("NSProcessInfo"), sel("processInfo"));
    id appName = objc_msgSend_id(processInfo, sel("processName"));

    id appMenu = objc_alloc("NSMenu");
    appMenu = objc_msgSend_t(id, id)(appMenu, sel("initWithTitle:"), appName);

    id quitTitlePrefixString =
        objc_msgSend_t(id, const char*)(class("NSString"), sel("stringWithUTF8String:"), "Quit ");
    id quitTitle = objc_msgSend_t(id, id)(
        quitTitlePrefixString, sel("stringByAppendingString:"), appName);

    id quitMenuItemKey =
        objc_msgSend_t(id, const char*)(class("NSString"), sel("stringWithUTF8String:"), "q");
    id quitMenuItem = objc_alloc("NSMenuItem");
    quitMenuItem = objc_msgSend_t(id, id, SEL, id)(
        quitMenuItem, sel("initWithTitle:action:keyEquivalent:"), quitTitle,
        sel("terminate:"), quitMenuItemKey);

    objc_msgSend_void_id(appMenu, sel("addItem:"), quitMenuItem);
    objc_msgSend_void_id(appMenuItem, sel("setSubmenu:"), appMenu);

    tigrOSXInited = true;

    showPools("init end");
}

void tigrError(Tigr* bmp, const char* message, ...) {
    char tmp[1024];

    va_list args;
    va_start(args, message);
    vsnprintf(tmp, sizeof(tmp), message, args);
    tmp[sizeof(tmp) - 1] = 0;
    va_end(args);

    CFStringRef header = CFStringCreateWithCString(NULL, "Error", kCFStringEncodingUTF8);
    CFStringRef msg = CFStringCreateWithCString(NULL, tmp, kCFStringEncodingUTF8);
    CFUserNotificationDisplayNotice(0.0, kCFUserNotificationStopAlertLevel, NULL, NULL, NULL, header, msg, NULL);
    CFRelease(header);
    CFRelease(msg);
    exit(1);
}

NSSize _tigrCocoaWindowSize(id window) {
    id contentView = objc_msgSend_id(window, sel("contentView"));
    NSRect rect = objc_msgSend_stret_t(NSRect)(contentView, sel("frame"));
    rect = objc_msgSend_stret_t(NSRect, NSRect)(contentView, sel("convertRectToBacking:"), rect);

    return rect.size;
}

enum {
    NSWindowStyleMaskTitled = 1 << 0,
    NSWindowStyleMaskClosable = 1 << 1,
    NSWindowStyleMaskMiniaturizable = 1 << 2,
    NSWindowStyleMaskResizable = 1 << 3,
    NSWindowStyleRegular = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
        NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable,
    NSWindowStyleMaskFullSizeContentView = 1 << 15
};

Tigr* tigrWindow(int w, int h, const char* title, int flags) {
    int scale;
    Tigr* bmp;
    TigrInternal* win;

    tigrInitOSX();

    NSUInteger windowStyleMask = NSWindowStyleRegular;

    if (flags & TIGR_AUTO) {
        // Always use a 1:1 pixel size, unless downscaled by tigrEnforceScale below.
        scale = 1;
    } else {
        // See how big we can make it and still fit on-screen.
        CGRect mainMonitor = CGDisplayBounds(CGMainDisplayID());
        int maxW = CGRectGetWidth(mainMonitor);
        int maxH = CGRectGetHeight(mainMonitor);
        NSRect screen = {{0, 0}, {maxW, maxH}};
        NSRect content = objc_msgSend_stret_t(NSRect, NSRect, NSUInteger)(
            class("NSWindow"), sel("contentRectForFrameRect:styleMask:"),
            screen, windowStyleMask
        );
        scale = tigrCalcScale(w, h, content.size.width, content.size.height);
    }

    scale = tigrEnforceScale(scale, flags);

    NSRect rect = { { 0, 0 }, { w * scale, h * scale } };
    id windowAlloc = objc_msgSend_id(class("NSWindow"), sel("alloc"));
    id window = ((id(*)(id, SEL, NSRect, NSUInteger, NSUInteger, BOOL))objc_msgSend)(
        windowAlloc, sel("initWithContentRect:styleMask:backing:defer:"), rect, windowStyleMask, 2, NO);

    if (flags & TIGR_FULLSCREEN) {
        objc_msgSend_void_id(window, sel("toggleFullScreen:"), window);
    }

    objc_msgSend_void_bool(window, sel("setReleasedWhenClosed:"), NO);

    Class WindowDelegateClass = objc_allocateClassPair((Class)objc_getClass("NSObject"), "WindowDelegate", 0);
    addIvar(WindowDelegateClass, "closed", sizeof(NSUInteger), NSUIntegerEncoding);
    addIvar(WindowDelegateClass, "tigrHandle", sizeof(void*), "ˆv");
    addMethod(WindowDelegateClass, "windowWillClose:", windowWillClose, "v@:@");
    addMethod(WindowDelegateClass, "windowDidBecomeKey:", windowDidBecomeKey, "v@:@");
    addMethod(WindowDelegateClass, "mouseEntered:", mouseEntered, "v@:@");
    addMethod(WindowDelegateClass, "mouseExited:", mouseExited, "v@:@");

    id wdgAlloc = objc_msgSend_id((id)WindowDelegateClass, sel("alloc"));
    id wdg = objc_msgSend_id(wdgAlloc, sel("init"));

    objc_msgSend_void_id(window, sel("setDelegate:"), wdg);

    id contentView = objc_msgSend_id(window, sel("contentView"));

    if (flags & TIGR_RETINA)
        objc_msgSend_void_bool(contentView, sel("setWantsBestResolutionOpenGLSurface:"), YES);

    NSPoint point = { 20, 20 };
    ((void (*)(id, SEL, NSPoint))objc_msgSend)(window, sel("cascadeTopLeftFromPoint:"), point);

    id titleString =
        objc_msgSend_t(id, const char*)(class("NSString"), sel("stringWithUTF8String:"), title);
    objc_msgSend_void_id(window, sel("setTitle:"), titleString);

    uint32_t glAttributes[] = { 8, 24,  //	NSOpenGLPFAColorSize, 24,
                                11, 8,  //	NSOpenGLPFAAlphaSize, 8,
                                5,      //	NSOpenGLPFADoubleBuffer,
                                73,     //	NSOpenGLPFAAccelerated,
                                // 72,			//	NSOpenGLPFANoRecovery,
                                // 55, 1,		//	NSOpenGLPFASampleBuffers, 1,
                                // 56, 4,		//	NSOpenGLPFASamples, 4,
                                99, 0x3200,  //	NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
                                0 };

    id pixelFormat = objc_alloc("NSOpenGLPixelFormat");
    pixelFormat = objc_msgSend_t(id, const uint32_t*)
        (pixelFormat, sel("initWithAttributes:"), glAttributes);
    objc_msgSend_void(pixelFormat, sel("autorelease"));

    id openGLContext = objc_alloc("NSOpenGLContext");
    openGLContext = objc_msgSend_t(id, id, id)
        (openGLContext, sel("initWithFormat:shareContext:"), pixelFormat, nil);

    objc_msgSend_void_id(openGLContext, sel("setView:"), contentView);
    objc_msgSend_void_id(window, sel("makeKeyAndOrderFront:"), window);
    objc_msgSend_void_bool(window, sel("setAcceptsMouseMovedEvents:"), YES);

    id blackColor = objc_msgSend_id(class("NSColor"), sel("blackColor"));
    objc_msgSend_void_id(window, sel("setBackgroundColor:"), blackColor);

    // TODO do we really need this?
    objc_msgSend_void_bool(NSApp, sel("activateIgnoringOtherApps:"), YES);

    // Wrap a bitmap around it.
    NSSize windowSize = _tigrCocoaWindowSize(window);
    bmp = tigrBitmap2(w, h, sizeof(TigrInternal));
    bmp->handle = window;

    // Set the handle
    object_setInstanceVariable(wdg, "tigrHandle", (void*)bmp);

    {
        #define NSTrackingMouseEnteredAndExited 1
        #define NSTrackingActiveInKeyWindow 0x20
        #define NSTrackingInVisibleRect 0x200

        int trackingFlags = NSTrackingMouseEnteredAndExited | NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect;
        id trackingArea = objc_msgSend_id(class("NSTrackingArea"), sel("alloc"));
        trackingArea = objc_msgSend_t(id, NSRect, int, id, id)(
            trackingArea, sel("initWithRect:options:owner:userInfo:"),
            rect, trackingFlags, wdg, 0
        );
        objc_msgSend_void_id(contentView, sel("addTrackingArea:"), trackingArea);
    }

    // Set up the Windows parts.
    win = tigrInternal(bmp);
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
    win->gl.glContext = openGLContext;
    win->mouseButtons = 0;
    win->mouseInView = 0;

    tigrPosition(bmp, win->scale, bmp->w, bmp->h, win->pos);

    objc_msgSend_void(openGLContext, sel("makeCurrentContext"));
    tigrGAPICreate(bmp);

    return bmp;
}

void tigrFree(Tigr* bmp) {
    if (bmp->handle) {
        TigrInternal* win = tigrInternal(bmp);
        tigrGAPIDestroy(bmp);

        id window = (id)bmp->handle;

        if (!_tigrCocoaIsWindowClosed(window) && !terminated) {
            objc_msgSend_void(window, sel("close"));
        }

        id wdg = objc_msgSend_id(window, sel("delegate"));
        objc_msgSend_void(wdg, sel("release"));
        objc_msgSend_void((id)win->gl.glContext, sel("release"));
        objc_msgSend_void(window, sel("release"));
    }
    free(bmp->pix);
    free(bmp);
}

uint8_t _tigrKeyFromOSX(uint16_t key) {
    // from Carbon HIToolbox/Events.h
    enum {
        kVK_ANSI_A = 0x00,
        kVK_ANSI_S = 0x01,
        kVK_ANSI_D = 0x02,
        kVK_ANSI_F = 0x03,
        kVK_ANSI_H = 0x04,
        kVK_ANSI_G = 0x05,
        kVK_ANSI_Z = 0x06,
        kVK_ANSI_X = 0x07,
        kVK_ANSI_C = 0x08,
        kVK_ANSI_V = 0x09,
        kVK_ANSI_B = 0x0B,
        kVK_ANSI_Q = 0x0C,
        kVK_ANSI_W = 0x0D,
        kVK_ANSI_E = 0x0E,
        kVK_ANSI_R = 0x0F,
        kVK_ANSI_Y = 0x10,
        kVK_ANSI_T = 0x11,
        kVK_ANSI_1 = 0x12,
        kVK_ANSI_2 = 0x13,
        kVK_ANSI_3 = 0x14,
        kVK_ANSI_4 = 0x15,
        kVK_ANSI_6 = 0x16,
        kVK_ANSI_5 = 0x17,
        kVK_ANSI_Equal = 0x18,
        kVK_ANSI_9 = 0x19,
        kVK_ANSI_7 = 0x1A,
        kVK_ANSI_Minus = 0x1B,
        kVK_ANSI_8 = 0x1C,
        kVK_ANSI_0 = 0x1D,
        kVK_ANSI_RightBracket = 0x1E,
        kVK_ANSI_O = 0x1F,
        kVK_ANSI_U = 0x20,
        kVK_ANSI_LeftBracket = 0x21,
        kVK_ANSI_I = 0x22,
        kVK_ANSI_P = 0x23,
        kVK_ANSI_L = 0x25,
        kVK_ANSI_J = 0x26,
        kVK_ANSI_Quote = 0x27,
        kVK_ANSI_K = 0x28,
        kVK_ANSI_Semicolon = 0x29,
        kVK_ANSI_Backslash = 0x2A,
        kVK_ANSI_Comma = 0x2B,
        kVK_ANSI_Slash = 0x2C,
        kVK_ANSI_N = 0x2D,
        kVK_ANSI_M = 0x2E,
        kVK_ANSI_Period = 0x2F,
        kVK_ANSI_Grave = 0x32,
        kVK_ANSI_KeypadDecimal = 0x41,
        kVK_ANSI_KeypadMultiply = 0x43,
        kVK_ANSI_KeypadPlus = 0x45,
        kVK_ANSI_KeypadClear = 0x47,
        kVK_ANSI_KeypadDivide = 0x4B,
        kVK_ANSI_KeypadEnter = 0x4C,
        kVK_ANSI_KeypadMinus = 0x4E,
        kVK_ANSI_KeypadEquals = 0x51,
        kVK_ANSI_Keypad0 = 0x52,
        kVK_ANSI_Keypad1 = 0x53,
        kVK_ANSI_Keypad2 = 0x54,
        kVK_ANSI_Keypad3 = 0x55,
        kVK_ANSI_Keypad4 = 0x56,
        kVK_ANSI_Keypad5 = 0x57,
        kVK_ANSI_Keypad6 = 0x58,
        kVK_ANSI_Keypad7 = 0x59,
        kVK_ANSI_Keypad8 = 0x5B,
        kVK_ANSI_Keypad9 = 0x5C,
        kVK_Return = 0x24,
        kVK_Tab = 0x30,
        kVK_Space = 0x31,
        kVK_Delete = 0x33,
        kVK_Escape = 0x35,
        kVK_Command = 0x37,
        kVK_Shift = 0x38,
        kVK_CapsLock = 0x39,
        kVK_Option = 0x3A,
        kVK_Control = 0x3B,
        kVK_RightShift = 0x3C,
        kVK_RightOption = 0x3D,
        kVK_RightControl = 0x3E,
        kVK_Function = 0x3F,
        kVK_F17 = 0x40,
        kVK_VolumeUp = 0x48,
        kVK_VolumeDown = 0x49,
        kVK_Mute = 0x4A,
        kVK_F18 = 0x4F,
        kVK_F19 = 0x50,
        kVK_F20 = 0x5A,
        kVK_F5 = 0x60,
        kVK_F6 = 0x61,
        kVK_F7 = 0x62,
        kVK_F3 = 0x63,
        kVK_F8 = 0x64,
        kVK_F9 = 0x65,
        kVK_F11 = 0x67,
        kVK_F13 = 0x69,
        kVK_F16 = 0x6A,
        kVK_F14 = 0x6B,
        kVK_F10 = 0x6D,
        kVK_F12 = 0x6F,
        kVK_F15 = 0x71,
        kVK_Help = 0x72,
        kVK_Home = 0x73,
        kVK_PageUp = 0x74,
        kVK_ForwardDelete = 0x75,
        kVK_F4 = 0x76,
        kVK_End = 0x77,
        kVK_F2 = 0x78,
        kVK_PageDown = 0x79,
        kVK_F1 = 0x7A,
        kVK_LeftArrow = 0x7B,
        kVK_RightArrow = 0x7C,
        kVK_DownArrow = 0x7D,
        kVK_UpArrow = 0x7E
    };

    switch (key) {
        case kVK_ANSI_Q:
            return 'Q';
        case kVK_ANSI_W:
            return 'W';
        case kVK_ANSI_E:
            return 'E';
        case kVK_ANSI_R:
            return 'R';
        case kVK_ANSI_T:
            return 'T';
        case kVK_ANSI_Y:
            return 'Y';
        case kVK_ANSI_U:
            return 'U';
        case kVK_ANSI_I:
            return 'I';
        case kVK_ANSI_O:
            return 'O';
        case kVK_ANSI_P:
            return 'P';
        case kVK_ANSI_A:
            return 'A';
        case kVK_ANSI_S:
            return 'S';
        case kVK_ANSI_D:
            return 'D';
        case kVK_ANSI_F:
            return 'F';
        case kVK_ANSI_G:
            return 'G';
        case kVK_ANSI_H:
            return 'H';
        case kVK_ANSI_J:
            return 'J';
        case kVK_ANSI_K:
            return 'K';
        case kVK_ANSI_L:
            return 'L';
        case kVK_ANSI_Z:
            return 'Z';
        case kVK_ANSI_X:
            return 'X';
        case kVK_ANSI_C:
            return 'C';
        case kVK_ANSI_V:
            return 'V';
        case kVK_ANSI_B:
            return 'B';
        case kVK_ANSI_N:
            return 'N';
        case kVK_ANSI_M:
            return 'M';
        case kVK_ANSI_0:
            return '0';
        case kVK_ANSI_1:
            return '1';
        case kVK_ANSI_2:
            return '2';
        case kVK_ANSI_3:
            return '3';
        case kVK_ANSI_4:
            return '4';
        case kVK_ANSI_5:
            return '5';
        case kVK_ANSI_6:
            return '6';
        case kVK_ANSI_7:
            return '7';
        case kVK_ANSI_8:
            return '8';
        case kVK_ANSI_9:
            return '9';
        case kVK_ANSI_Keypad0:
            return TK_PAD0;
        case kVK_ANSI_Keypad1:
            return TK_PAD1;
        case kVK_ANSI_Keypad2:
            return TK_PAD2;
        case kVK_ANSI_Keypad3:
            return TK_PAD3;
        case kVK_ANSI_Keypad4:
            return TK_PAD4;
        case kVK_ANSI_Keypad5:
            return TK_PAD5;
        case kVK_ANSI_Keypad6:
            return TK_PAD6;
        case kVK_ANSI_Keypad7:
            return TK_PAD7;
        case kVK_ANSI_Keypad8:
            return TK_PAD8;
        case kVK_ANSI_Keypad9:
            return TK_PAD9;
        case kVK_ANSI_KeypadMultiply:
            return TK_PADMUL;
        case kVK_ANSI_KeypadPlus:
            return TK_PADADD;
        case kVK_ANSI_KeypadEnter:
            return TK_PADENTER;
        case kVK_ANSI_KeypadMinus:
            return TK_PADSUB;
        case kVK_ANSI_KeypadDecimal:
            return TK_PADDOT;
        case kVK_ANSI_KeypadDivide:
            return TK_PADDIV;
        case kVK_F1:
            return TK_F1;
        case kVK_F2:
            return TK_F2;
        case kVK_F3:
            return TK_F3;
        case kVK_F4:
            return TK_F4;
        case kVK_F5:
            return TK_F5;
        case kVK_F6:
            return TK_F6;
        case kVK_F7:
            return TK_F7;
        case kVK_F8:
            return TK_F8;
        case kVK_F9:
            return TK_F9;
        case kVK_F10:
            return TK_F10;
        case kVK_F11:
            return TK_F11;
        case kVK_F12:
            return TK_F12;
        case kVK_Shift:
            return TK_LSHIFT;
        case kVK_Control:
            return TK_LCONTROL;
        case kVK_Option:
            return TK_LALT;
        case kVK_CapsLock:
            return TK_CAPSLOCK;
        case kVK_Command:
            return TK_LWIN;
        case kVK_Command - 1:
            return TK_RWIN;
        case kVK_RightShift:
            return TK_RSHIFT;
        case kVK_RightControl:
            return TK_RCONTROL;
        case kVK_RightOption:
            return TK_RALT;
        case kVK_Delete:
            return TK_BACKSPACE;
        case kVK_Tab:
            return TK_TAB;
        case kVK_Return:
            return TK_RETURN;
        case kVK_Escape:
            return TK_ESCAPE;
        case kVK_Space:
            return TK_SPACE;
        case kVK_PageUp:
            return TK_PAGEUP;
        case kVK_PageDown:
            return TK_PAGEDN;
        case kVK_End:
            return TK_END;
        case kVK_Home:
            return TK_HOME;
        case kVK_LeftArrow:
            return TK_LEFT;
        case kVK_UpArrow:
            return TK_UP;
        case kVK_RightArrow:
            return TK_RIGHT;
        case kVK_DownArrow:
            return TK_DOWN;
        case kVK_Help:
            return TK_INSERT;
        case kVK_ForwardDelete:
            return TK_DELETE;
        case kVK_F14:
            return TK_SCROLL;
        case kVK_F15:
            return TK_PAUSE;
        case kVK_ANSI_KeypadClear:
            return TK_NUMLOCK;
        case kVK_ANSI_Semicolon:
            return TK_SEMICOLON;
        case kVK_ANSI_Equal:
            return TK_EQUALS;
        case kVK_ANSI_Comma:
            return TK_COMMA;
        case kVK_ANSI_Minus:
            return TK_MINUS;
        case kVK_ANSI_Slash:
            return TK_SLASH;
        case kVK_ANSI_Backslash:
            return TK_BACKSLASH;
        case kVK_ANSI_Grave:
            return TK_BACKTICK;
        case kVK_ANSI_Quote:
            return TK_TICK;
        case kVK_ANSI_LeftBracket:
            return TK_LSQUARE;
        case kVK_ANSI_RightBracket:
            return TK_RSQUARE;
        case kVK_ANSI_Period:
            return TK_DOT;
        default:
            return 0;
    }
}

void _tigrOnCocoaEvent(id event, id window) {
    if (!event)
        return;

    TigrInternal* win = _tigrInternalCocoa(window);
    if (!win)  // just pipe the event
    {
        objc_msgSend_void_id(NSApp, sel("sendEvent:"), event);
        return;
    }

    NSUInteger eventType =  objc_msgSend_t(NSUInteger)(event, sel("type"));
    switch (eventType) {
        case 1:  // NSLeftMouseDown
            if (win->mouseInView) {
                win->mouseButtons |= 1;
            }
            break;
        case 2:  // NSLeftMouseUp
            win->mouseButtons &= ~1;
            break;
        case 3:  // NSRightMouseDown
            if (win->mouseInView) {
                win->mouseButtons |= 2;
            }
            break;
        case 4:  // NSRightMouseUp
            win->mouseButtons &= ~2;
            break;
        case 25:  // NSOtherMouseDown
        {
            // number == 2 is a middle button
            NSInteger number = objc_msgSend_t(NSUInteger)(event, sel("buttonNumber"));
            if (number == 2 && win->mouseInView) {
                win->mouseButtons |= 4;
            }
            break;
        }
        case 26:  // NSOtherMouseUp
        {
            NSInteger number = objc_msgSend_t(NSInteger)(event, sel("buttonNumber"));
            if (number == 2)
                win->mouseButtons &= ~4;
            break;
        }
        case 12:  // NSFlagsChanged
        {
            NSUInteger modifiers = objc_msgSend_t(NSUInteger)(event, sel("modifierFlags"));

            // based on NSEventModifierFlags and
            // NSDeviceIndependentModifierFlagsMask
            struct {
                union {
                    struct {
                        uint8_t alpha_shift : 1;
                        uint8_t shift : 1;
                        uint8_t control : 1;
                        uint8_t alternate : 1;
                        uint8_t command : 1;
                        uint8_t numeric_pad : 1;
                        uint8_t help : 1;
                        uint8_t function : 1;
                    };
                    uint8_t mask;
                };
            } keys;

            keys.mask = (modifiers & 0xffff0000UL) >> 16;

            // TODO L,R variation of keys?
            win->keys[TK_CONTROL] = keys.alpha_shift;
            win->keys[TK_SHIFT] = keys.shift;
            win->keys[TK_CONTROL] = keys.control;
            win->keys[TK_ALT] = keys.alternate;
            win->keys[TK_LWIN] = keys.command;
            win->keys[TK_RWIN] = keys.command;
            break;
        }
        case 10:  // NSKeyDown
        {
            id inputText = objc_msgSend_id(event, sel("characters"));
            const char* inputTextUTF8 = objc_msgSend_t(const char*)(inputText, sel("UTF8String"));

            tigrDecodeUTF8(inputTextUTF8, &win->lastChar);

            uint16_t keyCode = objc_msgSend_t(unsigned short)(event, sel("keyCode"));
            win->keys[_tigrKeyFromOSX(keyCode)] = 1;

            // Pass through cmd+key
            if (win->keys[TK_LWIN]) {
                break;
            }
            return;
        }
        case 11:  // NSKeyUp
        {
            uint16_t keyCode = objc_msgSend_t(unsigned short)(event, sel("keyCode"));
            win->keys[_tigrKeyFromOSX(keyCode)] = 0;
            return;
        }
        default:
            break;
    }

    objc_msgSend_void_id(NSApp, sel("sendEvent:"), event);
}

void tigrUpdate(Tigr* bmp) {
    popPool(autoreleasePool);
    autoreleasePool = pushPool();

    TigrInternal* win;
    id openGLContext;
    id window;
    win = tigrInternal(bmp);
    window = (id)bmp->handle;
    openGLContext = (id)win->gl.glContext;

    if (terminated || _tigrCocoaIsWindowClosed(window)) {
        return;
    }

    id keyWindow = objc_msgSend_id(NSApp, sel("keyWindow"));
    unsigned long long eventMask = NSAllEventMask;

    if (keyWindow == window) {
        memcpy(win->prev, win->keys, 256);
    } else {
        eventMask &= ~(NSKeyDownMask | NSKeyUpMask);
    }

    id distantPast = objc_msgSend_id(class("NSDate"), sel("distantPast"));
    id event = 0;
    do {
        event = objc_msgSend_t(id, NSUInteger, id, id, BOOL)(
            NSApp, sel("nextEventMatchingMask:untilDate:inMode:dequeue:"), eventMask, distantPast,
            NSDefaultRunLoopMode, YES
        );

        if (event != 0) {
            _tigrOnCocoaEvent(event, window);
        }
    } while (event != 0);

    // do runloop stuff
    objc_msgSend_void(NSApp, sel("updateWindows"));
    objc_msgSend_void(openGLContext, sel("update"));
    tigrGAPIBegin(bmp);

    NSSize windowSize = _tigrCocoaWindowSize(window);

    if (win->flags & TIGR_AUTO)
        tigrResize(bmp, windowSize.width / win->scale, windowSize.height / win->scale);
    else
        win->scale = tigrEnforceScale(tigrCalcScale(bmp->w, bmp->h, windowSize.width, windowSize.height), win->flags);

    tigrPosition(bmp, win->scale, windowSize.width, windowSize.height, win->pos);
    tigrGAPIPresent(bmp, windowSize.width, windowSize.height);
    objc_msgSend_void(openGLContext, sel("flushBuffer"));
    tigrGAPIEnd(bmp);
}

int tigrGAPIBegin(Tigr* bmp) {
    TigrInternal* win = tigrInternal(bmp);
    objc_msgSend_void((id)win->gl.glContext, sel("makeCurrentContext"));
    return 0;
}

int tigrGAPIEnd(Tigr* bmp) {
    (void)bmp;
    objc_msgSend_void(class("NSOpenGLContext"), sel("clearCurrentContext"));
    return 0;
}

int tigrClosed(Tigr* bmp) {
    return (terminated || _tigrCocoaIsWindowClosed((id)bmp->handle)) ? 1 : 0;
}

void tigrMouse(Tigr* bmp, int* x, int* y, int* buttons) {
    TigrInternal* win;
    id window;
    win = tigrInternal(bmp);
    window = (id)bmp->handle;

    id windowContentView = objc_msgSend_id(window, sel("contentView"));
    NSRect adjustFrame =  objc_msgSend_stret_t(NSRect)(windowContentView, sel("frame"));

    // NSPoint is small enough to fit a register, so no need for
    // objc_msgSend_stret
    NSPoint p = objc_msgSend_t(NSPoint)(window, sel("mouseLocationOutsideOfEventStream"));

    // map input to content view rect
    if (p.x < 0)
        p.x = 0;
    else if (p.x > adjustFrame.size.width)
        p.x = adjustFrame.size.width;
    if (p.y < 0)
        p.y = 0;
    else if (p.y > adjustFrame.size.height)
        p.y = adjustFrame.size.height;

    // map input to pixels
    NSRect r = { p, {0, 0} };
    r = objc_msgSend_stret_t(NSRect, NSRect)(windowContentView, sel("convertRectToBacking:"), r);
    p = r.origin;

    p.x = (p.x - win->pos[0]) / win->scale;
    p.y = bmp->h - (p.y - win->pos[1]) / win->scale;

    if (x)
        *x = p.x;
    if (y)
        *y = p.y;

    if (buttons) {
        id keyWindow = objc_msgSend_id(NSApp, sel("keyWindow"));
        *buttons = keyWindow != bmp->handle ? 0 : win->mouseButtons;
    }
}

int tigrTouch(Tigr *bmp, TigrTouchPoint* points, int maxPoints) {
	int buttons = 0;
	if (maxPoints > 0) {
		tigrMouse(bmp, &points[0].x, &points[1].y, &buttons);
	}
	return buttons ? 1 : 0;
}

int tigrKeyDown(Tigr* bmp, int key) {
    TigrInternal* win;
    assert(key < 256);
    win = tigrInternal(bmp);
    return (win->keys[key] != 0) && (win->prev[key] == 0);
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

float tigrTime() {
    static uint64_t time = 0;
    static mach_timebase_info_data_t timebaseInfo;

    if (timebaseInfo.denom == 0) {
        mach_timebase_info(&timebaseInfo);
        time = mach_absolute_time();
        return 0.0f;
    }

    uint64_t current_time = mach_absolute_time();
    double elapsed = (double)(current_time - time) * timebaseInfo.numer / (timebaseInfo.denom * 1000000000.0);
    time = current_time;
    return (float)elapsed;
}

#endif
