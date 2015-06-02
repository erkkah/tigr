/*
This is free and unencumbered software released into the public domain.

Our intent is that anyone is free to copy and use this software,
for any purpose, in any form, and by any means.

The authors dedicate any and all copyright interest in the software
to the public domain, at their own expense for the betterment of mankind.

The software is provided "as is", without any kind of warranty, including
any implied warranty. If it breaks, you get to keep both pieces.
*/

#include "tigr_bitmaps.c"
#include "tigr_loadpng.c"
#include "tigr_savepng.c"
#include "tigr_utils.c"
#include "tigr_inflate.c"
#include "tigr_print.c"

#ifdef _WIN32
#include "tigr_win.c"
#include "tigr_upscale_vs.h"
#include "tigr_upscale_ps.h"
#endif // _WIN32
