//
// TIGR massage test, runs through most API functions
// and performs basic sanity checks.
//
// Epilepsy warning: the tests will open windows quickly,
// causing multicolored intense flashing!
//

#include "tigr.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <winsock2.h>
#include <GL/gl.h>
#elif defined __linux__
#include <GL/gl.h>
#else
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#endif

void windowWithFlags(int flags) {
    Tigr* win = tigrWindow(100, 100, "CI", flags);
    tigrFill(win, 0, 0, win->w, win->h, tigrRGB(flags >> 1, 64, flags >> 1));
    tigrUpdate(win);
    assert(!tigrClosed(win));
    tigrFree(win);
}

void windowBasics() {
    windowWithFlags(0);
}

void windowFlags() {
    int flagsMax = TIGR_FULLSCREEN * 2 - 1;
    for (int flags = 0; flags < flagsMax; flags++) {
        windowWithFlags(flags);
    }
}

void offscreen() {
    Tigr* bmp = tigrBitmap(100, 100);
    assert(bmp != 0);
    assert(bmp->w == 100);
    assert(bmp->h == 100);
    bmp->pix[100 * 100 - 1] = tigrRGBA(1, 2, 3, 4);
    tigrFree(bmp);
}

static TPixel colors[5] = {
    { 0xff, 0x0, 0x0, 0xff },  { 0xff, 0xff, 0, 0xff }, { 0xff, 0x0, 0xff, 0xff },
    { 0x0, 0xff, 0xff, 0xff }, { 0x0, 0x0, 0x0, 0xff },
};

void drawFauxSierpinski(Tigr* bmp) {
    int scale = 255 / bmp->w + 1;
    for (int x = 0; x < bmp->w; x++) {
        for (int y = 0; y < bmp->h; y++) {
            int c = (((x & y) + (x ^ y)) * scale) & 0xff;
            tigrPlot(bmp, x, y, tigrRGBA(c, c, c, 220));
        }
    }
}

void drawTestPattern(Tigr* bmp) {
    int midW = bmp->w / 2;
    int midH = bmp->h / 2;

    const char* msg = "TIGR test Xy";
    int textHeight = tigrTextHeight(tfont, msg);
    int textWidth = tigrTextWidth(tfont, msg);

    tigrFill(bmp, 0, 0, bmp->w, bmp->h, colors[0]);
    tigrLine(bmp, 0, 0, midW, midH, colors[1]);
    tigrFill(bmp, midW, midH, midW, midH, colors[3]);
    tigrRect(bmp, midW, midH, midW, midH, colors[2]);

    tigrLine(bmp, 0, 10 + textHeight, bmp->w - 1, 10 + textHeight, colors[2]);
    tigrLine(bmp, 10 + textWidth, 0, 10 + textWidth, bmp->h - 1, colors[2]);

    tigrPrint(bmp, tfont, 10, 10, colors[1], "%s v%d", msg, 76);
    tigrPrint(bmp, tfont, 12, 12 + textHeight, colors[2], "%s v%d", msg, 76);
    tigrPrint(bmp, tfont, 14, 14 + 2 * textHeight, colors[3], "%s v%d", msg, 76);

    Tigr* fontImage = tigrLoadImage("5x7.png");
    assert(fontImage != 0);
    TigrFont* font = tigrLoadFont(fontImage, TCP_ASCII);
    assert(font != 0);
    tigrPrint(bmp, font, 10, midH - 10, colors[1], "*** TEENY TINY FONT ***");
    tigrFreeFont(font);

    fontImage = tigrLoadImage("ch.png");
    assert(fontImage != 0);
    font = tigrLoadFont(fontImage, TCP_UTF32);
    assert(font != 0);
    tigrPrint(bmp, font, 10, midH - 40, colors[4], "你好，世界！");
    tigrFreeFont(font);

    Tigr* img = tigrLoadImage("../tigr.png");
    tigrBlit(bmp, img, midW + 1, midH + 1, 42, 125, 70, 42);
    tigrBlitTint(bmp, img, midW + 11, midH + 16, 42, 125, 70, 42, colors[2]);
    tigrBlitAlpha(bmp, img, midW + 21, midH + 31, 42, 125, 70, 42, 0.5);

    Tigr* sierp = tigrBitmap(50, 50);
    drawFauxSierpinski(sierp);

    tigrBlitAlpha(bmp, sierp, 0, midH, 0, 0, sierp->w, sierp->h, 1);
    tigrBlitMode(bmp, TIGR_KEEP_ALPHA);
    tigrBlitAlpha(bmp, sierp, sierp->w, midH + sierp->h, 0, 0, sierp->w, sierp->h, 1);
}

