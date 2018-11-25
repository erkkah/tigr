#include "tigr_internal.h"
#include <stdio.h> // TODO can we remove this and printf's later?

#ifdef TIGR_GAPI_GL
//#ifndef __APPLE__
// please provide you own glext.h, you can download latest at https://www.opengl.org/registry/api/GL/glext.h
//#include <glext.h>
//#endif
#ifdef _WIN32
// please provide you own wglext.h, you can download latest at https://www.opengl.org/registry/api/GL/wglext.h
#include <wglext.h>
#endif
#ifdef __linux__
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glext.h>
#endif
extern const unsigned char tigr_upscale_gl_vs[], tigr_upscale_gl_fs[];
extern int tigr_upscale_gl_vs_size, tigr_upscale_gl_fs_size;

#ifdef _WIN32
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormat;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribs;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLGENBUFFERSARBPROC glGenBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLUNIFORM4FPROC glUniform4f;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
PFNGLACTIVETEXTUREPROC glActiveTexture;
int tigrGL11Init(Tigr *bmp)
{
	int pixel_format;
	TigrInternal *win = tigrInternal(bmp);
	GLStuff *gl= &win->gl;
	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SWAP_EXCHANGE,
		PFD_TYPE_RGBA,
		32, // color bits
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0,
		24, // depth
		8,  // stencil
		0,
		PFD_MAIN_PLANE, // is it ignored ?
		0,
		0, 0, 0
	};
	if(!(gl->dc = GetDC((HWND)bmp->handle))) {tigrError(bmp, "Cannot create OpenGL device context.\n"); return -1;}
	if(!(pixel_format = ChoosePixelFormat(gl->dc, &pfd))) {tigrError(bmp, "Cannot choose OpenGL pixel format.\n"); return -1;}
	if(!SetPixelFormat(gl->dc, pixel_format, &pfd)) {tigrError(bmp, "Cannot set OpenGL pixel format.\n"); return -1;}
	if(!(gl->hglrc = wglCreateContext(gl->dc))) {tigrError(bmp, "Cannot create OpenGL context.\n"); return -1;}
	if(!wglMakeCurrent(gl->dc, gl->hglrc)) {tigrError(bmp, "Cannot activate OpenGL context.\n"); return -1;}
	gl->gl_legacy = 1;
	return 0;
}
int tigrGL33Init(Tigr *bmp)
{
	int pixel_format;
	UINT num_formats;
	TigrInternal *win = tigrInternal(bmp);
	GLStuff *gl= &win->gl;

	wglChoosePixelFormat = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	wglCreateContextAttribs = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
	glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
	glGenBuffers = (PFNGLGENBUFFERSARBPROC)wglGetProcAddress("glGenBuffers");
	glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
	glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
	glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
	glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
	glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
	glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
	glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
	glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
	glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
	glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
	glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
	glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
	glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
	glUniform4f = (PFNGLUNIFORM4FPROC)wglGetProcAddress("glUniform4f");
	glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv");
	glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");

	if(!wglChoosePixelFormat || !wglCreateContextAttribs) {tigrError(bmp, "Cannot create OpenGL context.\n"); return -1;}
	const int attribList[] =
	{
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB, 32,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		0
	};
	int attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		0
	};
	if(!wglChoosePixelFormat(gl->dc, attribList, NULL, 1, &pixel_format, &num_formats)) {tigrError(bmp, "Cannot choose OpenGL pixel format.\n"); return -1;}
	if(!(gl->hglrc = wglCreateContextAttribs(gl->dc, gl->hglrc, attribs))) {tigrError(bmp, "Cannot create OpenGL context attribs.\n"); return -1;}
	if(!wglMakeCurrent(gl->dc, gl->hglrc)) {tigrError(bmp, "Cannot activate OpenGL context.\n"); return -1;}
	gl->gl_legacy = 0;
	return 0;
}
#endif

void tigrCheckGLError(const char *state)
{
	GLenum err = glGetError();
	if(err != GL_NO_ERROR)
		printf("got gl error %x when was doing %s\n", err, state);
}

void tigrCheckShaderErrors(GLuint object)
{
	GLint success;
	GLchar info[2048];
	glGetShaderiv(object, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(object, sizeof(info), NULL, info);
		printf("shader compile error : %s\n", info);
	}
}

void tigrCheckProgramErrors(GLuint object)
{
	GLint success;
	GLchar info[2048];
	glGetProgramiv(object, GL_LINK_STATUS, &success);
	if(!success)
	{
		glGetProgramInfoLog(object, sizeof(info), NULL, info);
		printf("shader link error : %s\n", info);
	}
}

