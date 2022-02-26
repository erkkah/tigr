#include "tigr.h"

Tigr* makeDemoWindow(int w, int h, int flags) {
    return tigrWindow(w, h, "Flag tester", flags);
}

void drawDemoWindow(Tigr* win) {
    TPixel lineColor = tigrRGB(100, 100, 100);
    tigrLine(win, 0, 0, win->w - 1, win->h - 1, lineColor);
    tigrLine(win, 0, win->h - 1, win->w - 1, 0, lineColor);
    tigrRect(win, 0, 0, win->w, win->h, tigrRGB(200, 10, 10));
    tigrPrint(win, tfont, 5, 5, tigrRGB(20, 200, 0), "%dx%d", win->w, win->h);
}

typedef struct Toggle {
    const char* text;
    int checked;
    int value;
    int key;
    TPixel color;
} Toggle;

void drawToggle(Tigr* bmp, Toggle* toggle, int x, int y, int stride) {
    int height = tigrTextHeight(tfont, toggle->text);
    int width = tigrTextWidth(tfont, toggle->text);

    int yOffset = stride / 2;
    int xOffset = width / -2;

    tigrPrint(bmp, tfont, x + xOffset, y + yOffset, toggle->color, toggle->text);

    yOffset += toggle->checked ? height : height / 3;
    TPixel lineColor = toggle->color;
    lineColor.a = 240;
    tigrLine(bmp, x + xOffset, y + yOffset, x + xOffset + width, y + yOffset, lineColor);
}

int main(int argc, char* argv[]) {
    int flags = 0;
    int initialW = 400;
    int initialH = 400;
    Tigr* win = makeDemoWindow(initialW, initialH, flags);
    TPixel white = tigrRGB(255, 255, 255);
    TPixel yellow = tigrRGB(255, 255, 0);
    TPixel black = tigrRGB(0, 0, 0);

    Toggle toggles[] = {
        //
        { "(A)UTO", 0, TIGR_AUTO, 'A', white },
        { "(R)ETINA", 0, TIGR_RETINA, 'R', white },
        { "(F)ULLSCREEN", 0, TIGR_FULLSCREEN, 'F', white },
        { "(2)X", 0, TIGR_2X, '2', yellow },
        { "(3)X", 0, TIGR_3X, '3', yellow },
        { "(4)X", 0, TIGR_4X, '4', yellow },
        { "(N)OCURSOR", 0, TIGR_NOCURSOR, 'N', white },
    };

    while (!tigrClosed(win) && !tigrKeyDown(win, TK_ESCAPE)) {
        tigrClear(win, black);

        drawDemoWindow(win);

        int numToggles = sizeof(toggles) / sizeof(*toggles);
        int stepY = win->h / numToggles;
        int toggleY = 0;
        int toggleX = win->w / 2;

        {
            int newFlags = 0;
            for (int i = 0; i < numToggles; i++, toggleY += stepY) {
                Toggle* toggle = toggles + i;
                if (tigrKeyDown(win, toggle->key)) {
                    toggle->checked ^= 1;
                }
                newFlags += toggle->checked * toggle->value;
                drawToggle(win, toggle, toggleX, toggleY, stepY);
            }

            if (flags != newFlags) {
                int modeFlags = TIGR_AUTO | TIGR_RETINA;
                int modeChange = (flags & modeFlags) != (newFlags & modeFlags);
                flags = newFlags;

                int w = modeChange ? initialW : win->w;
                int h = modeChange ? initialH : win->h;
                tigrFree(win);
                win = makeDemoWindow(w, h, flags);
            }
        }

        tigrUpdate(win);
    }

    tigrFree(win);

    return 0;
}