void assertPixelsEqual(TPixel c1, TPixel c2) {
    assert(c1.r == c2.r);
    assert(c1.g == c2.g);
    assert(c1.b == c2.b);
    assert(c1.a == c2.a);
}

void assertBitmapsEqual(Tigr* a, Tigr* b) {
    assert(a->w == b->w);
    assert(a->h == b->h);

    for (int x = 0; x < a->w; x++) {
        for (int y = 0; y < a->h; y++) {
            TPixel c1 = tigrGet(a, x, y);
            TPixel c2 = tigrGet(b, x, y);
            assertPixelsEqual(c1, c2);
        }
    }
}

void verifyLineContract() {
    TPixel bg = tigrRGB(0, 0, 255);
    TPixel fg = tigrRGB(255, 0, 0);

    Tigr* bmp = tigrBitmap(10, 10);

    {
        // Single pixel line

        tigrClear(bmp, bg);
        tigrLine(bmp, 0, 0, 0, 1, fg);

        TPixel firstPixel = tigrGet(bmp, 0, 0);
        assertPixelsEqual(firstPixel, fg);

        TPixel lastPixel = tigrGet(bmp, 0, 1);
        assertPixelsEqual(lastPixel, bg);
    }

    {
        // Diagonal line, first pixel inclusive, last pixel exclusive

        tigrClear(bmp, bg);
        tigrLine(bmp, 0, 0, 9, 9, fg);

        TPixel firstPixel = tigrGet(bmp, 0, 0);
        assertPixelsEqual(firstPixel, fg);

        TPixel lastPixel = tigrGet(bmp, 9, 9);
        assertPixelsEqual(lastPixel, bg);

        TPixel nextToLastPixel = tigrGet(bmp, 8, 8);
        assertPixelsEqual(nextToLastPixel, fg);
    }

    tigrFree(bmp);
}

void verifyRectContract() {
    TPixel bg = tigrRGB(0, 0, 255);
    TPixel fg = tigrRGBA(255, 0, 0, 100);

    Tigr* ref = tigrBitmap(10, 10);
    tigrClear(ref, bg);

    Tigr* bmp = tigrBitmap(10, 10);

    {
        // Zero size rect

        tigrClear(bmp, bg);
        tigrRect(bmp, 0, 0, 0, 0, fg);

        assertBitmapsEqual(bmp, ref);
    }

    {
        // Zero width rect

        tigrClear(bmp, bg);
        tigrRect(bmp, 0, 0, 0, 5, fg);

        assertBitmapsEqual(bmp, ref);
    }

    {
        // Zero height rect

        tigrClear(bmp, bg);
        tigrRect(bmp, 0, 0, 5, 0, fg);

        assertBitmapsEqual(bmp, ref);
    }

    {
        // 2 pixel rect

        tigrClear(ref, bg);
        tigrPlot(ref, 0, 0, fg);
        tigrPlot(ref, 0, 1, fg);
        tigrPlot(ref, 1, 0, fg);
        tigrPlot(ref, 1, 1, fg);

        tigrClear(bmp, bg);
        tigrRect(bmp, 0, 0, 2, 2, fg);

        assertBitmapsEqual(bmp, ref);
    }

    {
        // 2x1 pixel rect

        tigrClear(ref, bg);
        tigrPlot(ref, 0, 0, fg);
        tigrPlot(ref, 1, 0, fg);

        tigrClear(bmp, bg);
        tigrRect(bmp, 0, 0, 2, 1, fg);

        assertBitmapsEqual(bmp, ref);
    }

    {
        // 1x2 pixel rect

        tigrClear(ref, bg);
        tigrPlot(ref, 0, 0, fg);
        tigrPlot(ref, 0, 1, fg);

        tigrClear(bmp, bg);
        tigrRect(bmp, 0, 0, 1, 2, fg);

        assertBitmapsEqual(bmp, ref);
    }

    {
        // 1 pixel rect

        tigrClear(ref, bg);
        tigrPlot(ref, 1, 1, fg);

        tigrClear(bmp, bg);
        tigrRect(bmp, 1, 1, 1, 1, fg);

        assertBitmapsEqual(bmp, ref);
    }

    tigrFree(bmp);
    tigrFree(ref);
}

