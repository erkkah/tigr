#include "tigr.h"

int main(int argc, char* argv[]) {
    Tigr* bmp = tigrBitmap(320, 240);    
    tigrClear(bmp, tigrRGB(0x80, 0x90, 0xa0));
    tigrPrint(bmp, tfont, 120, 110, tigrRGB(0xff, 0xff, 0xff), "Hello, world.");
    tigrSaveImage("headless.png", bmp);
    tigrFree(bmp);
    return 0;
}
