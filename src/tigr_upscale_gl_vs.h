#ifndef __TIGR_UPSCALE_GL_VS_H__
#define __TIGR_UPSCALE_GL_VS_H__

#include "tigr_glsl_hdr.h"

const char tigr_upscale_gl_vs[] = {
    GLSL_VERSION_HEADER
    "layout (location = 0) in vec2 pos_in;"
    "layout (location = 1) in vec2 uv_in;"
    "out vec2 uv;"
    "uniform mat4 model;"
    "uniform mat4 projection;"
    "void main()"
    "{"
    "   uv = uv_in;"
    "   gl_Position = projection * model * vec4(pos_in, 0.0, 1.0);"
    "}"
};

const int tigr_upscale_gl_vs_size = (int)sizeof(tigr_upscale_gl_vs) - 1;

#endif
