#include "tigr_internal.h"
#include <assert.h>

// not really windows stuff
TigrInternal* tigrInternal(Tigr* bmp) {
    assert(bmp->handle);
    return (TigrInternal*)(bmp + 1);
}

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#pragma comment(lib, "opengl32")  // glViewport
#pragma comment(lib, "shell32")   // CommandLineToArgvW
#pragma comment(lib, "user32")    // SetWindowLong
#pragma comment(lib, "gdi32")     // ChoosePixelFormat
#pragma comment(lib, "advapi32")  // RegSetValueEx

#define WIDGET_SCALE 3
#define WIDGET_FADE 16

int main(int argc, char* argv[]);

#ifndef TIGR_DO_NOT_PRESERVE_WINDOW_POSITION
HKEY tigrRegKey;
#endif

#ifdef __TINYC__
#define CP_UTF8 65001
int WINAPI MultiByteToWideChar();
int WINAPI WideCharToMultiByte();
#endif

static wchar_t* unicode(const char* str) {
    int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, 0, 0);
    wchar_t* dest = (wchar_t*)malloc(sizeof(wchar_t) * len);
    MultiByteToWideChar(CP_UTF8, 0, str, -1, dest, len);
    return dest;
}

void tigrError(Tigr* bmp, const char* message, ...) {
    char tmp[1024];

    va_list args;
    va_start(args, message);
    _vsnprintf(tmp, sizeof(tmp), message, args);
    tmp[sizeof(tmp) - 1] = 0;
    va_end(args);

    MessageBoxW(bmp ? (HWND)bmp->handle : NULL, unicode(tmp), bmp ? tigrInternal(bmp)->wtitle : L"Error",
                MB_OK | MB_ICONERROR);
    exit(1);
}

void tigrEnterBorderlessWindowed(Tigr* bmp) {
    // Enter borderless windowed mode.
    MONITORINFO mi = { sizeof(mi) };
    TigrInternal* win = tigrInternal(bmp);

    GetWindowRect((HWND)bmp->handle, &win->oldPos);

    GetMonitorInfo(MonitorFromWindow((HWND)bmp->handle, MONITOR_DEFAULTTONEAREST), &mi);
    win->dwStyle = WS_VISIBLE | WS_POPUP;
    SetWindowLong((HWND)bmp->handle, GWL_STYLE, win->dwStyle);
    SetWindowPos((HWND)bmp->handle, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                 mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, 0);
}

void tigrLeaveBorderlessWindowed(Tigr* bmp) {
    TigrInternal* win = tigrInternal(bmp);

    win->dwStyle = WS_VISIBLE | WS_OVERLAPPEDWINDOW;
    SetWindowLong((HWND)bmp->handle, GWL_STYLE, win->dwStyle);

    SetWindowPos((HWND)bmp->handle, NULL, win->oldPos.left, win->oldPos.top, win->oldPos.right - win->oldPos.left,
                 win->oldPos.bottom - win->oldPos.top, 0);
}

