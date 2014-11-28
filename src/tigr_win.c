#include "tigr.h"
#include "tigr_internal.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[]);
extern const unsigned char tigrUpscaleVSCode[], tigrUpscalePSCode[];

typedef struct {
	int shown, closed, created;
	IDirect3DDevice9 *dev;
	D3DPRESENT_PARAMETERS params;
	IDirect3DTexture9 *sysTex, *vidTex;
	IDirect3DVertexShader9 *vs;
	IDirect3DPixelShader9 *ps;
	wchar_t *wtitle;
	DWORD dwStyle;

	int flags;
	int scale;
	int pos[4];
	int lastChar;
	char keys[256], prev[256]; 
} TigrWin;

IDirect3D9 *tigrD3D;

static wchar_t *unicode(const char *str)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, 0, 0);
	wchar_t *dest = (wchar_t *)malloc(sizeof(wchar_t) * len);
	MultiByteToWideChar(CP_UTF8, 0, str, -1, dest, len);
	return dest;
}

static TigrWin *tigrWin(Tigr *bmp)
{
	assert(bmp->handle);
	return (TigrWin *)(bmp + 1);
}

void tigrError(Tigr *bmp, const char *message, ...)
{
	char tmp[1024];

    va_list args;
    va_start(args, message);
	_vsnprintf(tmp, sizeof(tmp), message, args);
	tmp[sizeof(tmp)-1] = 0;
    va_end(args);

	MessageBoxW(bmp ? (HWND)bmp->handle : NULL, unicode(tmp), bmp ? tigrWin(bmp)->wtitle : L"Error", MB_OK|MB_ICONERROR);
	exit(1);
}

void tigrDxCreate(Tigr *bmp)
{
	HRESULT hr;
	TigrWin *win = tigrWin(bmp);

	if (!win->created)
	{
		// Check if it's OK to resume work yet.
		if (IDirect3DDevice9_TestCooperativeLevel(win->dev) == D3DERR_DEVICELOST)
			return;

		hr = IDirect3DDevice9_Reset(win->dev, &win->params);
		if (hr == D3DERR_DEVICELOST)
			return;

		if (FAILED(hr)
		 || FAILED(IDirect3DDevice9_CreateTexture(win->dev, bmp->w, bmp->h, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, 
			D3DPOOL_SYSTEMMEM, &win->sysTex, NULL))
		 || FAILED(IDirect3DDevice9_CreateTexture(win->dev, bmp->w, bmp->h, 1, 0, D3DFMT_A8R8G8B8, 
			D3DPOOL_DEFAULT, &win->vidTex, NULL)))
		{
			tigrError(bmp, "Error creating Direct3D 9 textures.\n");
		}

		win->created = 1;
	}
}

void tigrDxDestroy(Tigr *bmp)
{
	TigrWin *win = tigrWin(bmp);
	if (win->created)
	{
		IDirect3DTexture9_Release(win->sysTex);
		IDirect3DTexture9_Release(win->vidTex);
		IDirect3DDevice9_Reset(win->dev, &win->params);
		win->created = 0;
	}
}

