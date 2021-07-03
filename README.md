# TIGR - TIny GRaphics library

![](./tigr.png)

TIGR is a tiny graphics library, for when you just need to draw something in a window without any fuss. TIGR doesn't want to do everything. We don't want to bloat your program with hundreds of extra DLLs and megabytes of frameworks.

We don't want to supply every possible function you might ever need. There are already plenty of add-on libraries for doing sound, json, 3D, whatever.
Our goal is simply to allow you to easily throw together small 2D programs when you need them.

TIGR's core is a simple framebuffer library. On top of that, we provide a few helpers for the common tasks that 2D programs generally need:

 - Create bitmap windows.
 - Direct access to bitmaps, no locking.
 - Basic drawing helpers (plot, line, blitter).
 - Text output.
 - Mouse, touch and keyboard input.
 - PNG loading and saving.
 - Easy pixel shader access.

TIGR is designed to be small and independent. A typical 'hello world' is less than 40KB. We don't require you to distribute any additional DLLs; everything is baked right into your program.

TIGR is cross platform, providing a unified API for Windows, macOS, Linux and Android.

TIGR is free to copy with no restrictions; see tigr.h.

> NOTE: This repo contains a fork of TIGR with added Linux and Android support. The original repo lives [here](https://bitbucket.org/rmitton/tigr/overview).

## How to set up TIGR

### Desktop (Windows, macOS, Linux)

TIGR is supplied as a single .c and corresponding .h file.

To use it, you just drop them right into your project. No fancy build systems, no DLL hell, no package managers.

1. Grab  **tigr.c** and **tigr.h**
2. Throw them into your project.
3. Link with
    - -lopengl32 and -lgdi32 on Windows
    - -framework OpenGL and -framework Cocoa on macOS
    - -lGLU -lGL -lX11 on Linux
4. You're done!

### Android

Due to the complex lifecycle and packaging of Android apps
(there is no such thing as a single source file Android app),
a tiny wrapper around TIGR is needed. Still - the TIGR API stays the same!

To keep TIGR as tiny and focused as it is, the Android wrapper lives in a separate repo.

To get started on Android, head over to the [TIMOGR](https://github.com/erkkah/timogr) repo and continue there.
TIGR is included in TIMOGR, there is no need to install TIGR separately.

### How do I program with TIGR? ###

Here's an example Hello World program. For more information, just read **tigr.h** to see the APIs available.

```C
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

### Custom fonts

TIGR comes with a built-in bitmap font, accessed by the `tfont` variable. Custom fonts can be loaded from bitmaps using [`tigrLoadFont`](tigr.h#L149). A font bitmap contains rows of characters separated by same-colored borders. TIGR assumes that the borders use the same color as the top-left pixel in the bitmap. Each character is assumed to be drawn in white on a transparent background to make tinting work.

### Custom pixel shaders

TIGR uses a built-in pixel shader that provides a couple of stock effects as controlled by [`tigrSetPostFX`](tigr.h#L84).
These stock effects can be replaced by calling [`tigrSetPostShader`](tigr.h#L75) with a custom shader.
The custom shader is in the form of a shader function: `void fxShader(out vec4 color, in vec2 uv)` and has access to the four parameters from `tigrSetPostFX` as a `uniform vec4` called `parameters`.

See the [shader example](examples/shader/shader.c) for more details.

## Known issues

On macOS, seemingly depending on SDK version and if you use TIGR in an Xcode project, you need to define `OBJC_OLD_DISPATCH_PROTOTYPES` to avoid problems with `objc_msgSend` prototypes.
