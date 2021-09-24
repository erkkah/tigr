// can't use pragma once here because this file probably will endup in .c
#ifndef __TIGR_INTERNAL_H__
#define __TIGR_INTERNAL_H__

#define _CRT_SECURE_NO_WARNINGS NOPE

// Graphics configuration.
#define TIGR_GAPI_GL

// Creates a new bitmap, with extra payload bytes.
Tigr *tigrBitmap2(int w, int h, int extra);

// Resizes an existing bitmap.
void tigrResize(Tigr *bmp, int w, int h);

// Calculates the biggest scale that a bitmap can fit into an area at.
int tigrCalcScale(int bmpW, int bmpH, int areaW, int areaH);

// Calculates a new scale, taking minimum-scale flags into account.
int tigrEnforceScale(int scale, int flags);

// Calculates the correct position for a bitmap to fit into a window.
void tigrPosition(Tigr *bmp, int scale, int windowW, int windowH, int out[4]);

// ----------------------------------------------------------
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if __linux__ && !__ANDROID__
#include<X11/X.h>
#include<X11/Xlib.h>
#endif

#ifdef __APPLE__
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE
#define __IOS__ 1
#else
#define __MACOS__ 1
#endif
#endif

#ifdef TIGR_GAPI_GL
#if __MACOS__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#endif
#ifdef _WIN32
#include <GL/gl.h>
#endif
#if __linux__ && !__ANDROID__
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include<GL/glx.h>
#endif
#if __ANDROID__
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#endif
#if __IOS__
#define GLES_SILENCE_DEPRECATION
#include <OpenGLES/ES3/gl.h>
#endif

typedef struct {
	#ifdef _WIN32
	HGLRC hglrc;
	HDC dc;
	#endif
	#ifdef __APPLE__
	void *glContext;
	#endif
	GLuint tex[2];
	GLuint vao;
	GLuint program;
	GLuint uniform_projection;
	GLuint uniform_model;
	GLuint uniform_parameters;
	int gl_legacy;
	int gl_user_opengl_rendering;
} GLStuff;
#endif

#define MAX_TOUCH_POINTS 10

typedef struct {
	int shown, closed;
	#ifdef TIGR_GAPI_GL
	GLStuff gl;
	#endif

	#ifdef _WIN32
	wchar_t *wtitle;
	DWORD dwStyle;
	RECT oldPos;
	#endif
	#ifdef __linux__
	#if __ANDROID__
    EGLContext context;
	#else
	Display *dpy;
	Window win;
	GLXContext glc;
	XIC ic;
	#endif // __ANDROID__
	#endif // __linux__
	
	Tigr *widgets;
	int widgetsWanted;
	unsigned char widgetAlpha;
	float widgetsScale;

	float p1, p2, p3, p4;

	int flags;
	int scale;
	int pos[4];
	int lastChar;
	char keys[256], prev[256];
	#if defined(__MACOS__)
	int mouseInView;
	int mouseButtons;
	#endif
	#if defined(__linux__) || defined(__IOS__)
	int mouseButtons;
	int mouseX;
	int mouseY;
	#endif // __linux__ __IOS__
	#if defined(__ANDROID__) || defined(__IOS__)
	int numTouchPoints;
	TigrTouchPoint touchPoints[MAX_TOUCH_POINTS];
	#endif // __ANDROID__ __IOS__
} TigrInternal;
// ----------------------------------------------------------

TigrInternal *tigrInternal(Tigr *bmp);

void tigrGAPICreate(Tigr *bmp);
void tigrGAPIDestroy(Tigr *bmp);
int  tigrGAPIBegin(Tigr *bmp);
int  tigrGAPIEnd(Tigr *bmp);
void tigrGAPIPresent(Tigr *bmp, int w, int h);

#endif