void tigrDxPresent(Tigr *bmp)
{
	TigrWin *win;
	D3DLOCKED_RECT rect;
	HWND hWnd;
	HRESULT hr;
	TPixel *src, *dest;
	RECT rc;
	int y, dw, dh;

	win = tigrWin(bmp);
	hWnd = (HWND)bmp->handle;

	// Make sure we have a device.
	// If we don't, then sleep to simulate the vsync until it comes back.
	tigrDxCreate(bmp);
	if (!win->created)
	{
		Sleep(15);
		return;
	}

	IDirect3DDevice9_BeginScene(win->dev);

	// Lock the system memory texture.
	IDirect3DTexture9_LockRect(win->sysTex, 0, &rect, NULL, D3DLOCK_DISCARD);

	// Copy our bitmap into it.
	src = bmp->pix;
	for (y=0;y<bmp->h;y++)
	{
		dest = (TPixel *)( (char *)rect.pBits + rect.Pitch*y );
		memcpy(dest, src, bmp->w*sizeof(TPixel));
		src += bmp->w;
	}

	IDirect3DTexture9_UnlockRect(win->sysTex, 0);

	// Update that into somewhere useful.
	IDirect3DDevice9_UpdateTexture(win->dev, (IDirect3DBaseTexture9 *)win->sysTex, (IDirect3DBaseTexture9 *)win->vidTex);

	// Set up our upscale shader.
	IDirect3DDevice9_SetVertexShader(win->dev, win->vs);
	IDirect3DDevice9_SetPixelShader(win->dev, win->ps);
	IDirect3DDevice9_SetFVF(win->dev, D3DFVF_XYZ|D3DFVF_TEX1);
	IDirect3DDevice9_SetTexture(win->dev, 0, (IDirect3DBaseTexture9 *)win->vidTex);
	IDirect3DDevice9_SetSamplerState(win->dev, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	IDirect3DDevice9_SetSamplerState(win->dev, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	IDirect3DDevice9_SetSamplerState(win->dev, 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	IDirect3DDevice9_SetSamplerState(win->dev, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);

	// Get the window size and let the shader know.
	GetClientRect( hWnd, &rc );
	dw = rc.right - rc.left;
	dh = rc.bottom - rc.top;
	{
		float dsize[4];
		dsize[0] = (float)dw;
		dsize[1] = (float)dh;
		dsize[2] = 0.0f;
		dsize[3] = 0.0f;
		IDirect3DDevice9_SetVertexShaderConstantF(win->dev, 0, dsize, 1);
	}

	// Blit the final image to the screen.
	{
		float x0 = (float)win->pos[0];
		float y0 = (float)win->pos[1];
		float x1 = (float)win->pos[2];
		float y1 = (float)win->pos[3];
		float tri[6][5];
		//          x               y               z              u              v
		tri[0][0] = x0; tri[0][1] = y0; tri[0][2] = 0; tri[0][3] = 0; tri[0][4] = 0;
		tri[1][0] = x1; tri[1][1] = y0; tri[1][2] = 0; tri[1][3] = 1; tri[1][4] = 0;
		tri[2][0] = x0; tri[2][1] = y1; tri[2][2] = 0; tri[2][3] = 0; tri[2][4] = 1;

		tri[3][0] = x1; tri[3][1] = y0; tri[3][2] = 0; tri[3][3] = 1; tri[3][4] = 0;
		tri[4][0] = x1; tri[4][1] = y1; tri[4][2] = 0; tri[4][3] = 1; tri[4][4] = 1;
		tri[5][0] = x0; tri[5][1] = y1; tri[5][2] = 0; tri[5][3] = 0; tri[5][4] = 1;

		// We clear so that a) we fill the border, and b) to let the driver
		// know it can discard the contents.
		IDirect3DDevice9_Clear(win->dev, 0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
		IDirect3DDevice9_DrawPrimitiveUP(win->dev, D3DPT_TRIANGLELIST, 2, &tri, 5*4);
	}

	// Et fini.
	IDirect3DDevice9_EndScene(win->dev);
	hr = IDirect3DDevice9_Present(win->dev, NULL, NULL, hWnd, NULL );

	// See if we lost our device.
	if (hr == D3DERR_DEVICELOST)
		tigrDxDestroy(bmp);
}

void tigrUpdate(Tigr *bmp)
{
	MSG msg;
	TigrWin *win = tigrWin(bmp);

	if (!win->shown)
	{
		win->shown = 1;
		UpdateWindow((HWND)bmp->handle);
		ShowWindow((HWND)bmp->handle, SW_NORMAL);
	}

	tigrDxPresent(bmp);

	memcpy(win->prev, win->keys, 256);

	// Run the message pump.
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			break;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

static BOOL UnadjustWindowRectEx(LPRECT prc, DWORD dwStyle, BOOL fMenu, DWORD dwExStyle)
{
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

LRESULT CALLBACK tigrWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Tigr *bmp;
	TigrWin *win = NULL;
	int dw, dh;

	bmp = (Tigr *)GetPropW(hWnd, L"Tigr");
	if (bmp)
		win = tigrWin(bmp);

	switch (message)
	{
	case WM_PAINT:
		tigrDxPresent(bmp);
		ValidateRect(hWnd, NULL);
		break;
	case WM_CLOSE:
		if (win)
			win->closed = 1;
		break;
	case WM_GETMINMAXINFO:
		if (bmp)
		{
			MINMAXINFO *info = (MINMAXINFO *)lParam;
			RECT rc;
			rc.left = 0;
			rc.top = 0;
			if (win->flags & TIGR_AUTO)
			{
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
		if (win)
		{
			// Calculate scale-constrained sizes.
			RECT *rc = (RECT *)lParam;
			int dx, dy;
			UnadjustWindowRectEx(rc, win->dwStyle, FALSE, 0);
			dx = (rc->right - rc->left) % win->scale;
			dy = (rc->bottom - rc->top) % win->scale;
			switch (wParam) {
			case WMSZ_LEFT: rc->left += dx; break;
			case WMSZ_RIGHT: rc->right -= dx; break;
			case WMSZ_TOP: rc->top += dy; break;
			case WMSZ_TOPLEFT: rc->left += dx; rc->top += dy; break;
			case WMSZ_TOPRIGHT: rc->right -= dx; rc->top += dy; break;
			case WMSZ_BOTTOM: rc->bottom -= dy; break;
			case WMSZ_BOTTOMLEFT: rc->left += dx; rc->bottom -= dy; break;
			case WMSZ_BOTTOMRIGHT: rc->right -= dx; rc->bottom -= dy; break;
			}
			AdjustWindowRectEx(rc, win->dwStyle, FALSE, 0);
		}
		return TRUE;
	case WM_SIZE:
		if (win && (wParam != SIZE_MINIMIZED))
		{
			dw = LOWORD(lParam);
			dh = HIWORD(lParam);
			win->params.BackBufferWidth = dw;
			win->params.BackBufferHeight = dh;
			if (win->flags & TIGR_AUTO)
			{
				tigrResize(bmp, dw/win->scale, dh/win->scale);
			} else {
				win->scale = tigrEnforceScale(tigrCalcScale(bmp->w, bmp->h, dw, dh), win->flags);
			}
			tigrPosition(bmp, win->scale, dw, dh, win->pos);
			tigrDxDestroy(bmp);
			tigrDxCreate(bmp);
		}
		return 0;
	case WM_ACTIVATE:
		if (win) {
			memset(win->keys, 0, 256);
			memset(win->prev, 0, 256);
			win->lastChar = 0;
		}
		return 0;
	case WM_CHAR:
		if (win) {
			if (wParam == '\r') wParam = '\n';
			win->lastChar = wParam;
		}
		return DefWindowProcW(hWnd, message, wParam, lParam);
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		if (win)
			win->keys[wParam] = 1;
		return DefWindowProcW(hWnd, message, wParam, lParam);
	case WM_SYSKEYUP:
	case WM_KEYUP:
		if (win)
			win->keys[wParam] = 0;
		return DefWindowProcW(hWnd, message, wParam, lParam);
	default:
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}
	return 0;
}

void tigrDxSetup(Tigr *bmp)
{
	DWORD flags;
	D3DCAPS9 caps;
	TigrWin *win;

	win = tigrWin(bmp);
	
	// Initialize D3D
	ZeroMemory(&win->params, sizeof(win->params));
	win->params.Windowed				= TRUE;
	win->params.SwapEffect				= D3DSWAPEFFECT_DISCARD;
	win->params.BackBufferFormat		= D3DFMT_A8R8G8B8;
	win->params.EnableAutoDepthStencil	= FALSE;
	win->params.PresentationInterval	= D3DPRESENT_INTERVAL_ONE; // TODO- vsync off if fps suffers?
	win->params.Flags					= 0;
	win->params.BackBufferWidth			= bmp->w;
	win->params.BackBufferHeight		= bmp->h;

	// Check device caps.
	if (FAILED(IDirect3D9_GetDeviceCaps(tigrD3D, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps)))
		tigrError(bmp, "Cannot query Direct3D 9 capabilities.\n");

	// Check vertex processing mode. Ideally I'd just always use software,
	// but some hardware only supports hardware mode (!)
    flags = D3DCREATE_PUREDEVICE | D3DCREATE_FPU_PRESERVE;
    if (caps.VertexProcessingCaps != 0)
        flags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
    else
        flags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	// Create the device.
	if (IDirect3D9_CreateDevice(tigrD3D, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, (HWND)bmp->handle,
		flags, &win->params, &win->dev))
		tigrError(bmp, "Cannot create Direct3D 9 device.\n");

	// Create upscaling shaders.
	if (FAILED(IDirect3DDevice9_CreateVertexShader(win->dev, (DWORD *)tigrUpscaleVSCode, &win->vs))
	 || FAILED(IDirect3DDevice9_CreatePixelShader(win->dev, (DWORD *)tigrUpscalePSCode, &win->ps)))
		tigrError(bmp, "Cannot create Direct3D 9 shaders.\n");
}

Tigr *tigrWindow(int w, int h, const char *title, int flags)
{
	WNDCLASSEXW wcex = {0};
	int maxW, maxH, scale;
	HWND hWnd;
	DWORD dwStyle;
	RECT rc;
	DWORD err;
	Tigr *bmp;
	TigrWin *win;

	wchar_t *wtitle = unicode(title);

	// Register a window class.
	wcex.cbSize			= sizeof(WNDCLASSEXW);
	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc	= tigrWndProc;
	wcex.hInstance		= GetModuleHandle(NULL);
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.lpszClassName	= L"TIGR";
	RegisterClassExW(&wcex);

	if (flags & TIGR_AUTO)
	{
		// Always use a 1:1 pixel size.
		scale = 1;
	} else {
		// See how big we can make it and still fit on-screen.
		maxW = GetSystemMetrics(SM_CXSCREEN) * 3/4;
		maxH = GetSystemMetrics(SM_CYSCREEN) * 3/4;
		scale = tigrCalcScale(w, h, maxW, maxH);
	}

	scale = tigrEnforceScale(scale, flags);

	// Get the final window size.
	dwStyle = WS_OVERLAPPEDWINDOW;
	rc.left = 0; rc.top = 0; rc.right = w*scale; rc.bottom = h*scale;
	AdjustWindowRect(&rc, dwStyle, FALSE);

	// Make a window.
	hWnd = CreateWindowW(L"TIGR", wtitle, dwStyle, 
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right-rc.left, rc.bottom-rc.top,
		NULL, NULL, wcex.hInstance, NULL);
	err = GetLastError();
	if (!hWnd)
		ExitProcess(1);

	// Wrap a bitmap around it.
	bmp = tigrBitmap2(w, h, sizeof(TigrWin));
	bmp->handle = hWnd;

	// Set up the Windows parts.
	win = tigrWin(bmp);
	win->dwStyle = dwStyle;
	win->wtitle = wtitle;
	win->shown = 0;
	win->closed = 0;
	win->created = 0;
	win->scale = scale;
	win->lastChar = 0;
	win->flags = flags;

	SetPropW(hWnd, L"Tigr", bmp);

	if (!tigrD3D)
	{
		tigrD3D = Direct3DCreate9(D3D_SDK_VERSION);
		if (!tigrD3D)
			tigrError(bmp, "Cannot initialize Direct3D 9.");
	}

	tigrDxSetup(bmp);
	tigrDxCreate(bmp);

	return bmp;
}

void tigrFree(Tigr *bmp)
{
	if (bmp->handle)
	{
		TigrWin *win = tigrWin(bmp);
		DestroyWindow((HWND)bmp->handle);
		tigrDxDestroy(bmp);
		IDirect3DVertexShader9_Release(win->vs);
		IDirect3DPixelShader9_Release(win->ps);
		IDirect3DDevice9_Release(win->dev);
		free(win->wtitle);
	}
	free(bmp->pix);
	free(bmp);
}

int tigrClosed(Tigr *bmp)
{
	TigrWin *win = tigrWin(bmp);
	int val = win->closed;
	win->closed = 0;
	return val;
}

float tigrTime()
{
	static int first = 1;
	static LARGE_INTEGER prev;

	LARGE_INTEGER cnt, freq;
	ULONGLONG diff;
	QueryPerformanceCounter(&cnt);
	QueryPerformanceFrequency(&freq);

	if (first)
	{
		first = 0;
		prev = cnt;
	}

	diff = cnt.QuadPart - prev.QuadPart;
	prev = cnt;
	return (float)(diff / (double)freq.QuadPart);
}

void tigrMouse(Tigr *bmp, int *x, int *y, int *buttons)
{
	POINT pt;
	TigrWin *win;

	win = tigrWin(bmp);
	GetCursorPos(&pt);
	ScreenToClient((HWND)bmp->handle, &pt);
	*x = (pt.x - win->pos[0]) / win->scale;
	*y = (pt.y - win->pos[1]) / win->scale;
	*buttons = 0;
	if (GetFocus() != bmp->handle)
		return;
	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) *buttons |= 1;
	if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) *buttons |= 2;
	if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) *buttons |= 4;
}

static int tigrWinVK(int key)
{
	if (key >= 'A' && key <= 'Z') return key;
	if (key >= '0' && key <= '9') return key;
	switch (key) {
	case TK_BACKSPACE: return VK_BACK;
	case TK_TAB: return VK_TAB;
	case TK_RETURN: return VK_RETURN;
	case TK_SHIFT: return VK_SHIFT;
	case TK_CONTROL: return VK_CONTROL;
	case TK_ALT: return VK_MENU;
	case TK_PAUSE: return VK_PAUSE;
	case TK_CAPSLOCK: return VK_CAPITAL;
	case TK_ESCAPE: return VK_ESCAPE;
	case TK_SPACE: return VK_SPACE;
	case TK_PAGEUP: return VK_PRIOR;
	case TK_PAGEDN: return VK_NEXT;
	case TK_END: return VK_END;
	case TK_HOME: return VK_HOME;
	case TK_LEFT: return VK_LEFT;
	case TK_UP: return VK_UP;
	case TK_RIGHT: return VK_RIGHT;
	case TK_DOWN: return VK_DOWN;
	case TK_INSERT: return VK_INSERT;
	case TK_DELETE: return VK_DELETE;
	case TK_LWIN: return VK_LWIN;
	case TK_RWIN: return VK_RWIN;
	case TK_APPS: return VK_APPS;
	case TK_PAD0: return VK_NUMPAD0;
	case TK_PAD1: return VK_NUMPAD1;
	case TK_PAD2: return VK_NUMPAD2;
	case TK_PAD3: return VK_NUMPAD3;
	case TK_PAD4: return VK_NUMPAD4;
	case TK_PAD5: return VK_NUMPAD5;
	case TK_PAD6: return VK_NUMPAD6;
	case TK_PAD7: return VK_NUMPAD7;
	case TK_PAD8: return VK_NUMPAD8;
	case TK_PAD9: return VK_NUMPAD9;
	case TK_PADMUL: return VK_MULTIPLY;
	case TK_PADADD: return VK_ADD;
	case TK_PADENTER: return VK_SEPARATOR;
	case TK_PADSUB: return VK_SUBTRACT;
	case TK_PADDOT: return VK_DECIMAL;
	case TK_PADDIV: return VK_DIVIDE;
	case TK_F1: return VK_F1;
	case TK_F2: return VK_F2;
	case TK_F3: return VK_F3;
	case TK_F4: return VK_F4;
	case TK_F5: return VK_F5;
	case TK_F6: return VK_F6;
	case TK_F7: return VK_F7;
	case TK_F8: return VK_F8;
	case TK_F9: return VK_F9;
	case TK_F10: return VK_F10;
	case TK_F11: return VK_F11;
	case TK_F12: return VK_F12;
	case TK_NUMLOCK: return VK_NUMLOCK;
	case TK_SCROLL: return VK_SCROLL;
	case TK_LSHIFT: return VK_LSHIFT;
	case TK_RSHIFT: return VK_RSHIFT;
	case TK_LCONTROL: return VK_LCONTROL;
	case TK_RCONTROL: return VK_RCONTROL;
	case TK_LALT: return VK_LMENU;
	case TK_RALT: return VK_RMENU;
	case TK_SEMICOLON: return VK_OEM_1;
	case TK_EQUALS: return VK_OEM_PLUS;
	case TK_COMMA: return VK_OEM_COMMA;
	case TK_MINUS: return VK_OEM_MINUS;
	case TK_DOT: return VK_OEM_PERIOD;
	case TK_SLASH: return VK_OEM_2;
	case TK_BACKTICK: return VK_OEM_3;
	case TK_LSQUARE: return VK_OEM_4;
	case TK_BACKSLASH: return VK_OEM_5;
	case TK_RSQUARE: return VK_OEM_6;
	case TK_TICK: return VK_OEM_7;
	}
	return 0;
}

int tigrKeyDown(Tigr *bmp, int key)
{
	TigrWin *win;
	int k = tigrWinVK(key);
	if (GetFocus() != bmp->handle)
		return 0;
	win = tigrWin(bmp);
	return win->keys[k] && !win->prev[k];
}

int tigrKeyHeld(Tigr *bmp, int key)
{
	TigrWin *win;
	int k = tigrWinVK(key);
	if (GetFocus() != bmp->handle)
		return 0;
	win = tigrWin(bmp);
	return win->keys[k];
}

int tigrReadChar(Tigr *bmp)
{
	TigrWin *win = tigrWin(bmp);
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
	LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
	char **argv = (char **)calloc(argc+1, sizeof(int));

	(void)hInstance; (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;

	for (n=0;n<argc;n++)
	{
		int len = WideCharToMultiByte(CP_UTF8, 0, wargv[n], -1, 0, 0, NULL, NULL);
		argv[n] = (char *)malloc(len);
		WideCharToMultiByte(CP_UTF8, 0, wargv[n], -1, argv[n], len, NULL, NULL);
	}
	return main(argc, argv);
}