void tigrWinUpdateWidgets(Tigr* bmp, int dw, int dh) {
    POINT pt;
    int i, x, clicked = 0;
    char str[8];
    TPixel col;
    TPixel off = tigrRGB(255, 255, 255);
    TPixel on = tigrRGB(0, 200, 255);
    TigrInternal* win = tigrInternal(bmp);
    (void)dh;

    tigrClear(win->widgets, tigrRGBA(0, 0, 0, 0));

    if (!(win->dwStyle & WS_POPUP)) {
        win->widgetsWanted = 0;
        win->widgetAlpha = 0;
        return;
    }

    // See if we want to be showing widgets or not.
    GetCursorPos(&pt);
    ScreenToClient((HWND)bmp->handle, &pt);
    if (pt.y == 0)
        win->widgetsWanted = 1;
    if (pt.y > win->widgets->h * WIDGET_SCALE)
        win->widgetsWanted = 0;

    // Track the alpha.
    if (win->widgetsWanted)
        win->widgetAlpha = (win->widgetAlpha <= 255 - WIDGET_FADE) ? win->widgetAlpha + WIDGET_FADE : 255;
    else
        win->widgetAlpha = (win->widgetAlpha >= WIDGET_FADE) ? win->widgetAlpha - WIDGET_FADE : 0;

    // Get relative coords.
    pt.x -= (dw - win->widgets->w * WIDGET_SCALE);
    pt.x /= WIDGET_SCALE;
    pt.y /= WIDGET_SCALE;

    tigrClear(win->widgets, tigrRGBA(0, 0, 0, win->widgetAlpha));

    // Render it.
    for (i = 0; i < 3; i++) {
        switch (i) {
            case 0:
                str[0] = '_';
                str[1] = 0;
                break;  // "_" (minimize)
            case 1:
                str[0] = 0xEF;
                str[1] = 0xBF;
                str[2] = 0xBD;
                str[3] = 0;
                break;  // "[]" (maximize)
            case 2:
                str[0] = 0xC3;
                str[1] = 0x97;
                str[2] = 0;
                break;  // "x" (close)
        }
        x = win->widgets->w + (i - 3) * 12;
        if (i == 2)
            off = tigrRGB(255, 0, 0);
        if (pt.x >= x && pt.x < x + 10 && pt.y < win->widgets->h) {
            col = on;
            if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
                clicked |= 1 << i;
        } else {
            col = off;
        }
        col.a = win->widgetAlpha;
        tigrPrint(win->widgets, tfont, x, 2, col, str);
    }

    if (clicked & 1)
        ShowWindow((HWND)bmp->handle, SW_MINIMIZE);
    if (clicked & 2)
        tigrLeaveBorderlessWindowed(bmp);
    if (clicked & 4)
        SendMessage((HWND)bmp->handle, WM_CLOSE, 0, 0);
}

