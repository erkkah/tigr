#include "tigr.h"
#include "math.h"

const char fxShader[] =
    "void fxShader(out vec4 color, in vec2 uv) {"
    "   vec2 tex_size = vec2(textureSize(image, 0));"
    "   vec4 c = texture(image, (floor(uv * tex_size) + 0.5 * sin(parameters.x)) / tex_size);"
    "   color = c;"
    "}";

int main(int argc, char* argv[]) {
    Tigr* screen = tigrWindow(320, 240, "Shady", 0);
    tigrSetPostShader(screen, fxShader, sizeof(fxShader) - 1);

    float duration = 1;
    float phase = 0;
    while (!tigrClosed(screen) && !tigrKeyDown(screen, TK_ESCAPE)) {
        phase += tigrTime();
        while (phase > duration) {
            phase -= duration;
        }
        float p = 6.28 * phase / duration;
        tigrSetPostFX(screen, p, 0, 0, 0);
        tigrClear(screen, tigrRGB(0x80, 0x90, 0xa0));
        tigrPrint(screen, tfont, 120, 110, tigrRGB(0xff, 0xff, 0xff), "Shady business");
        tigrUpdate(screen);
    }
    tigrFree(screen);
    return 0;
}