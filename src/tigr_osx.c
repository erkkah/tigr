// this one is based on https://github.com/jimon/osx_app_in_plain_c

#include "tigr_internal.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#ifdef TARGET_OS_MAC

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <math.h>
#include <assert.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <objc/NSObjCRuntime.h>
#include <OpenGL/gl.h>
#include <mach/mach_time.h>

// maybe this is available somewhere in objc runtime?
#if __LP64__ || (TARGET_OS_EMBEDDED && !TARGET_OS_IPHONE) || TARGET_OS_WIN32 || NS_BUILD_32_LIKE_64
#define NSIntegerEncoding "q"
#define NSUIntegerEncoding "L"
#else
#define NSIntegerEncoding "i"
#define NSUIntegerEncoding "I"
#endif

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#else
// this is how they are defined originally
#include <CoreGraphics/CGBase.h>
#include <CoreGraphics/CGGeometry.h>
typedef CGPoint NSPoint;
typedef CGSize NSSize;
typedef CGRect NSRect;

extern id NSApp;
extern id const NSDefaultRunLoopMode;
#endif

#if defined(__OBJC__) && __has_feature(objc_arc)
//#define ARC_AVAILABLE
#error cant compile as objective-c code just yet! see autorelease pool todo bellow
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

bool terminated = false;
uint32_t windowCount = 0;

// we gonna construct objective-c class by hand in runtime, so wow, so hacker!
//@interface AppDelegate : NSObject<NSApplicationDelegate>
//-(NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;
//@end
//@implementation AppDelegate
//-(NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
//{
//	terminated = true;
//	return NSTerminateCancel;
//}
//@end
NSUInteger applicationShouldTerminate(id self, SEL _sel, id sender)
{
	terminated = true;
	return 0;
}

//@interface WindowDelegate : NSObject<NSWindowDelegate>
//-(void)windowWillClose:(NSNotification*)notification;
//@end
//@implementation WindowDelegate
//-(void)windowWillClose:(NSNotification*)notification
//{
//	(void)notification;
//	assert(windowCount);
//	if(--windowCount == 0)
//		terminated = true;
//}
//@end
void windowWillClose(id self, SEL _sel, id notification)
{
	assert(windowCount);
	if(--windowCount == 0)
		terminated = true;
}

static bool tigrOSXInited = false;
static id autoreleasePool = NULL;

