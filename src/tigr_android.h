#ifndef TIGR_ANDROID_H
#define TIGR_ANDROID_H
#ifdef __ANDROID__

#include <android/input.h>
#include <android/native_window.h>
#include <EGL/egl.h>

typedef enum {
    AE_INPUT,
    AE_WINDOW_CREATED,
    AE_WINDOW_DESTROYED,
    AE_RESUME,
    AE_CLOSE,
} AndroidEventType;

typedef struct {
    AndroidEventType type;
    AInputEvent* inputEvent;
    ANativeWindow* window;
    double time;
} AndroidEvent;

#ifdef __cplusplus
extern "C" {
#endif

/// Calls from TIGR to Android side, render thread
extern int android_pollEvent(int (*eventHandler)(AndroidEvent, void*), void*);
extern void android_swap(EGLDisplay display, EGLSurface surface);
extern void* android_loadAsset(const char* filename, int* outLength);
extern void android_showKeyboard(int show);

/// Calls from Android to TIGR side, main thread
void tigr_android_create();
void tigr_android_destroy();

#ifdef __cplusplus
}
#endif

#endif  // __ANDROID__
#endif  // TIGR_ANDROID_H
