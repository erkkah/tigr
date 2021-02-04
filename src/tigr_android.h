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
    AE_ACTIVITY_DESTROYED,
} AndroidEventType;

typedef struct {
    AndroidEventType type;
    AInputEvent* inputEvent;
    ANativeWindow* window;
} AndroidEvent;

#ifdef __cplusplus
extern "C" {
#endif

int android_pollEvent(int (*eventHandler)(AndroidEvent, void*), void*);
void android_swap(EGLDisplay display, EGLSurface surface);
void* android_loadAsset(const char* filename, int* outLength);

#ifdef __cplusplus
}
#endif

#endif  // __ANDROID__
#endif  // TIGR_ANDROID_H
