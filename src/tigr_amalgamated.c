
#include "tigr.h"

#include "tigr_bitmaps.c"
#include "tigr_loadpng.c"
#include "tigr_savepng.c"
#include "tigr_utils.c"
#include "tigr_inflate.c"
#include "tigr_print.c"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "tigr_upscale_d3d9_vs.h"
#include "tigr_upscale_d3d9_ps.h"
#include "tigr_d3d9.c"
#endif // _WIN32

#include "tigr_upscale_gl_vs.h"
#include "tigr_upscale_gl_fs.h"
#include "tigr_win.c"
#include "tigr_osx.c"
#include "tigr_linux.c"
#include "tigr_gl.c"