void tigrGAPICreate(Tigr *bmp)
{
	GLuint vs, fs;
	TigrInternal *win = tigrInternal(bmp);
	GLStuff *gl= &win->gl;
	GLuint VBO;
	GLfloat vertices[] = {
		// pos      uv
		0.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 1.0f, 0.0f
	};

	#ifdef _WIN32
	if(tigrGL11Init(bmp))
		return;
	tigrGL33Init(bmp);
	#endif

	//printf("ogl version %s\n", glGetString(GL_VERSION));

	if(!gl->gl_legacy)
	{
		// create vao
		glGenVertexArrays(1, &gl->vao);
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glBindVertexArray(gl->vao);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), NULL);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), NULL);

		// create program
		vs = glCreateShader(GL_VERTEX_SHADER);
		const char *vs_source = (const char*)&tigr_upscale_gl_vs;
		glShaderSource(vs, 1, &vs_source, &tigr_upscale_gl_vs_size);
		glCompileShader(vs);
		tigrCheckShaderErrors(vs);
		fs = glCreateShader(GL_FRAGMENT_SHADER);
		const char *fs_source = (const char*)&tigr_upscale_gl_fs;
		glShaderSource(fs, 1, &fs_source, &tigr_upscale_gl_fs_size);
		glCompileShader(fs);
		tigrCheckShaderErrors(fs);
		gl->program = glCreateProgram();
		glAttachShader(gl->program, vs);
		glAttachShader(gl->program, fs);
		glLinkProgram(gl->program);
		tigrCheckProgramErrors(gl->program);
		glDeleteShader(vs);
		glDeleteShader(fs);
		gl->uniform_projection = glGetUniformLocation(gl->program, "projection");
		gl->uniform_model = glGetUniformLocation(gl->program, "model");
		gl->uniform_parameters = glGetUniformLocation(gl->program, "parameters");
	}

	// create textures
	if(gl->gl_legacy)
		glEnable(GL_TEXTURE_2D);
	glGenTextures(2, gl->tex);
	for(int i = 0; i < 2; ++i) {
		glBindTexture(GL_TEXTURE_2D, gl->tex[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl->gl_legacy ? GL_NEAREST : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl->gl_legacy ? GL_NEAREST : GL_LINEAR);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	}

	tigrCheckGLError("initialization");
}

void tigrGAPIBegin(Tigr *bmp)
{
}

void tigrGAPIEnd(Tigr *bmp)
{
}

void tigrGAPIDestroy(Tigr *bmp)
{
	TigrInternal *win = tigrInternal(bmp);
	GLStuff *gl= &win->gl;

	if(!gl->gl_legacy)
	{
		glDeleteTextures(2, gl->tex);
		glDeleteProgram(gl->program);
	}

	tigrCheckGLError("destroy");

	#ifdef _WIN32
	if(!wglMakeCurrent(NULL, NULL)) {tigrError(bmp, "Cannot deactivate OpenGL context.\n"); return;}
	if(gl->hglrc && !wglDeleteContext(gl->hglrc)) {tigrError(bmp, "Cannot delete OpenGL context.\n"); return;}
	gl->hglrc = NULL;

	if(gl->dc && !ReleaseDC((HWND)bmp->handle, gl->dc)) {tigrError(bmp, "Cannot release OpenGL device context.\n"); return;}
	gl->dc = NULL;
	#endif
}

void tigrGAPIResize(Tigr *bmp, int width, int height)
{
	// no-op
	(void)bmp;
	(void)width;
	(void)height;
}

void tigrGAPIDraw(int legacy, GLuint uniform_model, GLuint tex, Tigr *bmp, int x1, int y1, int x2, int y2)
{
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, bmp->w, bmp->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, bmp->pix);

	if(!legacy)
	{
		float sx = (float)(x2 - x1);
		float sy = (float)(y2 - y1);
		float tx = (float)x1;
		float ty = (float)y1;

		float model[16] =
		{
			  sx, 0.0f, 0.0f, 0.0f,
			0.0f,   sy, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			  tx,   ty, 0.0f, 1.0f
		};

		glUniformMatrix4fv(uniform_model, 1, GL_FALSE, model);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	else
	{
		#ifndef __APPLE__
		glBegin(GL_QUADS);
		glTexCoord2f(1.0f, 0.0f); glVertex2i(x2, y1);
		glTexCoord2f(0.0f, 0.0f); glVertex2i(x1, y1);
		glTexCoord2f(0.0f, 1.0f); glVertex2i(x1, y2);
		glTexCoord2f(1.0f, 1.0f); glVertex2i(x2, y2);
		glEnd();
		#else
		assert(0);
		#endif
	}
}

void tigrGAPIPresent(Tigr *bmp, int w, int h)
{
	TigrInternal *win = tigrInternal(bmp);
	GLStuff *gl= &win->gl;

#ifdef _WIN32
	wglMakeCurrent(gl->dc, gl->hglrc);
#endif
#ifdef __linux__
	glXMakeCurrent(win->dpy, win->win, win->glc);
#endif
	glViewport(0, 0, w, h);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	if(!gl->gl_legacy)
	{
		float projection[16] =
		{
			 2.0f / w,  0.0f    , 0.0f, 0.0f,
			 0.0f    , -2.0f / h, 0.0f, 0.0f,
			 0.0f    ,  0.0f    , 1.0f, 0.0f,
			-1.0f    ,  1.0f    , 0.0f, 1.0f
		};

		glActiveTexture(GL_TEXTURE0);
		glBindVertexArray(gl->vao);
		glUseProgram(gl->program);
		glUniformMatrix4fv(gl->uniform_projection, 1, GL_FALSE, projection);
		glUniform4f(gl->uniform_parameters, win->hblur ? 1.0f : 0.0f, win->vblur ? 1.0f : 0.0f, win->scanlines, win->contrast);
	}
	else
	{
		#ifndef __APPLE__
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, w, h, 0, -1.0f, 1.0f);
		glEnable(GL_TEXTURE_2D);
		#else
		assert(0);
		#endif
	}

	glDisable(GL_BLEND);
	tigrGAPIDraw(gl->gl_legacy, gl->uniform_model, gl->tex[0], bmp, win->pos[0], win->pos[1], win->pos[2], win->pos[3]);

	if(win->widgetsScale > 0)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		tigrGAPIDraw(gl->gl_legacy, gl->uniform_model, gl->tex[1], win->widgets, 
			(int)(w - win->widgets->w * win->widgetsScale), 0, 
			w, (int)(win->widgets->h * win->widgetsScale));
	}

	tigrCheckGLError("present");

	#ifdef _WIN32
	if(!SwapBuffers(gl->dc)) {tigrError(bmp, "Cannot swap OpenGL buffers.\n"); return;}
	#endif

	#ifdef __linux__
	glXSwapBuffers(win->dpy, win->win);
	#endif
}

#endif