void verifyDrawing() {
    verifyLineContract();
    verifyRectContract();

    Tigr* bmp = tigrBitmap(200, 200);
    drawTestPattern(bmp);
#ifdef WRITE_REFERENCE
    tigrSaveImage("reference.png", bmp);
#endif
    Tigr* loaded = tigrLoadImage("reference.png");
    assertBitmapsEqual(bmp, loaded);
}

void directOpenGL() {
    Tigr* win = tigrWindow(100, 100, "CI", 0);
    assert(tigrBeginOpenGL(win));

    glClearColor(1, 1, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    tigrUpdate(win);
    tigrFree(win);
}

void customShader() {
    Tigr* win = tigrWindow(100, 100, "CI", 0);

    const char shader[] =
        "void fxShader(out vec4 color, in vec2 uv) {"
        "   vec2 tex_size = vec2(textureSize(image, 0));"
        "   vec4 c = texture(image, (floor(uv * tex_size) + 0.5 * sin(parameters.x)) / tex_size);"
        "   color = c;"
        "}\n";

    tigrSetPostShader(win, shader, sizeof(shader) - 1);
    tigrSetPostFX(win, 3.14 / 2, 0, 0, 0);
    tigrUpdate(win);
    tigrFree(win);
}

void timing() {
    float elapsed = tigrTime();
    assert(elapsed == 0);

    Tigr* win = tigrWindow(100, 100, "CI", 0);
    tigrUpdate(win);

    elapsed = tigrTime();
    assert(elapsed > 0 && elapsed < 1);
}

void input() {
    Tigr* win = tigrWindow(100, 100, "CI", 0);
    tigrUpdate(win);

    assert(tigrKeyHeld(win, TK_CONTROL) == tigrKeyDown(win, TK_CONTROL));
    assert(tigrReadChar(win) == 0);

    int nothing = 100000;
    int x = nothing;
    int y = nothing;
    int buttons = nothing;
    tigrMouse(win, &x, &y, &buttons);
    assert(buttons != nothing);
    assert(x != nothing);
    assert(y != nothing);

    TigrTouchPoint point;
    int touches = tigrTouch(win, &point, 1);
    assert(touches <= 1);
}

void unicode() {
    const int codePoints[] = { 0x00C4, 0x1F308, 'a' };
    const char utf8String[] = "Ä🌈a";

    int decoded = 0;
    const int* codePoint = codePoints;
    const char* utf8Char = utf8String;
    const char* lastChar = utf8Char;
    while (*utf8Char != 0 && (utf8Char = tigrDecodeUTF8(utf8Char, &decoded)) != 0) {
        assert(*codePoint == decoded);

        char buf[32];
        int len = tigrEncodeUTF8(buf, decoded) - buf;
        assert(strncmp(buf, lastChar, len) == 0);

        codePoint++;
        lastChar = utf8Char;
    }
}

typedef struct Test {
    const char* title;
    void (*test)(void);
    int level;
} Test;

int main(int argc, char* argv[]) {
    int limit = 1000;

    if (argc > 1) {
        limit = atoi(argv[1]);
    }

    Test tests[] = { { "Create offscreen", offscreen, 0 },
                     { "Drawing API", verifyDrawing, 0 },
                     { "Window basics", windowBasics, 1 },
                     { "Unicode", unicode, 0 },
                     { "Timing", timing, 1 },
                     { "Custom fx shader", customShader, 2 },
                     { "Direct OpenGL calls", directOpenGL, 2 },
                     { "Input processing", input, 1 },
                     { 0 } };

    for (Test* test = tests; test->title != 0; test++) {
        printf("%s...", test->title);
        if (test->level > limit) {
            printf("skipped\n");
        } else {
            test->test();
            printf("OK\n");
        }
    }

    if (argc == 2 && strcmp(argv[1], "full") == 0) {
        printf("Full window flag test...");
        windowFlags();
        printf("OK\n");
    }

    printf("*** All tests pass OK\n");
    return 0;
}
