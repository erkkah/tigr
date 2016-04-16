// can't use pragma once here because this file probably will endup in .c
#ifndef __TIGR_INTERNAL_H__
#define __TIGR_INTERNAL_H__

#define _CRT_SECURE_NO_WARNINGS NOPE

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

#ifdef TIGR_GAPI_D3D9
#include <d3d9.h>
typedef struct {
	int lost;
	IDirect3DDevice9 *dev;
	D3DPRESENT_PARAMETERS params;
	IDirect3DTexture9 *sysTex[2], *vidTex[2];
	IDirect3DVertexShader9 *vs;
	IDirect3DPixelShader9 *ps;
} D3D9Stuff;
#endif

#ifdef TIGR_GAPI_GL
#ifdef _WIN32
#include <GL/gl.h>
#else
#include <OpenGL/gl3.h>
#endif
typedef struct {
	#ifdef _WIN32
	HGLRC hglrc;
	HDC dc;
	#endif
	GLuint tex[2];
	GLuint vao;
	GLuint program;
	GLuint uniform_projection;
	GLuint uniform_model;
	GLuint uniform_parameters;
	int gl_legacy;
} GLStuff;
#endif

typedef struct {
	int shown, closed;
	#ifdef TIGR_GAPI_D3D9
	D3D9Stuff d3d9;
	#endif
	#ifdef TIGR_GAPI_GL
	GLStuff gl;
	#endif

	#ifdef _WIN32
	wchar_t *wtitle;
	DWORD dwStyle;
	RECT oldPos;
	#endif
	#ifdef __APPLE__
	void *glContext;
	#endif

	Tigr *widgets;
	int widgetsWanted;
	unsigned char widgetAlpha;
	float widgetsScale;

	int hblur, vblur;
	float scanlines, contrast;

	int flags;
	int scale;
	int pos[4];
	int lastChar;
	char keys[256], prev[256];
	#ifdef __APPLE__
	int mouseButtons;
	#endif
} TigrInternal;
// ----------------------------------------------------------

TigrInternal *tigrInternal(Tigr *bmp);

void tigrGAPICreate(Tigr *bmp);
void tigrGAPIDestroy(Tigr *bmp);
void tigrGAPIBegin(Tigr *bmp);
void tigrGAPIEnd(Tigr *bmp);
void tigrGAPIResize(Tigr *bmp, int width, int height);
void tigrGAPIPresent(Tigr *bmp, int w, int h);

#endif

