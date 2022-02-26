#include "tigr_internal.h"
#include <assert.h>

#ifdef TIGR_GAPI_GL
#if __linux__
#if __ANDROID__
#include <GLES3/gl3.h>
#else
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glext.h>
#endif
#endif
extern const char tigr_upscale_gl_vs[], tigr_upscale_gl_fs[], tigr_default_fx_gl_fs[];
extern const int tigr_upscale_gl_vs_size, tigr_upscale_gl_fs_size, tigr_default_fx_gl_fs_size;

#ifdef _WIN32

#ifdef TIGR_GAPI_GL_WIN_USE_GLEXT
#include <glext.h>
#include <wglext.h>
#else  // short version of glext.h and wglext.h so we don't need to depend on them
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY*
#endif
typedef ptrdiff_t GLsizeiptr;
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_BGRA 0x80E1
#define GL_TEXTURE0 0x84C0
typedef void(APIENTRYP PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint* arrays);
typedef void(APIENTRYP PFNGLGENBUFFERSARBPROC)(GLsizei n, GLuint* buffers);
typedef void(APIENTRYP PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void(APIENTRYP PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
typedef void(APIENTRYP PFNGLBINDVERTEXARRAYPROC)(GLuint array);
typedef void(APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void(APIENTRYP PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index,
                                                     GLint size,
                                                     GLenum type,
                                                     GLboolean normalized,
                                                     GLsizei stride,
                                                     const void* pointer);
typedef GLuint(APIENTRYP PFNGLCREATESHADERPROC)(GLenum type);
typedef char GLchar;
typedef void(APIENTRYP PFNGLSHADERSOURCEPROC)(GLuint shader,
                                              GLsizei count,
                                              const GLchar* const* string,
                                              const GLint* length);
typedef void(APIENTRYP PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef GLuint(APIENTRYP PFNGLCREATEPROGRAMPROC)(void);
typedef void(APIENTRYP PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void(APIENTRYP PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void(APIENTRYP PFNGLDELETESHADERPROC)(GLuint shader);
typedef void(APIENTRYP PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef void(APIENTRYP PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint* params);
typedef void(APIENTRYP PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
typedef void(APIENTRYP PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint* params);
typedef void(APIENTRYP PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
typedef void(APIENTRYP PFNGLUSEPROGRAMPROC)(GLuint program);
typedef GLint(APIENTRYP PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar* name);
typedef void(APIENTRYP PFNGLUNIFORM4FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void(APIENTRYP PFNGLUNIFORMMATRIX4FVPROC)(GLint location,
                                                  GLsizei count,
                                                  GLboolean transpose,
                                                  const GLfloat* value);
typedef void(APIENTRYP PFNGLACTIVETEXTUREPROC)(GLenum texture);
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023
#define WGL_TYPE_RGBA_ARB 0x202B
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
typedef BOOL(WINAPI* PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC hdc,
                                                     const int* piAttribIList,
                                                     const FLOAT* pfAttribFList,
                                                     UINT nMaxFormats,
                                                     int* piFormats,
                                                     UINT* nNumFormats);
typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC hDC, HGLRC hShareContext, const int* attribList);
#endif

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
int tigrGL11Init(Tigr* bmp) {
    int pixel_format;
    TigrInternal* win = tigrInternal(bmp);
    GLStuff* gl = &win->gl;
    PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR),
                                  1,
                                  PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SWAP_EXCHANGE,
                                  PFD_TYPE_RGBA,
                                  32,  // color bits
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  24,  // depth
                                  8,   // stencil
                                  0,
                                  PFD_MAIN_PLANE,  // is it ignored ?
                                  0,
                                  0,
                                  0,
                                  0 };
    if (!(gl->dc = GetDC((HWND)bmp->handle))) {
        tigrError(bmp, "Cannot create OpenGL device context.\n");
        return -1;
    }
    if (!(pixel_format = ChoosePixelFormat(gl->dc, &pfd))) {
        tigrError(bmp, "Cannot choose OpenGL pixel format.\n");
        return -1;
    }
    if (!SetPixelFormat(gl->dc, pixel_format, &pfd)) {
        tigrError(bmp, "Cannot set OpenGL pixel format.\n");
        return -1;
    }
    if (!(gl->hglrc = wglCreateContext(gl->dc))) {
        tigrError(bmp, "Cannot create OpenGL context.\n");
        return -1;
    }
    if (!wglMakeCurrent(gl->dc, gl->hglrc)) {
        tigrError(bmp, "Cannot activate OpenGL context.\n");
        return -1;
    }
    gl->gl_legacy = 1;
    return 0;
}
int tigrGL33Init(Tigr* bmp) {
    int pixel_format;
    UINT num_formats;
    TigrInternal* win = tigrInternal(bmp);
    GLStuff* gl = &win->gl;

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

    if (!wglChoosePixelFormat || !wglCreateContextAttribs) {
        tigrError(bmp, "Cannot create OpenGL context.\n");
        return -1;
    }
    const int attribList[] = { WGL_DRAW_TO_WINDOW_ARB,
                               GL_TRUE,
                               WGL_SUPPORT_OPENGL_ARB,
                               GL_TRUE,
                               WGL_DOUBLE_BUFFER_ARB,
                               GL_TRUE,
                               WGL_PIXEL_TYPE_ARB,
                               WGL_TYPE_RGBA_ARB,
                               WGL_COLOR_BITS_ARB,
                               32,
                               WGL_DEPTH_BITS_ARB,
                               24,
                               WGL_STENCIL_BITS_ARB,
                               8,
                               0 };
    int attribs[] = { WGL_CONTEXT_MAJOR_VERSION_ARB, 3, WGL_CONTEXT_MINOR_VERSION_ARB, 3, 0 };
    if (!wglChoosePixelFormat(gl->dc, attribList, NULL, 1, &pixel_format, &num_formats)) {
        tigrError(bmp, "Cannot choose OpenGL pixel format.\n");
        return -1;
    }
    if (!(gl->hglrc = wglCreateContextAttribs(gl->dc, gl->hglrc, attribs))) {
        tigrError(bmp, "Cannot create OpenGL context attribs.\n");
        return -1;
    }
    if (!wglMakeCurrent(gl->dc, gl->hglrc)) {
        tigrError(bmp, "Cannot activate OpenGL context.\n");
        return -1;
    }
    gl->gl_legacy = 0;
    return 0;
}
#endif

void tigrCheckGLError(const char* state) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        tigrError(NULL, "got GL error %x when doing %s\n", err, state);
    }
}

void tigrCheckShaderErrors(GLuint object) {
    GLint success;
    GLchar info[2048];
    glGetShaderiv(object, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(object, sizeof(info), NULL, info);
        tigrError(NULL, "shader compile error : %s\n", info);
    }
}

void tigrCheckProgramErrors(GLuint object) {
    GLint success;
    GLchar info[2048];
    glGetProgramiv(object, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(object, sizeof(info), NULL, info);
        tigrError(NULL, "shader link error : %s\n", info);
    }
}

void tigrCreateShaderProgram(GLStuff* gl, const char* fxSource, int fxSize) {
    if (gl->program != 0) {
        glDeleteProgram(gl->program);
        gl->program = 0;
    }

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    const char* vs_source = (const char*)&tigr_upscale_gl_vs;
    glShaderSource(vs, 1, &vs_source, &tigr_upscale_gl_vs_size);
    glCompileShader(vs);
    tigrCheckShaderErrors(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fs_sources[] = {
        (const char*)tigr_upscale_gl_fs,
        fxSource,
    };
    const int fs_lengths[] = {
        tigr_upscale_gl_fs_size,
        fxSize,
    };
    glShaderSource(fs, 2, fs_sources, fs_lengths);
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

void tigrGAPICreate(Tigr* bmp) {
    TigrInternal* win = tigrInternal(bmp);
    GLStuff* gl = &win->gl;
    GLuint VBO;
    GLfloat vertices[] = { // pos      uv
                           0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                           0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f
    };

#ifdef _WIN32
    if (tigrGL11Init(bmp))
        return;
    tigrGL33Init(bmp);
#endif

    if (!gl->gl_legacy) {
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
        tigrCreateShaderProgram(gl, tigr_default_fx_gl_fs, tigr_default_fx_gl_fs_size);
    }

    // create textures
    if (gl->gl_legacy) {
        glEnable(GL_TEXTURE_2D);
    }
    glGenTextures(2, gl->tex);
    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, gl->tex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl->gl_legacy ? GL_NEAREST : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl->gl_legacy ? GL_NEAREST : GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }

    tigrCheckGLError("initialization");
}

void tigrGAPIDestroy(Tigr* bmp) {
    TigrInternal* win = tigrInternal(bmp);
    GLStuff* gl = &win->gl;

    if (tigrGAPIBegin(bmp) < 0) {
        tigrError(bmp, "Cannot activate OpenGL context.\n");
        return;
    }

    if (!gl->gl_legacy) {
        glDeleteTextures(2, gl->tex);
        glDeleteProgram(gl->program);
    }

    tigrCheckGLError("destroy");

    if (tigrGAPIEnd(bmp) < 0) {
        tigrError(bmp, "Cannot deactivate OpenGL context.\n");
        return;
    }
}

void tigrGAPIDraw(int legacy, GLuint uniform_model, GLuint tex, Tigr* bmp, int x1, int y1, int x2, int y2) {
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, bmp->w, bmp->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, bmp->pix);

    if (!legacy) {
        float sx = (float)(x2 - x1);
        float sy = (float)(y2 - y1);
        float tx = (float)x1;
        float ty = (float)y1;

        float model[16] = { sx, 0.0f, 0.0f, 0.0f, 0.0f, sy, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, tx, ty, 0.0f, 1.0f };

        glUniformMatrix4fv(uniform_model, 1, GL_FALSE, model);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    } else {
#if !(__APPLE__ || __ANDROID__)
        glBegin(GL_QUADS);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2i(x2, y1);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2i(x1, y1);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2i(x1, y2);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2i(x2, y2);
        glEnd();
#else
        assert(0);
#endif
    }
}

void tigrGAPIPresent(Tigr* bmp, int w, int h) {
    TigrInternal* win = tigrInternal(bmp);
    GLStuff* gl = &win->gl;

    glViewport(0, 0, w, h);
    if (!gl->gl_user_opengl_rendering) {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    if (!gl->gl_legacy) {
        float projection[16] = { 2.0f / w, 0.0f, 0.0f, 0.0f, 0.0f,  -2.0f / h, 0.0f, 0.0f,
                                 0.0f,     0.0f, 1.0f, 0.0f, -1.0f, 1.0f,      0.0f, 1.0f };

        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(gl->vao);
        glUseProgram(gl->program);
        glUniformMatrix4fv(gl->uniform_projection, 1, GL_FALSE, projection);
        glUniform4f(gl->uniform_parameters, win->p1, win->p2, win->p3, win->p4);
    } else {
#if !(__APPLE__ || __ANDROID__)
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, w, h, 0, -1.0f, 1.0f);
        glEnable(GL_TEXTURE_2D);
#else
        assert(0);
#endif
    }

    if (gl->gl_user_opengl_rendering) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }
    tigrGAPIDraw(gl->gl_legacy, gl->uniform_model, gl->tex[0], bmp, win->pos[0], win->pos[1], win->pos[2], win->pos[3]);

    if (win->widgetsScale > 0) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        tigrGAPIDraw(gl->gl_legacy, gl->uniform_model, gl->tex[1], win->widgets,
                     (int)(w - win->widgets->w * win->widgetsScale), 0, w, (int)(win->widgets->h * win->widgetsScale));
    }

    tigrCheckGLError("present");

    gl->gl_user_opengl_rendering = 0;
}

#endif
