#ifndef __TIGR_UPSCALE_GL_VS_H__
#define __TIGR_UPSCALE_GL_VS_H__

const unsigned char tigr_upscale_gl_vs[] = {
#if __ANDROID__
    "#version 300 es\n"
    "precision mediump float;\n"
#else
    "#version 330 core\n"
#endif
    "\n"
    "layout (location = 0) in vec2 pos_in;\n"
    "layout (location = 1) in vec2 uv_in;\n"
    "\n"
    "out vec2 uv;\n"
    "\n"
    "uniform mat4 model;\n"
    "uniform mat4 projection;\n"
    "\n"
    "void main()\n"
    "{\n"
    "   uv = uv_in;\n"
    "   gl_Position = projection * model * vec4(pos_in, 0.0, 1.0);\n"
    "}\n"
};

int tigr_upscale_gl_vs_size = (int)sizeof(tigr_upscale_gl_vs) - 1;

#endif