void tigrInitOSX()
{
	if(tigrOSXInited)
		return;

	#ifdef ARC_AVAILABLE
	// TODO and what do we do now? it's a bit too tricky to use @autoreleasepool here
	#else
	//NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
	//would be nice to use objc_autoreleasePoolPush instead, but it's not publically available in the headers
	id poolAlloc = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSAutoreleasePool"), sel_registerName("alloc"));
	autoreleasePool = ((id (*)(id, SEL))objc_msgSend)(poolAlloc, sel_registerName("init"));
	#endif

	//[NSApplication sharedApplication];
	((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSApplication"), sel_registerName("sharedApplication"));

	//[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
	((void (*)(id, SEL, NSInteger))objc_msgSend)(NSApp, sel_registerName("setActivationPolicy:"), 0);

	//AppDelegate * dg = [[AppDelegate alloc] init];
	Class appDelegateClass = objc_allocateClassPair((Class)objc_getClass("NSObject"), "AppDelegate", 0);
	bool resultAddProtoc = class_addProtocol(appDelegateClass, objc_getProtocol("NSApplicationDelegate"));
	assert(resultAddProtoc);
	bool resultAddMethod = class_addMethod(appDelegateClass, sel_registerName("applicationShouldTerminate:"), (IMP)applicationShouldTerminate, NSUIntegerEncoding "@:@");
	assert(resultAddMethod);
	id dgAlloc = ((id (*)(id, SEL))objc_msgSend)((id)appDelegateClass, sel_registerName("alloc"));
	id dg = ((id (*)(id, SEL))objc_msgSend)(dgAlloc, sel_registerName("init"));
	#ifndef ARC_AVAILABLE
	((void (*)(id, SEL))objc_msgSend)(dg, sel_registerName("autorelease"));
	#endif

	//[NSApp setDelegate:dg];
	((void (*)(id, SEL, id))objc_msgSend)(NSApp, sel_registerName("setDelegate:"), dg);

	// only needed if we don't use [NSApp run]
	//[NSApp finishLaunching];
	((void (*)(id, SEL))objc_msgSend)(NSApp, sel_registerName("finishLaunching"));

	//id menubar = [[NSMenu alloc] init];
	id menubarAlloc = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSMenu"), sel_registerName("alloc"));
	id menubar = ((id (*)(id, SEL))objc_msgSend)(menubarAlloc, sel_registerName("init"));
	#ifndef ARC_AVAILABLE
	((void (*)(id, SEL))objc_msgSend)(menubar, sel_registerName("autorelease"));
	#endif

	//id appMenuItem = [[NSMenuItem alloc] init];
	id appMenuItemAlloc = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSMenuItem"), sel_registerName("alloc"));
	id appMenuItem = ((id (*)(id, SEL))objc_msgSend)(appMenuItemAlloc, sel_registerName("init"));
	#ifndef ARC_AVAILABLE
	((void (*)(id, SEL))objc_msgSend)(appMenuItem, sel_registerName("autorelease"));
	#endif

	//[menubar addItem:appMenuItem];
	((void (*)(id, SEL, id))objc_msgSend)(menubar, sel_registerName("addItem:"), appMenuItem);

	//[NSApp setMainMenu:menubar];
	((id (*)(id, SEL, id))objc_msgSend)(NSApp, sel_registerName("setMainMenu:"), menubar);

	//id appMenu = [[NSMenu alloc] init];
	id appMenuAlloc = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSMenu"), sel_registerName("alloc"));
	id appMenu = ((id (*)(id, SEL))objc_msgSend)(appMenuAlloc, sel_registerName("init"));
	#ifndef ARC_AVAILABLE
	((void (*)(id, SEL))objc_msgSend)(appMenu, sel_registerName("autorelease"));
	#endif

	//id appName = [[NSProcessInfo processInfo] processName];
	id processInfo = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSProcessInfo"), sel_registerName("processInfo"));
	id appName = ((id (*)(id, SEL))objc_msgSend)(processInfo, sel_registerName("processName"));

	//id quitTitle = [@"Quit " stringByAppendingString:appName];
	id quitTitlePrefixString = ((id (*)(id, SEL, const char*))objc_msgSend)((id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), "Quit ");
	id quitTitle = ((id (*)(id, SEL, id))objc_msgSend)(quitTitlePrefixString, sel_registerName("stringByAppendingString:"), appName);

	//id quitMenuItem = [[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"];
	id quitMenuItemKey = ((id (*)(id, SEL, const char*))objc_msgSend)((id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), "q");
	id quitMenuItemAlloc = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSMenuItem"), sel_registerName("alloc"));
	id quitMenuItem = ((id (*)(id, SEL, id, SEL, id))objc_msgSend)(quitMenuItemAlloc, sel_registerName("initWithTitle:action:keyEquivalent:"), quitTitle, sel_registerName("terminate:"), quitMenuItemKey);
	#ifndef ARC_AVAILABLE
	((void (*)(id, SEL))objc_msgSend)(quitMenuItem, sel_registerName("autorelease"));
	#endif

	//[appMenu addItem:quitMenuItem];
	((void (*)(id, SEL, id))objc_msgSend)(appMenu, sel_registerName("addItem:"), quitMenuItem);

	//[appMenuItem setSubmenu:appMenu];
	((void (*)(id, SEL, id))objc_msgSend)(appMenuItem, sel_registerName("setSubmenu:"), appMenu);

	tigrOSXInited = true;
}

#if TIGR_PRESERVE_WINDOW_POSITION
#error not implemented
#endif

void tigrError(Tigr *bmp, const char *message, ...)
{
	char tmp[1024];

	va_list args;
	va_start(args, message);
	vsnprintf(tmp, sizeof(tmp), message, args);
	tmp[sizeof(tmp)-1] = 0;
	va_end(args);

	CFStringRef header = CFStringCreateWithCString(NULL, "Error", kCFStringEncodingUTF8);
	CFStringRef msg = CFStringCreateWithCString(NULL, tmp, kCFStringEncodingUTF8);
	CFUserNotificationDisplayNotice(0.0, kCFUserNotificationStopAlertLevel, NULL, NULL, NULL, header, msg, NULL);
	CFRelease(header);
	CFRelease(msg);
	exit(1);
}

NSSize _tigrCocoaWindowSize(id window)
{
	//NSView * contentView = [window contentView];
	id contentView = ((id (*)(id, SEL))objc_msgSend)(window, sel_registerName("contentView"));

	//NSRect rect = [contentView frame];
	NSRect rect = ((NSRect (*)(id, SEL))abi_objc_msgSend_stret)(contentView, sel_registerName("frame"));

	//rect = [contentView convertRectToBacking:rect];
	rect = ((NSRect (*)(id, SEL, NSRect))abi_objc_msgSend_stret)(contentView, sel_registerName("convertRectToBacking:"), rect);

	return rect.size;
}

Tigr *tigrWindow(int w, int h, const char *title, int flags)
{
	int scale;
	Tigr *bmp;
	TigrInternal *win;

	tigrInitOSX();

	if (flags & TIGR_AUTO)
	{
		// Always use a 1:1 pixel size.
		scale = 1;
	} else {
		// See how big we can make it and still fit on-screen.
		CGRect mainMonitor = CGDisplayBounds(CGMainDisplayID());
		int maxW = CGRectGetHeight(mainMonitor) * 3/4;
		int maxH = CGRectGetWidth(mainMonitor) * 3/4;
		scale = tigrCalcScale(w, h, maxW, maxH);
	}

	scale = tigrEnforceScale(scale, flags);

	//id window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, w, h) styleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask backing:NSBackingStoreBuffered defer:NO];
	NSRect rect = {{0, 0}, {w * scale, h * scale}};
	id windowAlloc = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSWindow"), sel_registerName("alloc"));
	id window = ((id (*)(id, SEL, NSRect, NSUInteger, NSUInteger, BOOL))objc_msgSend)(windowAlloc, sel_registerName("initWithContentRect:styleMask:backing:defer:"), rect, 15, 2, NO);
	#ifndef ARC_AVAILABLE
	((void (*)(id, SEL))objc_msgSend)(window, sel_registerName("autorelease"));
	#endif

	// when we are not using ARC, than window will be added to autorelease pool
	// so if we close it by hand (pressing red button), we don't want it to be released for us
	// so it will be released by autorelease pool later
	//[window setReleasedWhenClosed:NO];
	((void (*)(id, SEL, BOOL))objc_msgSend)(window, sel_registerName("setReleasedWhenClosed:"), NO);

	++windowCount;

	//WindowDelegate * wdg = [[WindowDelegate alloc] init];
	Class WindowDelegateClass = objc_allocateClassPair((Class)objc_getClass("NSObject"), "WindowDelegate", 0);
	bool resultAddProtoc = class_addProtocol(WindowDelegateClass, objc_getProtocol("NSWindowDelegate"));
	assert(resultAddProtoc);
	bool resultAddMethod = class_addMethod(WindowDelegateClass, sel_registerName("windowWillClose:"), (IMP)windowWillClose,  "v@:@");
	assert(resultAddMethod);
	id wdgAlloc = ((id (*)(id, SEL))objc_msgSend)((id)WindowDelegateClass, sel_registerName("alloc"));
	id wdg = ((id (*)(id, SEL))objc_msgSend)(wdgAlloc, sel_registerName("init"));
	#ifndef ARC_AVAILABLE
	((void (*)(id, SEL))objc_msgSend)(wdg, sel_registerName("autorelease"));
	#endif

	//[window setDelegate:wdg];
	((void (*)(id, SEL, id))objc_msgSend)(window, sel_registerName("setDelegate:"), wdg);

	//NSView * contentView = [window contentView];
	id contentView = ((id (*)(id, SEL))objc_msgSend)(window, sel_registerName("contentView"));

	if(flags & TIGR_RETINA)
	{
		// disable this if you don't want retina support
		//[contentView setWantsBestResolutionOpenGLSurface:YES];
		((void (*)(id, SEL, BOOL))objc_msgSend)(contentView, sel_registerName("setWantsBestResolutionOpenGLSurface:"), YES);
	}

	//[window cascadeTopLeftFromPoint:NSMakePoint(20,20)];
	NSPoint point = {20, 20};
	((void (*)(id, SEL, NSPoint))objc_msgSend)(window, sel_registerName("cascadeTopLeftFromPoint:"), point);

	//[window setTitle:@"sup"];
	id titleString = ((id (*)(id, SEL, const char*))objc_msgSend)((id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), title);
	((void (*)(id, SEL, id))objc_msgSend)(window, sel_registerName("setTitle:"), titleString);

	//NSOpenGLPixelFormatAttribute glAttributes[] =
	//{
	//	NSOpenGLPFAColorSize, 24,
	//	NSOpenGLPFAAlphaSize, 8,
	//	NSOpenGLPFADoubleBuffer,
	//	NSOpenGLPFAAccelerated,
	//	NSOpenGLPFANoRecovery,
	//	NSOpenGLPFASampleBuffers, 1,
	//	NSOpenGLPFASamples, 4,
	//	NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
	//	0
	//};
	uint32_t glAttributes[] =
	{
		8, 24,
		11, 8,
		5,
		73,
		72,
		//55, 1,
		//56, 4,
		99, 0x3200,
		0
	};

	//NSOpenGLPixelFormat * pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:glAttributes];
	id pixelFormatAlloc = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSOpenGLPixelFormat"), sel_registerName("alloc"));
	id pixelFormat = ((id (*)(id, SEL, const uint32_t*))objc_msgSend)(pixelFormatAlloc, sel_registerName("initWithAttributes:"), glAttributes);
	#ifndef ARC_AVAILABLE
	((void (*)(id, SEL))objc_msgSend)(pixelFormat, sel_registerName("autorelease"));
	#endif

	//NSOpenGLContext * openGLContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
	id openGLContextAlloc = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSOpenGLContext"), sel_registerName("alloc"));
	id openGLContext = ((id (*)(id, SEL, id, id))objc_msgSend)(openGLContextAlloc, sel_registerName("initWithFormat:shareContext:"), pixelFormat, nil);
	#ifndef ARC_AVAILABLE
	((void (*)(id, SEL))objc_msgSend)(openGLContext, sel_registerName("autorelease"));
	#endif

	//[openGLContext setView:contentView];
	((void (*)(id, SEL, id))objc_msgSend)(openGLContext, sel_registerName("setView:"), contentView);

	//[window makeKeyAndOrderFront:window];
	((void (*)(id, SEL, id))objc_msgSend)(window, sel_registerName("makeKeyAndOrderFront:"), window);

	//[window setAcceptsMouseMovedEvents:YES];
	((void (*)(id, SEL, BOOL))objc_msgSend)(window, sel_registerName("setAcceptsMouseMovedEvents:"), YES);

	//[window setBackgroundColor:[NSColor blackColor]];
	id blackColor = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSColor"), sel_registerName("blackColor"));
	((void (*)(id, SEL, id))objc_msgSend)(window, sel_registerName("setBackgroundColor:"), blackColor);

	// TODO do we really need this?
	//[NSApp activateIgnoringOtherApps:YES];
	((void (*)(id, SEL, BOOL))objc_msgSend)(NSApp, sel_registerName("activateIgnoringOtherApps:"), YES);

	// Wrap a bitmap around it.
	NSSize windowSize = _tigrCocoaWindowSize(window);
	bmp = tigrBitmap2(windowSize.width / scale, windowSize.height / scale, sizeof(TigrInternal));
	bmp->handle = window;

	// Set up the Windows parts.
	win = tigrInternal(bmp);
	win->glContext = openGLContext;
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
	win->widgets = tigrBitmap(40, 14);
	win->gl.gl_legacy = 0;
	win->mouseButtons = 0;

	tigrPosition(bmp, win->scale, bmp->w, bmp->h, win->pos);

	//[openGLContext makeCurrentContext];
	((void (*)(id, SEL))objc_msgSend)(openGLContext, sel_registerName("makeCurrentContext"));

	tigrGAPICreate(bmp);
	tigrGAPIResize(bmp, bmp->w, bmp->h);

	return bmp;
}

void tigrFree(Tigr *bmp)
{
	if(bmp->handle)
	{
		TigrInternal * win = tigrInternal(bmp);
		tigrGAPIDestroy(bmp);
		tigrFree(win->widgets);

		#ifdef ARC_AVAILABLE
		// TODO autorelease pool
		#else
		//[autoreleasePool drain];
		((void (*)(id, SEL))objc_msgSend)(autoreleasePool, sel_registerName("drain"));
		#endif
	}
	free(bmp->pix);
	free(bmp);
}

uint8_t _tigrKeyFromOSX(uint16_t key)
{
	// from Carbon HIToolbox/Events.h
	enum
	{
		kVK_ANSI_A                    = 0x00,
		kVK_ANSI_S                    = 0x01,
		kVK_ANSI_D                    = 0x02,
		kVK_ANSI_F                    = 0x03,
		kVK_ANSI_H                    = 0x04,
		kVK_ANSI_G                    = 0x05,
		kVK_ANSI_Z                    = 0x06,
		kVK_ANSI_X                    = 0x07,
		kVK_ANSI_C                    = 0x08,
		kVK_ANSI_V                    = 0x09,
		kVK_ANSI_B                    = 0x0B,
		kVK_ANSI_Q                    = 0x0C,
		kVK_ANSI_W                    = 0x0D,
		kVK_ANSI_E                    = 0x0E,
		kVK_ANSI_R                    = 0x0F,
		kVK_ANSI_Y                    = 0x10,
		kVK_ANSI_T                    = 0x11,
		kVK_ANSI_1                    = 0x12,
		kVK_ANSI_2                    = 0x13,
		kVK_ANSI_3                    = 0x14,
		kVK_ANSI_4                    = 0x15,
		kVK_ANSI_6                    = 0x16,
		kVK_ANSI_5                    = 0x17,
		kVK_ANSI_Equal                = 0x18,
		kVK_ANSI_9                    = 0x19,
		kVK_ANSI_7                    = 0x1A,
		kVK_ANSI_Minus                = 0x1B,
		kVK_ANSI_8                    = 0x1C,
		kVK_ANSI_0                    = 0x1D,
		kVK_ANSI_RightBracket         = 0x1E,
		kVK_ANSI_O                    = 0x1F,
		kVK_ANSI_U                    = 0x20,
		kVK_ANSI_LeftBracket          = 0x21,
		kVK_ANSI_I                    = 0x22,
		kVK_ANSI_P                    = 0x23,
		kVK_ANSI_L                    = 0x25,
		kVK_ANSI_J                    = 0x26,
		kVK_ANSI_Quote                = 0x27,
		kVK_ANSI_K                    = 0x28,
		kVK_ANSI_Semicolon            = 0x29,
		kVK_ANSI_Backslash            = 0x2A,
		kVK_ANSI_Comma                = 0x2B,
		kVK_ANSI_Slash                = 0x2C,
		kVK_ANSI_N                    = 0x2D,
		kVK_ANSI_M                    = 0x2E,
		kVK_ANSI_Period               = 0x2F,
		kVK_ANSI_Grave                = 0x32,
		kVK_ANSI_KeypadDecimal        = 0x41,
		kVK_ANSI_KeypadMultiply       = 0x43,
		kVK_ANSI_KeypadPlus           = 0x45,
		kVK_ANSI_KeypadClear          = 0x47,
		kVK_ANSI_KeypadDivide         = 0x4B,
		kVK_ANSI_KeypadEnter          = 0x4C,
		kVK_ANSI_KeypadMinus          = 0x4E,
		kVK_ANSI_KeypadEquals         = 0x51,
		kVK_ANSI_Keypad0              = 0x52,
		kVK_ANSI_Keypad1              = 0x53,
		kVK_ANSI_Keypad2              = 0x54,
		kVK_ANSI_Keypad3              = 0x55,
		kVK_ANSI_Keypad4              = 0x56,
		kVK_ANSI_Keypad5              = 0x57,
		kVK_ANSI_Keypad6              = 0x58,
		kVK_ANSI_Keypad7              = 0x59,
		kVK_ANSI_Keypad8              = 0x5B,
		kVK_ANSI_Keypad9              = 0x5C,
		kVK_Return                    = 0x24,
		kVK_Tab                       = 0x30,
		kVK_Space                     = 0x31,
		kVK_Delete                    = 0x33,
		kVK_Escape                    = 0x35,
		kVK_Command                   = 0x37,
		kVK_Shift                     = 0x38,
		kVK_CapsLock                  = 0x39,
		kVK_Option                    = 0x3A,
		kVK_Control                   = 0x3B,
		kVK_RightShift                = 0x3C,
		kVK_RightOption               = 0x3D,
		kVK_RightControl              = 0x3E,
		kVK_Function                  = 0x3F,
		kVK_F17                       = 0x40,
		kVK_VolumeUp                  = 0x48,
		kVK_VolumeDown                = 0x49,
		kVK_Mute                      = 0x4A,
		kVK_F18                       = 0x4F,
		kVK_F19                       = 0x50,
		kVK_F20                       = 0x5A,
		kVK_F5                        = 0x60,
		kVK_F6                        = 0x61,
		kVK_F7                        = 0x62,
		kVK_F3                        = 0x63,
		kVK_F8                        = 0x64,
		kVK_F9                        = 0x65,
		kVK_F11                       = 0x67,
		kVK_F13                       = 0x69,
		kVK_F16                       = 0x6A,
		kVK_F14                       = 0x6B,
		kVK_F10                       = 0x6D,
		kVK_F12                       = 0x6F,
		kVK_F15                       = 0x71,
		kVK_Help                      = 0x72,
		kVK_Home                      = 0x73,
		kVK_PageUp                    = 0x74,
		kVK_ForwardDelete             = 0x75,
		kVK_F4                        = 0x76,
		kVK_End                       = 0x77,
		kVK_F2                        = 0x78,
		kVK_PageDown                  = 0x79,
		kVK_F1                        = 0x7A,
		kVK_LeftArrow                 = 0x7B,
		kVK_RightArrow                = 0x7C,
		kVK_DownArrow                 = 0x7D,
		kVK_UpArrow                   = 0x7E
	};

	switch(key)
	{
	case kVK_ANSI_Q: return 'Q';
	case kVK_ANSI_W: return 'W';
	case kVK_ANSI_E: return 'E';
	case kVK_ANSI_R: return 'R';
	case kVK_ANSI_T: return 'T';
	case kVK_ANSI_Y: return 'Y';
	case kVK_ANSI_U: return 'U';
	case kVK_ANSI_I: return 'I';
	case kVK_ANSI_O: return 'O';
	case kVK_ANSI_P: return 'P';
	case kVK_ANSI_A: return 'A';
	case kVK_ANSI_S: return 'S';
	case kVK_ANSI_D: return 'D';
	case kVK_ANSI_F: return 'F';
	case kVK_ANSI_G: return 'G';
	case kVK_ANSI_H: return 'H';
	case kVK_ANSI_J: return 'J';
	case kVK_ANSI_K: return 'K';
	case kVK_ANSI_L: return 'L';
	case kVK_ANSI_Z: return 'Z';
	case kVK_ANSI_X: return 'X';
	case kVK_ANSI_C: return 'C';
	case kVK_ANSI_V: return 'V';
	case kVK_ANSI_B: return 'B';
	case kVK_ANSI_N: return 'N';
	case kVK_ANSI_M: return 'M';
	case kVK_ANSI_0: return '0';
	case kVK_ANSI_1: return '1';
	case kVK_ANSI_2: return '2';
	case kVK_ANSI_3: return '3';
	case kVK_ANSI_4: return '4';
	case kVK_ANSI_5: return '5';
	case kVK_ANSI_6: return '6';
	case kVK_ANSI_7: return '7';
	case kVK_ANSI_8: return '8';
	case kVK_ANSI_9: return '9';
	case kVK_ANSI_Keypad0: return TK_PAD0;
	case kVK_ANSI_Keypad1: return TK_PAD1;
	case kVK_ANSI_Keypad2: return TK_PAD2;
	case kVK_ANSI_Keypad3: return TK_PAD3;
	case kVK_ANSI_Keypad4: return TK_PAD4;
	case kVK_ANSI_Keypad5: return TK_PAD5;
	case kVK_ANSI_Keypad6: return TK_PAD6;
	case kVK_ANSI_Keypad7: return TK_PAD7;
	case kVK_ANSI_Keypad8: return TK_PAD8;
	case kVK_ANSI_Keypad9: return TK_PAD9;
	case kVK_ANSI_KeypadMultiply: return TK_PADMUL;
	case kVK_ANSI_KeypadPlus: return TK_PADADD;
	case kVK_ANSI_KeypadEnter: return TK_PADENTER;
	case kVK_ANSI_KeypadMinus: return TK_PADSUB;
	case kVK_ANSI_KeypadDecimal: return TK_PADDOT;
	case kVK_ANSI_KeypadDivide: return TK_PADDIV;
	case kVK_F1: return TK_F1;
	case kVK_F2: return TK_F2;
	case kVK_F3: return TK_F3;
	case kVK_F4: return TK_F4;
	case kVK_F5: return TK_F5;
	case kVK_F6: return TK_F6;
	case kVK_F7: return TK_F7;
	case kVK_F8: return TK_F8;
	case kVK_F9: return TK_F9;
	case kVK_F10: return TK_F10;
	case kVK_F11: return TK_F11;
	case kVK_F12: return TK_F12;
	case kVK_Shift: return TK_LSHIFT;
	case kVK_Control: return TK_LCONTROL;
	case kVK_Option: return TK_LALT;
	case kVK_CapsLock: return TK_CAPSLOCK;
	case kVK_Command: return TK_LWIN;
	case kVK_Command - 1: return TK_RWIN;
	case kVK_RightShift: return TK_RSHIFT;
	case kVK_RightControl: return TK_RCONTROL;
	case kVK_RightOption: return TK_RALT;
	case kVK_Delete: return TK_BACKSPACE;
	case kVK_Tab: return TK_TAB;
	case kVK_Return: return TK_RETURN;
	case kVK_Escape: return TK_ESCAPE;
	case kVK_Space: return TK_SPACE;
	case kVK_PageUp: return TK_PAGEUP;
	case kVK_PageDown: return TK_PAGEDN;
	case kVK_End: return TK_END;
	case kVK_Home: return TK_HOME;
	case kVK_LeftArrow: return TK_LEFT;
	case kVK_UpArrow: return TK_UP;
	case kVK_RightArrow: return TK_RIGHT;
	case kVK_DownArrow: return TK_DOWN;
	case kVK_Help: return TK_INSERT;
	case kVK_ForwardDelete: return TK_DELETE;
	case kVK_F14: return TK_SCROLL;
	case kVK_F15: return TK_PAUSE;
	case kVK_ANSI_KeypadClear: return TK_NUMLOCK;
	case kVK_ANSI_Semicolon: return TK_SEMICOLON;
	case kVK_ANSI_Equal: return TK_EQUALS;
	case kVK_ANSI_Comma: return TK_COMMA;
	case kVK_ANSI_Minus: return TK_MINUS;
	case kVK_ANSI_Slash: return TK_SLASH;
	case kVK_ANSI_Backslash: return TK_BACKSLASH;
	case kVK_ANSI_Grave: return TK_BACKTICK;
	case kVK_ANSI_Quote: return TK_TICK;
	case kVK_ANSI_LeftBracket: return TK_LSQUARE;
	case kVK_ANSI_RightBracket: return TK_RSQUARE;
	case kVK_ANSI_Period: return TK_DOT;
	default: return 0;
	}
}

void tigrUpdate(Tigr *bmp)
{
	TigrInternal *win;
	id openGLContext;
	id window;
	win = tigrInternal(bmp);
	window = bmp->handle;
	openGLContext = win->glContext;

	//NSEvent * event = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
	id distantPast = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSDate"), sel_registerName("distantPast"));
	id event = ((id (*)(id, SEL, NSUInteger, id, id, BOOL))objc_msgSend)(NSApp, sel_registerName("nextEventMatchingMask:untilDate:inMode:dequeue:"), NSUIntegerMax, distantPast, NSDefaultRunLoopMode, YES);

	//NSWindow * keyWindow = [NSApp keyWindow];
	id keyWindow = ((id (*)(id, SEL))objc_msgSend)(NSApp, sel_registerName("keyWindow"));

	if(keyWindow == window)
	{
		memcpy(win->prev, win->keys, 256);
	}
	else
	{
		memcpy(win->prev, win->keys, 256);
		memset(win->keys, 0, 256);
		win->mouseButtons = 0;
	}

	if(event)
	{
		//NSEventType eventType = [event type];
		NSUInteger eventType = ((NSUInteger (*)(id, SEL))objc_msgSend)(event, sel_registerName("type"));

		switch(eventType)
		{
		//case NSLeftMouseDown:
		case 1:
			win->mouseButtons |= 1;
			break;
		//case NSLeftMouseUp:
		case 2:
			win->mouseButtons &= ~1;
			break;
		//case NSRightMouseDown:
		case 3:
			win->mouseButtons |= 2;
			break;
		//case NSRightMouseUp:
		case 4:
			win->mouseButtons &= ~2;
			break;
		//case NSOtherMouseDown:
		case 25:
		{
			// number == 2 is a middle button
			//NSInteger number = [event buttonNumber];
			NSInteger number = ((NSInteger (*)(id, SEL))objc_msgSend)(event, sel_registerName("buttonNumber"));
			if(number == 2)
				win->mouseButtons |= 4;
			break;
		}
		//case NSOtherMouseUp:
		case 26:
		{
			//NSInteger number = [event buttonNumber];
			NSInteger number = ((NSInteger (*)(id, SEL))objc_msgSend)(event, sel_registerName("buttonNumber"));
			if(number == 2)
				win->mouseButtons &= ~4;
			break;
		}
		//case NSScrollWheel:
		//case 22:
		//{
		//	//CGFloat deltaX = [event scrollingDeltaX];
		//	CGFloat deltaX = ((CGFloat (*)(id, SEL))abi_objc_msgSend_fpret)(event, sel_registerName("scrollingDeltaX"));
		//
		//	//CGFloat deltaY = [event scrollingDeltaY];
		//	CGFloat deltaY = ((CGFloat (*)(id, SEL))abi_objc_msgSend_fpret)(event, sel_registerName("scrollingDeltaY"));
		//
		//	//BOOL precisionScrolling = [event hasPreciseScrollingDeltas];
		//	BOOL precisionScrolling = ((BOOL (*)(id, SEL))objc_msgSend)(event, sel_registerName("hasPreciseScrollingDeltas"));
		//
		//	if(precisionScrolling)
		//	{
		//		deltaX *= 0.1f; // similar to glfw
		//		deltaY *= 0.1f;
		//	}
		//
		//	if(fabs(deltaX) > 0.0f || fabs(deltaY) > 0.0f)
		//		printf("mouse scroll wheel delta %f %f\n", deltaX, deltaY);
		//	break;
		//}
		//case NSFlagsChanged:
		case 12:
		{
			//NSEventModifierFlags modifiers = [event modifierFlags];
			NSUInteger modifiers = ((NSUInteger (*)(id, SEL))objc_msgSend)(event, sel_registerName("modifierFlags"));

			// based on NSEventModifierFlags
			struct
			{
				union
				{
					struct
					{
						uint8_t alpha_shift:1;
						uint8_t shift:1;
						uint8_t control:1;
						uint8_t alternate:1;
						uint8_t command:1;
						uint8_t numeric_pad:1;
						uint8_t help:1;
						uint8_t function:1;
					};
					uint8_t mask;
				};
			} keys;

			//keys.mask = (modifiers & NSDeviceIndependentModifierFlagsMask) >> 16;
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
		//case NSKeyDown:
		case 10:
		{
			//NSString * inputText = [event characters];
			id inputText = ((id (*)(id, SEL))objc_msgSend)(event, sel_registerName("characters"));

			//const char * inputTextUTF8 = [inputText UTF8String];
			const char * inputTextUTF8 = ((const char* (*)(id, SEL))objc_msgSend)(inputText, sel_registerName("UTF8String"));

			tigrDecodeUTF8(inputTextUTF8, &win->lastChar);

			//uint16_t keyCode = [event keyCode];
			uint16_t keyCode = ((unsigned short (*)(id, SEL))objc_msgSend)(event, sel_registerName("keyCode"));

			win->keys[_tigrKeyFromOSX(keyCode)] = 1;

			break;
		}
		//case NSKeyUp:
		case 11:
		{
			//uint16_t keyCode = [event keyCode];
			uint16_t keyCode = ((unsigned short (*)(id, SEL))objc_msgSend)(event, sel_registerName("keyCode"));

			win->keys[_tigrKeyFromOSX(keyCode)] = 0;
			break;
		}
		default:
			break;
		}

		//[NSApp sendEvent:event];
		((void (*)(id, SEL, id))objc_msgSend)(NSApp, sel_registerName("sendEvent:"), event);

		// if user closes the window we might need to terminate asap
		if(terminated)
			return;

		//[NSApp updateWindows];
		((void (*)(id, SEL))objc_msgSend)(NSApp, sel_registerName("updateWindows"));
	}

	// do runloop stuff
	//[openGLContext update]; // probably we only need to do it when we resize the window
	((void (*)(id, SEL))objc_msgSend)(openGLContext, sel_registerName("update"));

	//[openGLContext makeCurrentContext];
	((void (*)(id, SEL))objc_msgSend)(openGLContext, sel_registerName("makeCurrentContext"));

	NSSize windowSize = _tigrCocoaWindowSize(window);

	if (win->flags & TIGR_AUTO)
		tigrResize(bmp, windowSize.width / win->scale, windowSize.height / win->scale);
	else
		win->scale = tigrEnforceScale(tigrCalcScale(bmp->w, bmp->h, windowSize.width, windowSize.height), win->flags);
	tigrPosition(bmp, win->scale, windowSize.width, windowSize.height, win->pos);
	tigrGAPIResize(bmp, windowSize.width, windowSize.height);

	tigrGAPIPresent(bmp, windowSize.width, windowSize.height);

	//[openGLContext flushBuffer];
	((void (*)(id, SEL))objc_msgSend)(openGLContext, sel_registerName("flushBuffer"));
}

int tigrClosed(Tigr *bmp)
{
	return terminated; // return 1 if closed
}

void tigrMouse(Tigr *bmp, int *x, int *y, int *buttons)
{
	TigrInternal *win;
	id window;
	win = tigrInternal(bmp);
	window = bmp->handle;

	//NSRect adjustFrame = [[currentWindow contentView] frame];
	id windowContentView = ((id (*)(id, SEL))objc_msgSend)(window, sel_registerName("contentView"));
	NSRect adjustFrame = ((NSRect (*)(id, SEL))abi_objc_msgSend_stret)(windowContentView, sel_registerName("frame"));

	//NSPoint p = [currentWindow mouseLocationOutsideOfEventStream];
	// NSPoint is small enough to fit a register, so no need for objc_msgSend_stret
	NSPoint p = ((NSPoint (*)(id, SEL))objc_msgSend)(window, sel_registerName("mouseLocationOutsideOfEventStream"));

	// map input to content view rect
	if(p.x < 0) p.x = 0;
	else if(p.x > adjustFrame.size.width) p.x = adjustFrame.size.width;
	if(p.y < 0) p.y = 0;
	else if(p.y > adjustFrame.size.height) p.y = adjustFrame.size.height;

	// map input to pixels
	NSRect r = {p.x, p.y, 0, 0};
	//r = [currentWindowContentView convertRectToBacking:r];
	r = ((NSRect (*)(id, SEL, NSRect))abi_objc_msgSend_stret)(windowContentView, sel_registerName("convertRectToBacking:"), r);
	p = r.origin;

	p.x = (p.x - win->pos[0]) / win->scale;
	p.y = (adjustFrame.size.height - p.y - win->pos[1]) / win->scale;

	if(x)
		*x = p.x;
	if(y)
		*y = p.y;

	//NSWindow * keyWindow = [NSApp keyWindow];
	id keyWindow = ((id (*)(id, SEL))objc_msgSend)(NSApp, sel_registerName("keyWindow"));

	if(buttons)
		*buttons = keyWindow != bmp->handle ? 0 : win->mouseButtons;
}

int tigrKeyDown(Tigr *bmp, int key)
{
	TigrInternal *win;
	assert(key < 256);
	//NSWindow * keyWindow = [NSApp keyWindow];
	id keyWindow = ((id (*)(id, SEL))objc_msgSend)(NSApp, sel_registerName("keyWindow"));
	if(keyWindow != bmp->handle)
		return 0;
	win = tigrInternal(bmp);
	return win->keys[key] && !win->prev[key];
	return 0;
}

int tigrKeyHeld(Tigr *bmp, int key)
{
	TigrInternal *win;
	assert(key < 256);
	//NSWindow * keyWindow = [NSApp keyWindow];
	id keyWindow = ((id (*)(id, SEL))objc_msgSend)(NSApp, sel_registerName("keyWindow"));
	if(keyWindow != bmp->handle)
		return 0;
	win = tigrInternal(bmp);
	return win->keys[key];
	return 0;
}

int tigrReadChar(Tigr *bmp)
{
	TigrInternal *win = tigrInternal(bmp);
	int c = win->lastChar;
	win->lastChar = 0;
	return c;
}

float tigrTime()
{
	static uint64_t time = 0;
	static mach_timebase_info_data_t timebaseInfo;

	if(timebaseInfo.denom == 0)
	{
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
#endif
