#ifndef __TIGR_UPSCALE_GL_FS_H__
#define __TIGR_UPSCALE_GL_FS_H__

#include "tigr_glsl_hdr.h"

const char tigr_upscale_gl_fs[] = {
    GLSL_VERSION_HEADER
    "in vec2 uv;"
    "out vec4 color;"
    "uniform sampler2D image;"
    "uniform vec4 parameters;"
    "void fxShader(out vec4 color, in vec2 coord);"
    "void main()"
    "{"
    "   fxShader(color, uv);"
    "}\n"
};

const int tigr_upscale_gl_fs_size = (int)sizeof(tigr_upscale_gl_fs) - 1;

const char tigr_default_fx_gl_fs[] = {
    "void fxShader(out vec4 color, in vec2 uv) {"
    "   vec2 tex_size = vec2(textureSize(image, 0));"
    "   vec2 uv_blur = mix(floor(uv * tex_size) + 0.5, uv * tex_size, parameters.xy) / tex_size;"
    "   vec4 c = texture(image, uv_blur);"
    "   c.rgb *= mix(0.5, 1.0 - fract(uv.y * tex_size.y), parameters.z) * 2.0; //scanline\n"
    "   c = mix(vec4(0.5), c, parameters.w); //contrast\n"
    "   color = c;"
    "}"
};

const int tigr_default_fx_gl_fs_size = (int)sizeof(tigr_default_fx_gl_fs) - 1;


#endif
