#include "tigr_internal.h"

#if __linux__ && !__ANDROID__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/XKBlib.h>
#include <GL/glx.h>

static Display *dpy;
static Window root;
static XVisualInfo *vi;
static Atom wmDeleteMessage;
static XIM inputMethod;
static GLXFBConfig fbConfig;

PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = 0;

static void initX11Stuff() {
	static int done = 0;
	if(!done) {
		dpy = XOpenDisplay(NULL);
		if(dpy == NULL) {
			tigrError(0, "Cannot connect to X server");
		}

		root = DefaultRootWindow(dpy);

		static int attribList[] = {
        	GLX_RENDER_TYPE, GLX_RGBA_BIT,
        	GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        	GLX_DOUBLEBUFFER, 1,
        	GLX_RED_SIZE, 1,
        	GLX_GREEN_SIZE, 1,
        	GLX_BLUE_SIZE, 1,
        	None
    	};

    	int fbcCount = 0;
    	GLXFBConfig *fbc = 
			glXChooseFBConfig(
				dpy, DefaultScreen(dpy),
                attribList, &fbcCount
			);
		if (!fbc) {
			tigrError(0, "Failed to choose FB config");
		}
		fbConfig = fbc[0];

		vi = glXGetVisualFromFBConfig(dpy, fbConfig);
	 	if(vi == NULL) {
	 		tigrError(0, "No appropriate visual found");
	 	}

		GLXContext tmpCtx = glXCreateContext(dpy, vi, 0, GL_TRUE);
		glXCreateContextAttribsARB =
			(PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");
		glXDestroyContext(dpy, tmpCtx);
		if (!glXCreateContextAttribsARB) {
			tigrError(0, "Failed to get glXCreateContextAttribsARB");
		}

	 	inputMethod = XOpenIM(dpy, NULL, NULL, NULL);
	 	if(inputMethod == NULL) {
	 		tigrError(0, "Failed to create input method");
	 	}

		wmDeleteMessage = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

		done = 1;
	}
}

static int hasGLXExtension(Display* display, const char* wanted) {
	const char* extensions = glXQueryExtensionsString(display, DefaultScreen(display));
	char* dup = strdup(extensions);
	char* found = 0;

	for (char* start = dup; ;start = 0) {
		found = strtok(start, " ");
		if (found == 0 || strcmp(found, wanted) == 0) {
			break;
		}
	}

	free(dup);
	return found != 0;
}

static void setupVSync(Display* display, Window win) {
	if (hasGLXExtension(display, "GLX_EXT_swap_control")) {
		PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT=
			(PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalEXT");
		if (glXSwapIntervalEXT) {
			glXSwapIntervalEXT(display, win, 1);
		}
	} else if (hasGLXExtension(display, "GLX_MESA_swap_control")) {
		PFNGLXSWAPINTERVALMESAPROC glXSwapIntervalMESA =
			(PFNGLXSWAPINTERVALMESAPROC)glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalMESA");
		if (glXSwapIntervalMESA) {
			glXSwapIntervalMESA(1);
		}
	} else if (hasGLXExtension(display, "GLX_SGI_swap_control")) {
		PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI =
			(PFNGLXSWAPINTERVALSGIPROC)glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalSGI");
		if (glXSwapIntervalSGI) {
			glXSwapIntervalSGI(1);
		}
	}
}

static void tigrHideCursor(TigrInternal *win) {
	Cursor invisibleCursor;
	Pixmap bitmapNoData;
	XColor black;
	static char noData[] = { 0,0,0,0,0,0,0,0 };
	black.red = black.green = black.blue = 0;

	bitmapNoData = XCreateBitmapFromData(win->dpy, win->win, noData, 8, 8);
	invisibleCursor = XCreatePixmapCursor(win->dpy, bitmapNoData, bitmapNoData, 
										&black, &black, 0, 0);
	XDefineCursor(win->dpy, win->win, invisibleCursor);
	XFreeCursor(win->dpy, invisibleCursor);
	XFreePixmap(win->dpy, bitmapNoData);
}



typedef struct {
	unsigned long flags;
	unsigned long functions;
	unsigned long decorations;
	long          inputMode;
	unsigned long status;
} WindowHints;


Tigr *tigrWindow(int w, int h, const char *title, int flags) {
	Tigr* bmp = 0;
	Colormap cmap;
	XSetWindowAttributes swa;
	Window xwin;
	GLXContext glc;
	XIC ic;
	int scale;

	initX11Stuff();

	if (flags & TIGR_AUTO) {
		// Always use a 1:1 pixel size, unless downscaled by tigrEnforceScale below.
		scale = 1;
	} else {
		// See how big we can make it and still fit on-screen.
		Screen *screen = DefaultScreenOfDisplay(dpy);
		int maxW = WidthOfScreen(screen);
		int maxH = HeightOfScreen(screen);
		scale = tigrCalcScale(w, h, maxW, maxH);
	}

	scale = tigrEnforceScale(scale, flags);

	cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
	swa.colormap = cmap;
	swa.event_mask = StructureNotifyMask;

	// Create window of wanted size
	xwin = XCreateWindow(dpy, root, 0, 0, w * scale, h * scale, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
	XMapWindow(dpy, xwin);

	if (flags & TIGR_FULLSCREEN) {
		// https://www.tonyobryan.com//index.php?article=9
		WindowHints hints;
		Atom property;
		hints.flags = 2;
		hints.decorations = 0;
		property = XInternAtom(dpy, "_MOTIF_WM_HINTS", True);
		XChangeProperty(dpy, xwin, property, property, 32, PropModeReplace, (unsigned char *)&hints, 5);
		int screen = DefaultScreen(dpy);
		int dWidth = DisplayWidth(dpy, screen);
		int dHeight = DisplayHeight(dpy, screen);
		XMoveResizeWindow(dpy, xwin, 0, 0, dWidth, dHeight);
		XMapRaised(dpy, xwin);
		XGrabPointer(dpy, xwin, True, 0, GrabModeAsync, GrabModeAsync, xwin, 0L, CurrentTime);
		XGrabKeyboard(dpy, xwin, False, GrabModeAsync, GrabModeAsync, CurrentTime);
	} else  {
		// Wait for window to get mapped
		for(;;) {
			XEvent e;
			XNextEvent(dpy, &e);
			if (e.type == MapNotify) {
				break;
			}
		}

		// Reset size if we did not get the window size we wanted above.
		XWindowAttributes wa;
		XGetWindowAttributes(dpy, xwin, &wa);
		scale = tigrCalcScale(w, h, wa.width, wa.height);
		scale = tigrEnforceScale(scale, flags);
		XResizeWindow(dpy, xwin, w * scale, h * scale);
	}

	XTextProperty prop;
	int result = Xutf8TextListToTextProperty(dpy, (char**) &title, 1, XUTF8StringStyle, &prop);
	if(result == Success) {
		Atom wmName = XInternAtom(dpy, "_NET_WM_NAME", 0);
		XSetTextProperty(dpy, xwin, &prop, wmName);
		XFree(prop.value);
	}

    ic = XCreateIC(inputMethod, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, xwin, NULL);
 	if(ic == NULL) {
 		printf("Failed to create input context\n");
 		exit(0);
 	}
 	XSetICFocus(ic);

	XSetWMProtocols(dpy, xwin, &wmDeleteMessage, 1);

	glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	int contextAttributes[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 3,
		None
	};
	glc = glXCreateContextAttribsARB(dpy, fbConfig, NULL, GL_TRUE, contextAttributes);
	glXMakeCurrent(dpy, xwin, glc);

	setupVSync(dpy, xwin);

	bmp = tigrBitmap2(w, h, sizeof(TigrInternal));
	bmp->handle = (void*)xwin;

	TigrInternal *win = tigrInternal(bmp);
	win->win = xwin;
	win->dpy = dpy;
	win->glc = glc;
	win->ic = ic;

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

	if (flags & TIGR_NOCURSOR) {
		tigrHideCursor(win);
	}

	tigrPosition(bmp, win->scale, bmp->w, bmp->h, win->pos);
 	tigrGAPICreate(bmp);
	tigrGAPIBegin(bmp);

	return bmp;
}

int tigrClosed(Tigr *bmp) {
	TigrInternal *win = tigrInternal(bmp);
	return win->win == 0;
}

int tigrGAPIBegin(Tigr *bmp) {
	TigrInternal *win = tigrInternal(bmp);
	return glXMakeCurrent(win->dpy, win->win, win->glc) ? 0 : -1;
}

int tigrGAPIEnd(Tigr *bmp) {
	(void)bmp;
	return glXMakeCurrent(NULL, 0, 0) ? 0 : -1;
}

int tigrKeyDown(Tigr *bmp, int key) {
	TigrInternal *win;
	assert(key < 256);
	win = tigrInternal(bmp);
	return win->keys[key] && !win->prev[key];
}

int tigrKeyHeld(Tigr *bmp, int key)
{
	TigrInternal *win;
	assert(key < 256);
	win = tigrInternal(bmp);
	return win->keys[key];
}

int tigrReadChar(Tigr *bmp)
{
	TigrInternal *win = tigrInternal(bmp);
	int c = win->lastChar;
	win->lastChar = 0;
	return c;
}


uint8_t tigrKeyFromX11(KeySym sym) {
	if(sym >= 'a' && sym <= 'z'){
		return (uint8_t) sym - ('a' - 'A');
	}

	if(sym >= '0' && sym <= '9') {
		return (uint8_t) sym;
	}

	switch(sym) {
		case XK_KP_0: return TK_PAD0;
		case XK_KP_1: return TK_PAD1;
		case XK_KP_2: return TK_PAD2;
		case XK_KP_3: return TK_PAD3;
		case XK_KP_4: return TK_PAD4;
		case XK_KP_5: return TK_PAD5;
		case XK_KP_6: return TK_PAD6;
		case XK_KP_7: return TK_PAD7;
		case XK_KP_8: return TK_PAD8;
		case XK_KP_9: return TK_PAD9;

		case XK_KP_Multiply: return TK_PADMUL;
		case XK_KP_Divide: return TK_PADDIV;
		case XK_KP_Add: return TK_PADADD;
		case XK_KP_Subtract: return TK_PADSUB;
		case XK_KP_Decimal: return TK_PADDOT;
		case XK_KP_Enter: return TK_PADENTER;

		case XK_F1: return TK_F1;
		case XK_F2: return TK_F2;
		case XK_F3: return TK_F3;
		case XK_F4: return TK_F4;
		case XK_F5: return TK_F5;
		case XK_F6: return TK_F6;
		case XK_F7: return TK_F7;
		case XK_F8: return TK_F8;
		case XK_F9: return TK_F9;
		case XK_F10: return TK_F10;
		case XK_F11: return TK_F11;
		case XK_F12: return TK_F12;

		case XK_BackSpace: return TK_BACKSPACE;
		case XK_Tab: return TK_TAB;
		case XK_Return: return TK_RETURN;
		case XK_Pause: return TK_PAUSE;
		case XK_Caps_Lock: return TK_CAPSLOCK;
		case XK_Escape: return TK_ESCAPE;
		case XK_space: return TK_SPACE;

		case XK_Page_Up: return TK_PAGEUP;
		case XK_Page_Down: return TK_PAGEDN;
		case XK_End: return TK_END;
		case XK_Home: return TK_HOME;
		case XK_Left: return TK_LEFT;
		case XK_Up: return TK_UP;
		case XK_Right: return TK_RIGHT;
		case XK_Down: return TK_DOWN;
		case XK_Insert: return TK_INSERT;
		case XK_Delete: return TK_DELETE;

		case XK_Meta_L: return TK_LWIN;
		case XK_Meta_R: return TK_RWIN;
		case XK_Num_Lock: return TK_NUMLOCK;
		case XK_Scroll_Lock: return TK_SCROLL;
		case XK_Shift_L: return TK_LSHIFT;
		case XK_Shift_R: return TK_RSHIFT;
		case XK_Control_L: return TK_LCONTROL;
		case XK_Control_R: return TK_RCONTROL;
		case XK_Alt_L: return TK_LALT;
		case XK_Alt_R: return TK_RALT;

		case XK_semicolon: return TK_SEMICOLON;
		case XK_equal: return TK_EQUALS;
		case XK_comma: return TK_COMMA;
		case XK_minus: return TK_MINUS;
		case XK_period: return TK_DOT;
		case XK_slash: return TK_SLASH;
		case XK_grave: return TK_BACKTICK;
		case XK_bracketleft: return TK_LSQUARE;
		case XK_backslash: return TK_BACKSLASH;
		case XK_bracketright: return TK_RSQUARE;
		case XK_apostrophe: return TK_TICK;
	}
	return 0;
}

static void tigrUpdateModifiers(TigrInternal *win) {
    win->keys[TK_SHIFT] = win->keys[TK_LSHIFT] || win->keys[TK_RSHIFT];
    win->keys[TK_CONTROL] = win->keys[TK_LCONTROL] || win->keys[TK_RCONTROL];
    win->keys[TK_ALT] = win->keys[TK_LALT] || win->keys[TK_RALT];
}

static void tigrInterpretChar(TigrInternal* win, Window root, unsigned int keycode, unsigned int mask) {
	XKeyEvent event;
	memset(&event, 0, sizeof(event));
	event.type = KeyPress;
	event.display = win->dpy;
	event.root = root;
	event.window = win->win;
	event.state = mask;
	event.keycode = keycode;
	char inputTextUTF8[10];
	Status status = 0;
	Xutf8LookupString(win->ic, &event, inputTextUTF8, sizeof(inputTextUTF8), NULL, &status);

	if(status == XLookupChars) {
		tigrDecodeUTF8(inputTextUTF8, &win->lastChar);
	}
}

static void tigrProcessInput(TigrInternal* win, int winWidth, int winHeight) {
	{
		Window focused;
		int revertTo;
		XGetInputFocus(win->dpy, &focused, &revertTo);

		if (win->win != focused) {
			return;
		}
	}

	Window root;
	Window child;
	int rootX;
	int rootY;
	int winX;
	int winY;
	unsigned int mask;

	if (XQueryPointer(win->dpy, win->win, &root, &child, &rootX, &rootY, &winX, &winY, &mask)) {
		static unsigned int prevButtons;
		unsigned int buttons = mask & (Button1Mask | Button2Mask | Button3Mask);

		win->mouseX = (winX - win->pos[0]) / win->scale;
		win->mouseY = (winY - win->pos[1]) / win->scale;

		if (buttons != prevButtons && (winX > 0 && winX < winWidth) && (winY > 0 && winY < winHeight)) {
			win->mouseButtons = 
				(buttons & Button1Mask) ? 1 : 0 |
				(buttons & Button3Mask) ? 2 : 0 |
				(buttons & Button2Mask) ? 4 : 0 ;
		}
		prevButtons = buttons;
	}

	static char prevKeys[32];
	char keys[32];
	XQueryKeymap(win->dpy, keys);
	for (int i = 0; i < 32; i++) {
		char thisBlock = keys[i];
		char prevBlock = prevKeys[i];
		if (thisBlock != prevBlock) {
			for (int j = 0; j < 8; j++) {
				int thisBit = thisBlock & 1;
				int prevBit = prevBlock & 1;
				thisBlock >>= 1;
				prevBlock >>= 1;
				if (thisBit != prevBit) {
					int keyCode = 8 * i + j;
					KeySym keySym = XkbKeycodeToKeysym(win->dpy, keyCode, 0, 0);
					if (keySym != NoSymbol) {
						int key = tigrKeyFromX11(keySym);
						win->keys[key] = thisBit;
						tigrUpdateModifiers(win);

						if (thisBit) {
							tigrInterpretChar(win, root, keyCode, mask);
						}
					}
				}
			}
		}
	}
	memcpy(prevKeys, keys, 32);

	XEvent event;
	while (XCheckTypedWindowEvent(win->dpy, win->win, ClientMessage, &event)) {
		if(event.xclient.data.l[0] == wmDeleteMessage) {
			glXMakeCurrent(win->dpy, None, NULL);
			glXDestroyContext(win->dpy, win->glc);
			XDestroyWindow(win->dpy, win->win);
			win->win = 0;
		}
	}
	XFlush(win->dpy);
}

void tigrUpdate(Tigr *bmp) {
	XWindowAttributes gwa;

	TigrInternal *win = tigrInternal(bmp);

	memcpy(win->prev, win->keys, 256);

	XGetWindowAttributes(win->dpy, win->win, &gwa);

	if (win->flags & TIGR_AUTO)
		tigrResize(bmp, gwa.width / win->scale, gwa.height / win->scale);
	else
		win->scale = tigrEnforceScale(tigrCalcScale(bmp->w, bmp->h, gwa.width, gwa.height), win->flags);

	tigrPosition(bmp, win->scale, gwa.width, gwa.height, win->pos);
	glXMakeCurrent(win->dpy, win->win, win->glc);
	tigrGAPIPresent(bmp, gwa.width, gwa.height);
	glXSwapBuffers(win->dpy, win->win);

	tigrProcessInput(win, gwa.width, gwa.height);
}

void tigrFree(Tigr *bmp) {
	if (bmp->handle)
	{
		TigrInternal *win = tigrInternal(bmp);
		if(win->win) {
	    	glXMakeCurrent(win->dpy, None, NULL);
        	glXDestroyContext(win->dpy, win->glc);
        	XDestroyWindow(win->dpy, win->win);
        	win->win = 0;
        }
	}
	free(bmp->pix);
	free(bmp);
}

void tigrError(Tigr *bmp, const char *message, ...)
{
	char tmp[1024];

	va_list args;
	va_start(args, message);
	vsnprintf(tmp, sizeof(tmp), message, args);
	tmp[sizeof(tmp)-1] = 0;
	va_end(args);

	printf("tigr fatal error: %s\n", tmp);

	exit(1);
}

float tigrTime()
{
	static double lastTime = 0;

	struct timeval tv;
	gettimeofday(&tv, NULL);

	double now = (double)tv.tv_sec + (tv.tv_usec / 1000000.0);
	double elapsed = lastTime == 0 ? 0 : now - lastTime;
	lastTime = now;

	return (float) elapsed;
}

void tigrMouse(Tigr *bmp, int *x, int *y, int *buttons)
{
	TigrInternal *win = tigrInternal(bmp);
	if(x) {
		*x = win->mouseX;
	}
	if(y) {
		*y = win->mouseY;
	}
	if(buttons) {
		*buttons = win->mouseButtons;
	}
}

int tigrTouch(Tigr *bmp, TigrTouchPoint* points, int maxPoints)
{
	int buttons = 0;
	if (maxPoints > 0) {
		tigrMouse(bmp, &points[0].x, &points[1].y, &buttons);
	}
	return buttons ? 1 : 0;
}

#endif // __linux__ && !__ANDROID__