void tigrUpdate(Tigr* bmp) {
    MSG msg;
    RECT rc;
    int dw, dh;
    TigrInternal* win = tigrInternal(bmp);

    if (!win->shown) {
        win->shown = 1;
        UpdateWindow((HWND)bmp->handle);
        ShowWindow((HWND)bmp->handle, SW_SHOW);
    }

    // Get the window size.
    GetClientRect((HWND)bmp->handle, &rc);
    dw = rc.right - rc.left;
    dh = rc.bottom - rc.top;

    // Update the widget overlay.
    tigrWinUpdateWidgets(bmp, dw, dh);

    if (!tigrGAPIBegin(bmp)) {
        tigrGAPIPresent(bmp, dw, dh);
        SwapBuffers(win->gl.dc);
        tigrGAPIEnd(bmp);
    }

    memcpy(win->prev, win->keys, 256);

    // Run the message pump.
    while (PeekMessage(&msg, (HWND)bmp->handle, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT)
            break;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

typedef BOOL(APIENTRY* PFNWGLSWAPINTERVALFARPROC_)(int);
static PFNWGLSWAPINTERVALFARPROC_ wglSwapIntervalEXT_ = 0;

int tigrGAPIBegin(Tigr* bmp) {
    TigrInternal* win = tigrInternal(bmp);

    return wglMakeCurrent(win->gl.dc, win->gl.hglrc) ? 0 : -1;
}

int tigrGAPIEnd(Tigr* bmp) {
    (void)bmp;
    return wglMakeCurrent(NULL, NULL) ? 0 : -1;
}

static BOOL UnadjustWindowRectEx(LPRECT prc, DWORD dwStyle, BOOL fMenu, DWORD dwExStyle) {
    BOOL fRc;
    RECT rc;
    SetRectEmpty(&rc);
    fRc = AdjustWindowRectEx(&rc, dwStyle, fMenu, dwExStyle);
    if (fRc) {
        prc->left -= rc.left;
        prc->top -= rc.top;
        prc->right -= rc.right;
        prc->bottom -= rc.bottom;
    }
    return fRc;
}

LRESULT CALLBACK tigrWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    Tigr* bmp;
    TigrInternal* win = NULL;
    RECT rc;
    int dw, dh;

    GetClientRect(hWnd, &rc);
    dw = rc.right - rc.left;
    dh = rc.bottom - rc.top;

    bmp = (Tigr*)GetPropW(hWnd, L"Tigr");
    if (bmp)
        win = tigrInternal(bmp);

    switch (message) {
        case WM_PAINT:
            if (!tigrGAPIBegin(bmp)) {
                tigrGAPIPresent(bmp, dw, dh);
                SwapBuffers(win->gl.dc);
                tigrGAPIEnd(bmp);
            }
            ValidateRect(hWnd, NULL);
            break;
        case WM_CLOSE:
            if (win)
                win->closed = 1;
            break;
        case WM_GETMINMAXINFO:
            if (bmp) {
                MINMAXINFO* info = (MINMAXINFO*)lParam;
                RECT rc;
                rc.left = 0;
                rc.top = 0;
                if (win->flags & TIGR_AUTO) {
                    rc.right = 32;
                    rc.bottom = 32;
                } else {
                    int minscale = tigrEnforceScale(1, win->flags);
                    rc.right = bmp->w * minscale;
                    rc.bottom = bmp->h * minscale;
                }
                AdjustWindowRectEx(&rc, win->dwStyle, FALSE, 0);
                info->ptMinTrackSize.x = rc.right - rc.left;
                info->ptMinTrackSize.y = rc.bottom - rc.top;
            }
            return 0;
        case WM_SIZING:
            if (win) {
                // Calculate scale-constrained sizes.
                RECT* rc = (RECT*)lParam;
                int dx, dy;
                UnadjustWindowRectEx(rc, win->dwStyle, FALSE, 0);
                dx = (rc->right - rc->left) % win->scale;
                dy = (rc->bottom - rc->top) % win->scale;
                switch (wParam) {
                    case WMSZ_LEFT:
                        rc->left += dx;
                        break;
                    case WMSZ_RIGHT:
                        rc->right -= dx;
                        break;
                    case WMSZ_TOP:
                        rc->top += dy;
                        break;
                    case WMSZ_TOPLEFT:
                        rc->left += dx;
                        rc->top += dy;
                        break;
                    case WMSZ_TOPRIGHT:
                        rc->right -= dx;
                        rc->top += dy;
                        break;
                    case WMSZ_BOTTOM:
                        rc->bottom -= dy;
                        break;
                    case WMSZ_BOTTOMLEFT:
                        rc->left += dx;
                        rc->bottom -= dy;
                        break;
                    case WMSZ_BOTTOMRIGHT:
                        rc->right -= dx;
                        rc->bottom -= dy;
                        break;
                }
                AdjustWindowRectEx(rc, win->dwStyle, FALSE, 0);
            }
            return TRUE;
        case WM_SIZE:
            if (win) {
                if (wParam != SIZE_MINIMIZED) {
                    // Detect window size changes and update our bitmap accordingly.
                    dw = LOWORD(lParam);
                    dh = HIWORD(lParam);
                    if (win->flags & TIGR_AUTO) {
                        tigrResize(bmp, dw / win->scale, dh / win->scale);
                    } else {
                        win->scale = tigrEnforceScale(tigrCalcScale(bmp->w, bmp->h, dw, dh), win->flags);
                    }
                    tigrPosition(bmp, win->scale, dw, dh, win->pos);
                }

                // If someone tried to maximize us (e.g. via shortcut launch options),
                // prefer instead to be borderless.
                if (wParam == SIZE_MAXIMIZED) {
                    ShowWindow((HWND)bmp->handle, SW_NORMAL);
                    tigrEnterBorderlessWindowed(bmp);
                }
            }
            return 0;
#ifndef TIGR_DO_NOT_PRESERVE_WINDOW_POSITION
        case WM_WINDOWPOSCHANGED: {
            // Save our position.
            WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
            GetWindowPlacement(hWnd, &wp);
            if (win->dwStyle & WS_POPUP)
                wp.showCmd = SW_MAXIMIZE;
            RegSetValueExW(tigrRegKey, win->wtitle, 0, REG_BINARY, (BYTE*)&wp, sizeof(wp));
            return DefWindowProcW(hWnd, message, wParam, lParam);
        }
#endif
        case WM_ACTIVATE:
            if (win) {
                memset(win->keys, 0, 256);
                memset(win->prev, 0, 256);
                win->lastChar = 0;
            }
            return 0;
        case WM_CHAR:
            if (win) {
                if (wParam == '\r') {
                    wParam = '\n';
                }
                int repeating = (HIWORD(lParam) & KF_REPEAT) == KF_REPEAT;
                if (!repeating) {
                    win->lastChar = wParam;
                }
            }
            return DefWindowProcW(hWnd, message, wParam, lParam);
        case WM_MENUCHAR:
            // Disable beep on Alt+Enter
            if (LOWORD(wParam) == VK_RETURN)
                return MNC_CLOSE << 16;
            return DefWindowProcW(hWnd, message, wParam, lParam);
        case WM_SYSKEYDOWN:
            if (win) {
                if (wParam == VK_RETURN) {
                    // Alt+Enter
                    if (win->dwStyle & WS_POPUP)
                        tigrLeaveBorderlessWindowed(bmp);
                    else
                        tigrEnterBorderlessWindowed(bmp);
                    return 0;
                }
            }
            // fall-thru
        case WM_KEYDOWN:
            if (win)
                win->keys[wParam] = 1;
            return DefWindowProcW(hWnd, message, wParam, lParam);
        case WM_SYSKEYUP:
            // fall-thru
        case WM_KEYUP:
            if (win)
                win->keys[wParam] = 0;
            return DefWindowProcW(hWnd, message, wParam, lParam);
        default:
            return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

Tigr* tigrWindow(int w, int h, const char* title, int flags) {
    WNDCLASSEXW wcex = { 0 };
    int maxW, maxH, scale;
    HWND hWnd;
    DWORD dwStyle;
    RECT rc;
    DWORD err;
    Tigr* bmp;
    TigrInternal* win;
#ifndef TIGR_DO_NOT_PRESERVE_WINDOW_POSITION
    WINDOWPLACEMENT wp;
    DWORD wpsize = sizeof(wp);
#endif

    wchar_t* wtitle = unicode(title);

// Find our registry key.
#ifndef TIGR_DO_NOT_PRESERVE_WINDOW_POSITION
    RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\TIGR", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                    &tigrRegKey, NULL);
#endif

    // Register a window class.
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc = tigrWndProc;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.hIcon = NULL;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = L"TIGR";
    RegisterClassExW(&wcex);

    if (flags & TIGR_AUTO) {
        // Always use a 1:1 pixel size.
        scale = 1;
    } else {
        // See how big we can make it and still fit on-screen.
        maxW = GetSystemMetrics(SM_CXSCREEN) * 3 / 4;
        maxH = GetSystemMetrics(SM_CYSCREEN) * 3 / 4;
        scale = tigrCalcScale(w, h, maxW, maxH);
    }

    scale = tigrEnforceScale(scale, flags);

    // Get the final window size.
    dwStyle = WS_OVERLAPPEDWINDOW;
    rc.left = 0;
    rc.top = 0;
    rc.right = w * scale;
    rc.bottom = h * scale;
    AdjustWindowRect(&rc, dwStyle, FALSE);

    // Make a window.
    hWnd = CreateWindowW(L"TIGR", wtitle, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
                         NULL, NULL, wcex.hInstance, NULL);
    err = GetLastError();
    if (!hWnd)
        ExitProcess(1);

    if (flags & TIGR_NOCURSOR) {
        ShowCursor(FALSE);
    }

    // Wrap a bitmap around it.
    bmp = tigrBitmap2(w, h, sizeof(TigrInternal));
    bmp->handle = hWnd;

    // Set up the Windows parts.
    win = tigrInternal(bmp);
    win->dwStyle = dwStyle;
    win->wtitle = wtitle;
    win->shown = 0;
    win->closed = 0;
    win->scale = scale;
    win->lastChar = 0;
    win->flags = flags;

    win->p1 = win->p2 = win->p3 = 0;
    win->p4 = 1;

    win->widgetsWanted = 0;
    win->widgetAlpha = 0;
    win->widgetsScale = WIDGET_SCALE;
    win->widgets = tigrBitmap(40, 14);

    SetPropW(hWnd, L"Tigr", bmp);

    tigrGAPICreate(bmp);

    if (flags & TIGR_FULLSCREEN) {
        tigrEnterBorderlessWindowed(bmp);
    } else {
// Try and restore our window position.
#ifndef TIGR_DO_NOT_PRESERVE_WINDOW_POSITION
        if (RegQueryValueExW(tigrRegKey, wtitle, NULL, NULL, (BYTE*)&wp, &wpsize) == ERROR_SUCCESS) {
            if (wp.showCmd == SW_MAXIMIZE)
                tigrEnterBorderlessWindowed(bmp);
            else
                SetWindowPlacement(hWnd, &wp);
        }
#endif
    }

    wglSwapIntervalEXT_ = (PFNWGLSWAPINTERVALFARPROC_)wglGetProcAddress("wglSwapIntervalEXT");
    if (wglSwapIntervalEXT_)
        wglSwapIntervalEXT_(1);

    return bmp;
}

void tigrFree(Tigr* bmp) {
    if (bmp->handle) {
        TigrInternal* win = tigrInternal(bmp);
        tigrGAPIDestroy(bmp);

        if (win->gl.hglrc && !wglDeleteContext(win->gl.hglrc)) {
            tigrError(bmp, "Cannot delete OpenGL context.\n");
        }
        win->gl.hglrc = NULL;

        if (win->gl.dc && !ReleaseDC((HWND)bmp->handle, win->gl.dc)) {
            tigrError(bmp, "Cannot release OpenGL device context.\n");
        }
        win->gl.dc = NULL;

        DestroyWindow((HWND)bmp->handle);
        free(win->wtitle);
        tigrFree(win->widgets);
    }
    free(bmp->pix);
    free(bmp);
}

int tigrClosed(Tigr* bmp) {
    TigrInternal* win = tigrInternal(bmp);
    int val = win->closed;
    win->closed = 0;
    return val;
}

float tigrTime() {
    static int first = 1;
    static LARGE_INTEGER prev;

    LARGE_INTEGER cnt, freq;
    ULONGLONG diff;
    QueryPerformanceCounter(&cnt);
    QueryPerformanceFrequency(&freq);

    if (first) {
        first = 0;
        prev = cnt;
    }

    diff = cnt.QuadPart - prev.QuadPart;
    prev = cnt;
    return (float)(diff / (double)freq.QuadPart);
}

void tigrMouse(Tigr* bmp, int* x, int* y, int* buttons) {
    POINT pt;
    TigrInternal* win;

    win = tigrInternal(bmp);
    GetCursorPos(&pt);
    ScreenToClient((HWND)bmp->handle, &pt);
    *x = (pt.x - win->pos[0]) / win->scale;
    *y = (pt.y - win->pos[1]) / win->scale;
    *buttons = 0;
    if (GetFocus() != bmp->handle)
        return;
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
        *buttons |= 1;
    if (GetAsyncKeyState(VK_MBUTTON) & 0x8000)
        *buttons |= 2;
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
        *buttons |= 4;
}

int tigrTouch(Tigr* bmp, TigrTouchPoint* points, int maxPoints) {
    int buttons = 0;
    if (maxPoints > 0) {
        tigrMouse(bmp, &points[0].x, &points[1].y, &buttons);
    }
    return buttons ? 1 : 0;
}

static int tigrWinVK(int key) {
    if (key >= 'A' && key <= 'Z')
        return key;
    if (key >= '0' && key <= '9')
        return key;
    switch (key) {
        case TK_BACKSPACE:
            return VK_BACK;
        case TK_TAB:
            return VK_TAB;
        case TK_RETURN:
            return VK_RETURN;
        case TK_SHIFT:
            return VK_SHIFT;
        case TK_CONTROL:
            return VK_CONTROL;
        case TK_ALT:
            return VK_MENU;
        case TK_PAUSE:
            return VK_PAUSE;
        case TK_CAPSLOCK:
            return VK_CAPITAL;
        case TK_ESCAPE:
            return VK_ESCAPE;
        case TK_SPACE:
            return VK_SPACE;
        case TK_PAGEUP:
            return VK_PRIOR;
        case TK_PAGEDN:
            return VK_NEXT;
        case TK_END:
            return VK_END;
        case TK_HOME:
            return VK_HOME;
        case TK_LEFT:
            return VK_LEFT;
        case TK_UP:
            return VK_UP;
        case TK_RIGHT:
            return VK_RIGHT;
        case TK_DOWN:
            return VK_DOWN;
        case TK_INSERT:
            return VK_INSERT;
        case TK_DELETE:
            return VK_DELETE;
        case TK_LWIN:
            return VK_LWIN;
        case TK_RWIN:
            return VK_RWIN;
        // case TK_APPS: return VK_APPS; // this key doesn't exist on OS X
        case TK_PAD0:
            return VK_NUMPAD0;
        case TK_PAD1:
            return VK_NUMPAD1;
        case TK_PAD2:
            return VK_NUMPAD2;
        case TK_PAD3:
            return VK_NUMPAD3;
        case TK_PAD4:
            return VK_NUMPAD4;
        case TK_PAD5:
            return VK_NUMPAD5;
        case TK_PAD6:
            return VK_NUMPAD6;
        case TK_PAD7:
            return VK_NUMPAD7;
        case TK_PAD8:
            return VK_NUMPAD8;
        case TK_PAD9:
            return VK_NUMPAD9;
        case TK_PADMUL:
            return VK_MULTIPLY;
        case TK_PADADD:
            return VK_ADD;
        case TK_PADENTER:
            return VK_SEPARATOR;
        case TK_PADSUB:
            return VK_SUBTRACT;
        case TK_PADDOT:
            return VK_DECIMAL;
        case TK_PADDIV:
            return VK_DIVIDE;
        case TK_F1:
            return VK_F1;
        case TK_F2:
            return VK_F2;
        case TK_F3:
            return VK_F3;
        case TK_F4:
            return VK_F4;
        case TK_F5:
            return VK_F5;
        case TK_F6:
            return VK_F6;
        case TK_F7:
            return VK_F7;
        case TK_F8:
            return VK_F8;
        case TK_F9:
            return VK_F9;
        case TK_F10:
            return VK_F10;
        case TK_F11:
            return VK_F11;
        case TK_F12:
            return VK_F12;
        case TK_NUMLOCK:
            return VK_NUMLOCK;
        case TK_SCROLL:
            return VK_SCROLL;
        case TK_LSHIFT:
            return VK_LSHIFT;
        case TK_RSHIFT:
            return VK_RSHIFT;
        case TK_LCONTROL:
            return VK_LCONTROL;
        case TK_RCONTROL:
            return VK_RCONTROL;
        case TK_LALT:
            return VK_LMENU;
        case TK_RALT:
            return VK_RMENU;
        case TK_SEMICOLON:
            return VK_OEM_1;
        case TK_EQUALS:
            return VK_OEM_PLUS;
        case TK_COMMA:
            return VK_OEM_COMMA;
        case TK_MINUS:
            return VK_OEM_MINUS;
        case TK_DOT:
            return VK_OEM_PERIOD;
        case TK_SLASH:
            return VK_OEM_2;
        case TK_BACKTICK:
            return VK_OEM_3;
        case TK_LSQUARE:
            return VK_OEM_4;
        case TK_BACKSLASH:
            return VK_OEM_5;
        case TK_RSQUARE:
            return VK_OEM_6;
        case TK_TICK:
            return VK_OEM_7;
    }
    return 0;
}

int tigrKeyDown(Tigr* bmp, int key) {
    TigrInternal* win;
    int k = tigrWinVK(key);
    if (GetFocus() != bmp->handle)
        return 0;
    win = tigrInternal(bmp);
    return win->keys[k] && !win->prev[k];
}

int tigrKeyHeld(Tigr* bmp, int key) {
    TigrInternal* win;
    int k = tigrWinVK(key);
    if (GetFocus() != bmp->handle)
        return 0;
    win = tigrInternal(bmp);
    return win->keys[k];
}

int tigrReadChar(Tigr* bmp) {
    TigrInternal* win = tigrInternal(bmp);
    int c = win->lastChar;
    win->lastChar = 0;
    return c;
}

// We supply our own WinMain and just chain through to the user's
// real entry point.
#ifdef UNICODE
int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
#else
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
{
    int n, argc;
    LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    char** argv = (char**)calloc(argc + 1, sizeof(int));

    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    for (n = 0; n < argc; n++) {
        int len = WideCharToMultiByte(CP_UTF8, 0, wargv[n], -1, 0, 0, NULL, NULL);
        argv[n] = (char*)malloc(len);
        WideCharToMultiByte(CP_UTF8, 0, wargv[n], -1, argv[n], len, NULL, NULL);
    }
    return main(argc, argv);
}
#endif
