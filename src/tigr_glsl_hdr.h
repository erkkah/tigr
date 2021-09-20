#ifndef __TIGR_GLSL_HDR_H__
#define __TIGR_GLSL_HDR_H__

#if __ANDROID__ || __IOS__
#define GLSL_VERSION_HEADER \
    "#version 300 es\n" \
    "precision mediump float;\n"
#else
#define GLSL_VERSION_HEADER \
    "#version 330 core\n"
#endif

#endif // __TIGR_GLSL_HDR_H__
