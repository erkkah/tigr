#include "tigr.h"

int main(int argc, char* argv[]) {
    Tigr* screen = tigrWindow(320, 240, "Clip", 0);

    TPixel c0 = tigrRGB(55, 55, 55);
    TPixel c1 = tigrRGB(255, 255, 255);
    TPixel c2 = tigrRGB(100, 200, 100);
    TPixel c3 = tigrRGBA(100, 100, 200, 150);

    while (!tigrClosed(screen) && !tigrKeyDown(screen, TK_ESCAPE)) {
        tigrClear(screen, c0);

        int cx = screen->w / 2;
        int cy = screen->h / 2;
        int w = 100;
        int d = 50;

        tigrClip(screen, cx - d, cy - d, w, w);
        tigrFill(screen, cx - d, cy - d, w, w, c1);

        tigrRect(screen, cx - w, cy - w, w, w, c2);
        tigrFillRect(screen, cx - w, cy - w, w, w, c3);

        tigrCircle(screen, cx + d, cy - d, d, c2);
        tigrFillCircle(screen, cx + d, cy - d, d, c3);

        const char* message = "Half a thought is also a thought";
        int tw = tigrTextWidth(tfont, message);
        int th = tigrTextHeight(tfont, message);
        tigrPrint(screen, tfont, cx - tw / 2, cy + d - th / 2, c2, message);

        tigrUpdate(screen);
    }
    tigrFree(screen);
    return 0;
}
