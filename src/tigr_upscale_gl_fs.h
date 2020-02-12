#ifndef __TIGR_UPSCALE_GL_FS_H__
#define __TIGR_UPSCALE_GL_FS_H__

const unsigned char tigr_upscale_gl_fs[] = {
   "#version 330 core\n"
   "\n"
   "in vec2 uv;\n"
   "\n"
   "out vec4 color;\n"
   "\n"
   "uniform sampler2D image;\n"
   "uniform vec4 parameters;\n"
   "\n"
   "void main()\n"
   "{\n"
   "   vec2 tex_size = textureSize(image, 0);\n"
   "   vec2 uv_blur = mix(floor(uv * tex_size) + 0.5, uv * tex_size, parameters.xy) / tex_size;\n"
   "   vec4 c = texture(image, uv_blur);\n"
   "   c.rgb *= mix(0.5, 1.0 - fract(uv.y * tex_size.y), parameters.z) * 2.0; //scanline\n"
   "   c = mix(vec4(0.5), c, parameters.w); //contrast \n"
   "   color = c;\n"
   "}\n"
};

int tigr_upscale_gl_fs_size = (int)sizeof(tigr_upscale_gl_fs) - 1;

#endif
