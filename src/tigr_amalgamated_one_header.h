/*
Like most of stb libraries, this include needs to be included in one
compilation unit with TIGR_IMPLEMENTATION defined.
*/

#include "../tigr.h"

#ifdef TIGR_IMPLEMENTATION
#ifdef __cplusplus
extern "C" {
#endif

#include "tigr_amalgamated.c"

#ifdef __cplusplus
}
#endif
#endif