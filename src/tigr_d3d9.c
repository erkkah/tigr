#include "tigr_internal.h"

#ifdef TIGR_GAPI_D3D9
IDirect3D9 *tigrD3D = NULL;
extern const unsigned char tigrUpscaleVSCode[], tigrUpscalePSCode[];

void tigrGAPICreate(Tigr *bmp)
{
	TigrInternal *win = tigrInternal(bmp);
	D3D9Stuff *d3d = &win->d3d9;
	DWORD flags;
	D3DCAPS9 caps;

	if (!tigrD3D)
	{
		tigrD3D = Direct3DCreate9(D3D_SDK_VERSION);
		if (!tigrD3D)
			tigrError(bmp, "Cannot initialize Direct3D 9.");
	}

	// Initialize D3D
	ZeroMemory(&d3d->params, sizeof(d3d->params));
	d3d->params.Windowed				= TRUE;
	d3d->params.SwapEffect				= D3DSWAPEFFECT_DISCARD;
	d3d->params.BackBufferFormat		= D3DFMT_A8R8G8B8;
	d3d->params.EnableAutoDepthStencil	= FALSE;
	d3d->params.PresentationInterval	= D3DPRESENT_INTERVAL_ONE; // TODO- vsync off if fps suffers?
	d3d->params.Flags					= 0;
	d3d->params.BackBufferWidth			= bmp->w;
	d3d->params.BackBufferHeight		= bmp->h;

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
		flags, &d3d->params, &d3d->dev))
		tigrError(bmp, "Cannot create Direct3D 9 device.\n");

	// Create upscaling shaders.
	if (FAILED(IDirect3DDevice9_CreateVertexShader(d3d->dev, (DWORD *)tigrUpscaleVSCode, &d3d->vs))
	 || FAILED(IDirect3DDevice9_CreatePixelShader(d3d->dev, (DWORD *)tigrUpscalePSCode, &d3d->ps)))
		tigrError(bmp, "Cannot create Direct3D 9 shaders.\n");
}

void tigrGAPIBegin(Tigr *bmp)
{
	HRESULT hr;
	TigrInternal *win = tigrInternal(bmp);
	D3D9Stuff *d3d = &win->d3d9;

	if (d3d->lost)
	{
		// Check if it's OK to resume work yet.
		if (IDirect3DDevice9_TestCooperativeLevel(d3d->dev) == D3DERR_DEVICELOST)
			return;

		hr = IDirect3DDevice9_Reset(d3d->dev, &d3d->params);
		if (hr == D3DERR_DEVICELOST)
			return;

		if (FAILED(hr)
		 || FAILED(IDirect3DDevice9_CreateTexture(d3d->dev, bmp->w, bmp->h, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8,
			D3DPOOL_SYSTEMMEM, &d3d->sysTex[0], NULL))
		 || FAILED(IDirect3DDevice9_CreateTexture(d3d->dev, bmp->w, bmp->h, 1, 0, D3DFMT_A8R8G8B8,
			D3DPOOL_DEFAULT, &d3d->vidTex[0], NULL))
		 || FAILED(IDirect3DDevice9_CreateTexture(d3d->dev, win->widgets->w, win->widgets->h, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8,
			D3DPOOL_SYSTEMMEM, &d3d->sysTex[1], NULL))
		 || FAILED(IDirect3DDevice9_CreateTexture(d3d->dev, win->widgets->w, win->widgets->h, 1, 0, D3DFMT_A8R8G8B8,
			D3DPOOL_DEFAULT, &d3d->vidTex[1], NULL)))
		{
			tigrError(bmp, "Error creating Direct3D 9 textures.\n");
		}

		d3d->lost = 0;
	}
}

void tigrGAPIEnd(Tigr *bmp)
{
	TigrInternal *win = tigrInternal(bmp);
	D3D9Stuff *d3d = &win->d3d9;
	if (!d3d->lost)
	{
		IDirect3DTexture9_Release(d3d->sysTex[0]);
		IDirect3DTexture9_Release(d3d->vidTex[0]);
		IDirect3DTexture9_Release(d3d->sysTex[1]);
		IDirect3DTexture9_Release(d3d->vidTex[1]);
		IDirect3DDevice9_Reset(d3d->dev, &d3d->params);
		d3d->lost = 1;
	}
}

void tigrGAPIDestroy(Tigr *bmp)
{
	TigrInternal *win = tigrInternal(bmp);
	D3D9Stuff *d3d = &win->d3d9;

	IDirect3DVertexShader9_Release(d3d->vs);
	IDirect3DPixelShader9_Release(d3d->ps);
	IDirect3DDevice9_Release(d3d->dev);
}

void tigrGAPIResize(Tigr *bmp, int width, int height)
{
	TigrInternal *win = tigrInternal(bmp);
	D3D9Stuff *d3d = &win->d3d9;

	tigrGAPIEnd(bmp);
	d3d->params.BackBufferWidth = width;
	d3d->params.BackBufferHeight = height;
	tigrGAPIBegin(bmp);
}

void tigrDxUpdate(IDirect3DDevice9 *dev, IDirect3DTexture9 *sysTex, IDirect3DTexture9 *vidTex, Tigr *bmp)
{
	D3DLOCKED_RECT rect;
	TPixel *src, *dest;
	int y;

	// Lock the system memory texture.
	IDirect3DTexture9_LockRect(sysTex, 0, &rect, NULL, D3DLOCK_DISCARD);

	// Copy our bitmap into it.
	src = bmp->pix;
	for (y=0;y<bmp->h;y++)
	{
		dest = (TPixel *)( (char *)rect.pBits + rect.Pitch*y );
		memcpy(dest, src, bmp->w*sizeof(TPixel));
		src += bmp->w;
	}

	IDirect3DTexture9_UnlockRect(sysTex, 0);

	// Update that into somewhere useful.
	IDirect3DDevice9_UpdateTexture(dev, (IDirect3DBaseTexture9 *)sysTex, (IDirect3DBaseTexture9 *)vidTex);
}

