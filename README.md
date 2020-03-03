# README

TIGR is a tiny graphics library, for when you just need to draw something in a window without any fuss. TIGR doesn't want to do everything. We don't want to bloat your program with hundreds of extra DLLs and megabytes of frameworks.

TIGR is free to copy with no restrictions; see tigr.h

> NOTE: This repo contains a fork of TIGR with added Linux support. The original repo lives [here](https://bitbucket.org/rmitton/tigr/overview).

We don't want to supply every possible function you might ever need. There are already plenty of add-on libraries for doing sound, XML, 3D, whatever. Our goal is simply to allow you to easily throw together small 2D programs when you need them.

TIGR's core is a simple framebuffer library. On top of that, we provide a few helpers for the common tasks that 2D programs generally need:

 - Create bitmap windows.
 - Direct access to bitmaps, no locking.
 - Basic drawing helpers (plot, line, blitter).
 - Text output.
 - PNG loading and saving.

TIGR is designed to be small and independent. A typical 'hello world' is less than 40KB. We don't require you to distribute any additional DLLs; everything is baked right into your program.

TIGR is cross platform, providing a unified API for Windows, OSX and Linux.

### How to set up TIGR ###

TIGR is supplied as a single .h file.

To use it, you just drop them right into your project. No fancy build systems, no DLL hell, no package managers.

1. Grab  **tigr.c** and **tigr.h**
2. Throw them into your project.
3. Link with
    - -lopengl32 and -lgdi32 on Windows
    - -framework OpenGL and -framework Cocoa on OSX
    - -lGLU -lGL -lX11 on Linux
4. You're done!

### How do I program with TIGR? ###

Here's an example Hello World program. For more information, just read **tigr.h** to see the APIs available.

```
#include "tigr.h"

int main(int argc, char *argv[])
{
    Tigr *screen = tigrWindow(320, 240, "Hello", 0);
    while (!tigrClosed(screen))
    {
        tigrClear(screen, tigrRGB(0x80, 0x90, 0xa0));
        tigrPrint(screen, tfont, 120, 110, tigrRGB(0xff, 0xff, 0xff), "Hello, world.");
        tigrUpdate(screen);
    }
    tigrFree(screen);
    return 0;
}
```
