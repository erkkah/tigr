#include "tigr_internal.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef __ANDROID__

#ifdef __IOS__
void* _tigrReadFile(const char* fileName, int* length) {
#else
void* tigrReadFile(const char* fileName, int* length) {
#endif
    // TODO - unicode?
    FILE* file;
    char* data;
    size_t len;

    if (length)
        *length = 0;

    file = fopen(fileName, "rb");
    if (!file)
        return NULL;

    fseek(file, 0, SEEK_END);
    len = ftell(file);
    fseek(file, 0, SEEK_SET);

    data = (char*)malloc(len + 1);
    if (!data) {
        fclose(file);
        return NULL;
    }

    if (fread(data, 1, len, file) != len) {
        free(data);
        fclose(file);
        return NULL;
    }
    data[len] = '\0';
    fclose(file);

    if (length)
        *length = len;

    return data;
}

#endif  // __ANDROID__

// Reads a single UTF8 codepoint.
const char* tigrDecodeUTF8(const char* text, int* cp) {
    unsigned char c = *text++;
    int extra = 0, min = 0;
    *cp = 0;
    if (c >= 0xf0) {
        *cp = c & 0x07;
        extra = 3;
        min = 0x10000;
    } else if (c >= 0xe0) {
        *cp = c & 0x0f;
        extra = 2;
        min = 0x800;
    } else if (c >= 0xc0) {
        *cp = c & 0x1f;
        extra = 1;
        min = 0x80;
    } else if (c >= 0x80) {
        *cp = 0xfffd;
    } else {
        *cp = c;
    }
    while (extra--) {
        c = *text++;
        if ((c & 0xc0) != 0x80) {
            *cp = 0xfffd;
            break;
        }
        (*cp) = ((*cp) << 6) | (c & 0x3f);
    }
    if (*cp < min) {
        *cp = 0xfffd;
    }
    return text;
}

char* tigrEncodeUTF8(char* text, int cp) {
    if (cp < 0 || cp > 0x10ffff) {
        cp = 0xfffd;
    }

#define EMIT(X, Y, Z) *text++ = X | ((cp >> Y) & Z)
    if (cp < 0x80) {
        EMIT(0x00, 0, 0x7f);
    } else if (cp < 0x800) {
        EMIT(0xc0, 6, 0x1f);
        EMIT(0x80, 0, 0x3f);
    } else if (cp < 0x10000) {
        EMIT(0xe0, 12, 0xf);
        EMIT(0x80, 6, 0x3f);
        EMIT(0x80, 0, 0x3f);
    } else {
        EMIT(0xf0, 18, 0x7);
        EMIT(0x80, 12, 0x3f);
        EMIT(0x80, 6, 0x3f);
        EMIT(0x80, 0, 0x3f);
    }
    return text;
#undef EMIT
}

int tigrBeginOpenGL(Tigr* bmp) {
#ifdef TIGR_GAPI_GL
    TigrInternal* win = tigrInternal(bmp);
    win->gl.gl_user_opengl_rendering = 1;
    return tigrGAPIBegin(bmp) == 0;
#else
    return 0;
#endif
}

void tigrSetPostShader(Tigr* bmp, const char* code, int size) {
#ifdef TIGR_GAPI_GL
    tigrGAPIBegin(bmp);
    TigrInternal* win = tigrInternal(bmp);
    GLStuff* gl = &win->gl;
    tigrCreateShaderProgram(gl, code, size);
    tigrGAPIEnd(bmp);
#endif
}

void tigrSetPostFX(Tigr* bmp, float p1, float p2, float p3, float p4) {
    TigrInternal* win = tigrInternal(bmp);
    win->p1 = p1;
    win->p2 = p2;
    win->p3 = p3;
    win->p4 = p4;
}
