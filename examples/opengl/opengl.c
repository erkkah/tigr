#include "tigr.h"
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

int main(int argc, char* argv[]) {
    Tigr* win1 = tigrWindow(320, 240, "Window1", 0);
    Tigr* win2 = tigrWindow(320, 240, "Window2", 0);
    while (win1 || win2) {
        if (win1) {
            tigrClear(win1, tigrRGB(0x80, 0x90, 0xa0));
            tigrPrint(win1, tfont, 120, 110, tigrRGB(0xff, 0xff, 0xff), "Hello, world #1.");
            tigrUpdate(win1);

            if (tigrClosed(win1) || tigrKeyDown(win1, TK_ESCAPE)) {
                tigrFree(win1);
                win1 = NULL;
            }
        }

        if (win2) {
            if (tigrBeginOpenGL(win2)) {
                glClearColor(1, 0, 1, 1);
                glClear(GL_COLOR_BUFFER_BIT);
            }

            tigrClear(win2, tigrRGBA(0x00, 0x00, 0x00, 0x00));
            tigrPrint(win2, tfont, 120, 110, tigrRGB(0xff, 0xff, 0xff), "Hello, world #2.");
            tigrUpdate(win2);

            if (tigrClosed(win2) || tigrKeyDown(win2, TK_ESCAPE)) {
                tigrFree(win2);
                win2 = NULL;
            }
        }
    }
    if (win1)
        tigrFree(win1);
    if (win2)
        tigrFree(win2);
    return 0;
}