void tigrDxQuad(IDirect3DDevice9 *dev, IDirect3DTexture9 *tex, int ix0, int iy0, int ix1, int iy1)
{
	float tri[6][5];
	float x0 = (float)ix0;
	float y0 = (float)iy0;
	float x1 = (float)ix1;
	float y1 = (float)iy1;

	//          x               y               z              u              v
	tri[0][0] = x0; tri[0][1] = y0; tri[0][2] = 0; tri[0][3] = 0; tri[0][4] = 0;
	tri[1][0] = x1; tri[1][1] = y0; tri[1][2] = 0; tri[1][3] = 1; tri[1][4] = 0;
	tri[2][0] = x0; tri[2][1] = y1; tri[2][2] = 0; tri[2][3] = 0; tri[2][4] = 1;

	tri[3][0] = x1; tri[3][1] = y0; tri[3][2] = 0; tri[3][3] = 1; tri[3][4] = 0;
	tri[4][0] = x1; tri[4][1] = y1; tri[4][2] = 0; tri[4][3] = 1; tri[4][4] = 1;
	tri[5][0] = x0; tri[5][1] = y1; tri[5][2] = 0; tri[5][3] = 0; tri[5][4] = 1;

	IDirect3DDevice9_SetTexture(dev, 0, (IDirect3DBaseTexture9 *)tex);
	IDirect3DDevice9_DrawPrimitiveUP(dev, D3DPT_TRIANGLELIST, 2, &tri, 5*4);
}

void tigrGAPIPresent(Tigr *bmp, int dw, int dh)
{
	TigrInternal *win;
	D3D9Stuff *d3d;
	HWND hWnd;
	HRESULT hr;

	win = tigrInternal(bmp);
	d3d = &win->d3d9;
	hWnd = (HWND)bmp->handle;

	// Make sure we have a device.
	// If we don't, then sleep to simulate the vsync until it comes back.
	if (d3d->lost)
	{
		tigrGAPIBegin(bmp);
		if (d3d->lost) {
			Sleep(15);
			return;
		}
	}

	IDirect3DDevice9_BeginScene(d3d->dev);

	// Upload the pixel data.
	tigrDxUpdate(d3d->dev, d3d->sysTex[0], d3d->vidTex[0], bmp);
	tigrDxUpdate(d3d->dev, d3d->sysTex[1], d3d->vidTex[1], win->widgets);

	// Set up our upscale shader.
	IDirect3DDevice9_SetRenderState(d3d->dev, D3DRS_ALPHABLENDENABLE, FALSE);
	IDirect3DDevice9_SetVertexShader(d3d->dev, d3d->vs);
	IDirect3DDevice9_SetPixelShader(d3d->dev, d3d->ps);
	IDirect3DDevice9_SetFVF(d3d->dev, D3DFVF_XYZ|D3DFVF_TEX1);
	IDirect3DDevice9_SetSamplerState(d3d->dev, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
	IDirect3DDevice9_SetSamplerState(d3d->dev, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
	IDirect3DDevice9_SetSamplerState(d3d->dev, 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	IDirect3DDevice9_SetSamplerState(d3d->dev, 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

	// Let the shader know about the window size.
	{
		float consts[12];
		consts[0] = (float)dw;
		consts[1] = (float)dh;
		consts[2] = 1.0f / dw;
		consts[3] = 1.0f / dh;
		consts[4] = (float)bmp->w;
		consts[5] = (float)bmp->h;
		consts[6] = 1.0f / bmp->w;
		consts[7] = 1.0f / bmp->h;
		consts[8] = win->hblur ? 1.0f : 0.0f;
		consts[9] = win->vblur ? 1.0f : 0.0f;
		consts[10] = win->scanlines;
		consts[11] = win->contrast;
		IDirect3DDevice9_SetVertexShaderConstantF(d3d->dev, 0, consts, 3);
		IDirect3DDevice9_SetPixelShaderConstantF(d3d->dev, 0, consts, 3);
	}

	// We clear so that a) we fill the border, and b) to let the driver
	// know it can discard the contents.
	IDirect3DDevice9_Clear(d3d->dev, 0, NULL, D3DCLEAR_TARGET, 0, 0, 0);

	// Blit the final image to the screen.
	tigrDxQuad(d3d->dev, d3d->vidTex[0], win->pos[0], win->pos[1], win->pos[2], win->pos[3]);

	// Draw the widget overlay.
	IDirect3DDevice9_SetRenderState(d3d->dev, D3DRS_ALPHABLENDENABLE, TRUE);
	IDirect3DDevice9_SetRenderState(d3d->dev, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	IDirect3DDevice9_SetRenderState(d3d->dev, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	tigrDxQuad(d3d->dev, d3d->vidTex[1], 
		(int)(dw - win->widgets->w * win->widgetsScale), 0, 
		dw, (int)(win->widgets->h * win->widgetsScale));

	// Et fini.
	IDirect3DDevice9_EndScene(d3d->dev);
	hr = IDirect3DDevice9_Present(d3d->dev, NULL, NULL, hWnd, NULL );

	// See if we lost our device.
	if (hr == D3DERR_DEVICELOST)
		tigrGAPIEnd(bmp);
}

#endif
