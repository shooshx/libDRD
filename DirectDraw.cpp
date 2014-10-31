#include <windows.h>
#include <iostream>
#include <fstream>

#include "drd.h"
#include <ddraw.h>


// define it again here so that we would not have a dependency in ddraw.lib
const GUID MY_IID_IDirectDraw7 = { 0x15e65ec0,0x3b9c,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b };


using namespace std;

extern "C" {

//#pragma comment (lib, "ddraw.lib")
//#pragma comment (lib, "dxguid.lib")

static HMODULE g_ddrawModule = NULL;
typedef HRESULT(WINAPI *TDirectDrawCreateEx)(GUID*, LPVOID*, REFIID, IUnknown*);
TDirectDrawCreateEx pDirectDrawCreateEx;

static HWND g_hMainWnd = NULL;
static IDirectDraw7* g_pDD = NULL;        // DirectDraw object
static IDirectDrawSurface7* g_pDDSFront = NULL;  // DirectDraw fronbuffer surface
static IDirectDrawSurface7* g_pDDSBack = NULL;   // DirectDraw backbuffer surface
static IDirectDrawClipper* g_clipper = NULL;
static bool g_isWindowed = false;
static DWORD g_wndWidth = 0, g_wndHeight = 0, g_srfPixelBytes = 0;
static void(__stdcall *g_keyHandler)(DWORD) = NULL;
static void(__stdcall *g_mouseHandler)(DWORD, DWORD, DWORD) = NULL;
static void(__stdcall *g_errorHandler)(const char* msg) = NULL;
static bool g_switchingWindow = FALSE;


void msgError(const char* msg) {
    if (g_errorHandler != NULL) {
        g_errorHandler(msg);
        return;
    }
    if (MessageBoxA(g_hMainWnd, msg, "Error", MB_RETRYCANCEL | MB_ICONEXCLAMATION) == IDRETRY)
        return;
    ExitProcess(1);
}

void msgError2(const char* msg, const char* msg2) {
    char buf[200] = {0};
    strcpy_s(buf, 200, msg);
    strcat_s(buf, 200, msg2);
    msgError(buf);
}

void msgErrorV(const char* msg, DWORD v) {
    msgError(msg);
}

bool checkHr(HRESULT hr, const char* msg) {
    if (hr != DD_OK) {
        msgError(msg);
        return false;
    }
    return true;
}

void __stdcall drd_setErrorHandler(void(__stdcall *callback)(const char*) ) {
    g_errorHandler = callback;
}



LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        if (!g_switchingWindow) {
            PostQuitMessage(0);
        }
        return 0;
    case WM_KEYDOWN:
        if(wParam == VK_ESCAPE) {
            PostQuitMessage(0);
            return 0;
        }
        if (g_keyHandler != NULL) {
            g_keyHandler(wParam);
        }
        break;
    case WM_LBUTTONUP:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_MOUSEMOVE:
        if (g_mouseHandler != NULL) {
            g_mouseHandler(message, wParam, lParam);
        }
        break;
    } // switch

    LRESULT ret = DefWindowProc(hWnd, message, wParam, lParam);
    return ret;
} 

void __stdcall drd_setKeyHandler(void(__stdcall *callback)(DWORD) ) {
    g_keyHandler = callback;
}
void __stdcall drd_setMouseHandler(void(__stdcall *callback)(DWORD, DWORD, DWORD) ) {
    g_mouseHandler = callback;
}



HWND createWindow(DWORD width, DWORD height, BOOL isWindow)
{
    HWND hWnd;
    WNDCLASSW wc;
    HINSTANCE hInst = GetModuleHandle(NULL);

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = LoadIcon(hInst, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = L"";
    wc.lpszClassName = L"WndClass";
    RegisterClassW(&wc);

    DWORD exstyle = 0, style = 0;
    if (isWindow) {
        exstyle = 0;
        style = WS_SYSMENU;
    }
    else {
        exstyle = WS_EX_TOPMOST;
        style = WS_POPUP;
    }



    hWnd = CreateWindowExA(
        exstyle, //WS_EX_TOPMOST,
        "WndClass", "WndName",
        style, //WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, hInst, NULL);


    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    SetFocus(hWnd);
    return hWnd;
}



// function to initialize DirectDraw in windowed mode
bool initDDrawWindow(HWND hwnd, DWORD width, DWORD height)
{
    g_isWindowed = true;

    HRESULT ddrval;
    // using DDSCL_NORMAL means we will coexist with GDI
    ddrval = g_pDD->SetCooperativeLevel(hwnd, DDSCL_NORMAL);
    if (ddrval != DD_OK) {
        return(false);
    }

    DDSURFACEDESC2 ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof( ddsd );
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    // The primary surface is not a page flipping surface this time
    ddrval = g_pDD->CreateSurface(&ddsd, &g_pDDSFront, NULL);
    if (ddrval != DD_OK)
        return false;

    // Create a clipper to ensure that our drawing stays inside our window
    ddrval = g_pDD->CreateClipper(0, &g_clipper, NULL);
    if (ddrval != DD_OK)
        return false;

    // setting it to our hwnd gives the clipper the coordinates from our window
    ddrval = g_clipper->SetHWnd(0, hwnd);
    if (ddrval != DD_OK)
        return false;

    // attach the clipper to the primary surface
    ddrval = g_pDDSFront->SetClipper(g_clipper);
    if (ddrval != DD_OK)
        return false;

    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = width;
    ddsd.dwHeight = height;

    // create the backbuffer separately
    ddrval = g_pDD->CreateSurface(&ddsd, &g_pDDSBack, NULL);
    if (ddrval != DD_OK)
        return false;

    DDPIXELFORMAT pf;
    pf.dwSize = sizeof(pf);
    ddrval = g_pDDSBack->GetPixelFormat(&pf);
    if (ddrval != DD_OK)
        return false;
    g_srfPixelBytes = pf.dwRGBBitCount / 8;

    return true;
}

bool initDDrawFullScreen(HWND hwnd, DWORD width, DWORD height)
{
    g_isWindowed = false;

    HRESULT ddrval;
    ddrval = g_pDD->SetCooperativeLevel(hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    if (ddrval != DD_OK)
        return false;

    ddrval = g_pDD->SetDisplayMode(width, height, 32 ,0, 0);
    if (ddrval != DD_OK) {
        msgErrorV("FullScreen: Failed SetDisplayMode", ddrval);
        return false;
    }

    // Create the primary surface with 1 back buffer
    DDSURFACEDESC2 ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
    ddsd.dwBackBufferCount = 1;

    ddrval = g_pDD->CreateSurface(&ddsd, &g_pDDSFront, NULL);
    if (ddrval != DD_OK) {
        msgErrorV("FullScreen: Failed CreateSurface", ddrval);
        return false;
    }

    // Get the pointer to the back buffer
    DDSCAPS2 ddscaps;
    ZeroMemory(&ddscaps, sizeof(ddscaps));
    ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
    ddrval = g_pDDSFront->GetAttachedSurface(&ddscaps, &g_pDDSBack); //DDERR_INVALIDOBJECT
    if (ddrval != DD_OK) {
        msgErrorV("FullScreen: GetAttachedSurface ", ddrval);
        return false;
    }

    return true;
}

void cleanUp()
{
    if (g_hMainWnd) 
        DestroyWindow(g_hMainWnd);
    g_hMainWnd = NULL;
    if (g_pDDSBack)
        g_pDDSBack->Release();
    g_pDDSBack = NULL;
    if (g_pDDSFront)
        g_pDDSFront->Release();
    g_pDDSFront = NULL;
    if (g_clipper)
        g_clipper->Release();
    g_clipper = NULL;
    if (g_pDD)
        g_pDD->Release();
    g_pDD = NULL;
}



bool __stdcall drd_init(DWORD width, DWORD height, BOOL isWindow) 
{
    if (g_ddrawModule == NULL) {
        g_ddrawModule = LoadLibraryW(L"DDRAW.DLL");
        if (g_ddrawModule == NULL) {
            msgError("Failing loading ddraw.dll");
            return false;
        }
        pDirectDrawCreateEx = (TDirectDrawCreateEx)GetProcAddress(g_ddrawModule, "DirectDrawCreateEx");
        if (pDirectDrawCreateEx == NULL) {
            msgError("Failing GetProcAdddress DirectDrawCreateEx");
            return false;
        }
    }

    if (g_hMainWnd != NULL) {
        g_switchingWindow = true; // prevent window destruction from quitting
        cleanUp();
        g_switchingWindow = false;
    }
    g_hMainWnd = createWindow(width, height, isWindow);
    if (!g_hMainWnd) {
        msgError("Failed creating window");
        return false;
    }
 
    HRESULT ddrval = pDirectDrawCreateEx(NULL, (VOID**)&g_pDD, MY_IID_IDirectDraw7, NULL);
    if (ddrval != DD_OK) {
        msgError("Could start DirectX engine in your computer.\nMake sure you have at least version 7 of DirectX installed.");
        return false;
    }

    bool initOk = false;
    if (isWindow)
        initOk = initDDrawWindow(g_hMainWnd, width, height);
    else
        initOk = initDDrawFullScreen(g_hMainWnd, width, height);

    if (!initOk) {
        cleanUp();
        msgError("Failed initializing DirectDraw");
        return false;
    }

    g_wndWidth = width;
    g_wndHeight = height;
    return true;
}


void __stdcall drd_flip()
{
    HRESULT ddrval;
    // if we're windowed do the blit, else just Flip
    if (g_isWindowed) {
        // first we need to figure out where on the primary surface our window lives
        POINT p = {0,0};
        ClientToScreen(g_hMainWnd, &p);
        RECT gr = {0};
        GetWindowRect(g_hMainWnd, &gr);
        RECT rcRectDest;
        GetClientRect(g_hMainWnd, &rcRectDest);

        if (rcRectDest.right != g_wndWidth) { // fix window size
            int borderWidth = p.x - gr.left;
            int extraHeight = p.y - gr.top + borderWidth;
            MoveWindow(g_hMainWnd, gr.left, gr.top, g_wndWidth + borderWidth * 2, g_wndHeight + extraHeight, FALSE);
        }

        GetClientRect(g_hMainWnd, &rcRectDest);
        OffsetRect(&rcRectDest, p.x, p.y);
        RECT rcRectSrc;
        SetRect(&rcRectSrc, 0, 0, g_wndWidth, g_wndHeight);
        ddrval = g_pDDSFront->Blt( &rcRectDest, g_pDDSBack, &rcRectSrc, DDBLT_WAIT, NULL);
    } 
    else {
        ddrval = g_pDDSFront->Flip(NULL, DDFLIP_WAIT);
    }
    checkHr(ddrval, "Failed flip");
}



void __stdcall drd_pixelsBegin(CPixelPaint* pp) {
    DDSURFACEDESC2 ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    HRESULT hr = g_pDDSBack->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
    checkHr(hr, "Failed Lock");
    pp->buf = (DWORD*)ddsd.lpSurface;
    pp->pitch = ddsd.lPitch;
    pp->bytesPerPixel = g_srfPixelBytes;
}

// very slow, do not use
DWORD* drd_pixelsPtr(CPixelPaint* pp, DWORD x, DWORD y) {
    return pp->buf + x + y * (pp->pitch / sizeof(DWORD));
}

void __stdcall drd_pixelsEnd() {
    HRESULT hr = g_pDDSBack->Unlock(NULL);
    checkHr(hr, "Failed Unlock");
}

void __stdcall drd_pixelsClear(DWORD color) {
    DDBLTFX fx;
    fx.dwSize = sizeof(fx);
    fx.dwFillColor = color;
    HRESULT ddrval = g_pDDSBack->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &fx);
    checkHr(ddrval, "clear failed Blt");
}

void __stdcall drd_imageDraw(CImg* img, int x, int y) {
    RECT srcRect;
    SetRect(&srcRect, 0, 0, img->width, img->height);
    DWORD flags = DDBLTFAST_WAIT;
    if (img->hasSrcKey)
        flags |= DDBLTFAST_SRCCOLORKEY;
    HRESULT ddrval = g_pDDSBack->BltFast(x, y, img->surface, &srcRect, flags); //DDBLTFAST_NOCOLORKEY
    checkHr(ddrval, "imageDraw failed BltFast");
}


static bool imageLoad(const char* filename, DWORD id, CImg* ret)
{
    BITMAP bm;
    IDirectDrawSurface7 *pdds;

    HBITMAP hbm;
    if (filename != NULL) {
        hbm = (HBITMAP)LoadImageA(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
        if (hbm == NULL) {
            msgError2("imageLoadFile: Failed to load file ", filename);
            return false;
        }
    }
    else {
        hbm = (HBITMAP)LoadImageA(GetModuleHandle(NULL), MAKEINTRESOURCEA(id), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        if (hbm == NULL) {
            msgErrorV("imageLoadFile: Failed  to load resoruce", id);
            return false;
        }
    }

    GetObject(hbm, sizeof(bm), &bm); // get size of bitmap

    // create the surface
    DDSURFACEDESC2 ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT |DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = bm.bmWidth;
    ddsd.dwHeight = bm.bmHeight;

    if (g_pDD->CreateSurface(&ddsd, &pdds, NULL) != DD_OK) {
        msgError("imageLoadFile: failed CreateSurface");
        return false;
    }

    // copy bitmap to surface
    HDC hdcImage = CreateCompatibleDC(NULL);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcImage, hbm);
    HDC hdc;
    if (pdds->GetDC(&hdc) != DD_OK) {
        msgError("imageLoadFile: failed GetDC");
        return false;
    }

    if (BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcImage, 0, 0, SRCCOPY) == 0) {
        msgError("imageLoadFile: failed BitBlt");
        return false;
    }
    pdds->ReleaseDC(hdc);

    SelectObject(hdcImage, hbmOld);
    DeleteDC(hdcImage);
    DeleteObject(hbm);

    ret->surface = pdds;
    ret->height = bm.bmHeight;
    ret->width = bm.bmWidth;
    ret->hasSrcKey = FALSE;
    return true;
}

bool __stdcall drd_imageLoadFile(const char* filename, CImg* ret) {
    return imageLoad(filename, 0, ret);
}

bool __stdcall drd_imageLoadResource(DWORD id, CImg* ret) {
    return imageLoad(NULL, id, ret);
}


void __stdcall drd_imageSetTransparent(CImg* img, DWORD color) {
    DDCOLORKEY ddck;
    ddck.dwColorSpaceLowValue = color;
    ddck.dwColorSpaceHighValue = color;
    HRESULT hr = img->surface->SetColorKey(DDCKEY_SRCBLT, &ddck);
    checkHr(hr, "Failed SetColorKey");
    img->hasSrcKey = TRUE;
}

void __stdcall drd_imageDelete(CImg* img) {
    img->surface->Release();
    img->surface = NULL;
    img->height = 0;
    img->width = 0;
}


void __stdcall drd_printFps() 
{
    static int frameCount = 0;
    static DWORD lastTime = 0;
    

    ++frameCount;
    DWORD time = GetTickCount();
    if (time - lastTime > 1000) 
    {
        ofstream fpsf("c:/temp/drd_fps.txt", ios::out | ios::app | ios::ate);
        if (!fpsf.good()) 
            return;

        fpsf << frameCount << " fps" << endl;
        lastTime = time;
        frameCount = 0;
    }
}

//*****************************************************************************

int sx, sy, w, h, r, g, b;

void drawRect()
{
    CPixelPaint pp;
    drd_pixelsBegin(&pp);

    for(int y = sy; y < sy + h; ++y) {
        for(int x = sx; x < sx + w; ++x) {
            DWORD* pPixelOffset =  pp.buf + x + y * (pp.pitch / sizeof(DWORD));
            //DWORD* pPixelOffset = pixelsPtr(&pp, x, y); //
            *(DWORD*)pPixelOffset = RGB(b, g, r);
        }
    }

    drd_pixelsEnd();
}



static DWORD x = 0, y = 0, dx = 1, dy = 1;
static CImg g_img;

void randomRect() {
    sx = rand() % 640;
    sy = rand() % 480;
    w = rand() % (640 - sx);
    h = rand() % (480 - sy);

    r = rand() % 200 + 55;
    g = rand() % 200 + 55;
    b = rand() % 200 + 55;
}

void moveImg() {
    if (x + g_img.width >= 640)
        dx = -1;
    if (x <= 0)
        dx = 1;
    if (y + g_img.height >= 480)
        dy = -1;
    if (y <= 0)
        dy = 1;
    x += dx;
    y += dy;
}


void processIdle()
{
    randomRect();
    drawRect();
    drd_imageDraw(&g_img, x, y);

    drd_flip();

    if (!g_isWindowed) {
        drawRect();
        //drawImage(&g_img, x, y);
    }

    moveImg();

    drd_printFps();
}

BOOL __stdcall drd_processMessages() {
    MSG msg;
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        // Check for a quit message
        if (msg.message == WM_QUIT) {
            return FALSE;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    } 
    return TRUE;
}

int __stdcall xmain() //int argc, char* argv[])
{
    if (!drd_init(640, 480, TRUE))
        return 0;

    drd_imageLoadFile("C:/projects/Gvahim/DirectDrawTest/test1.bmp", &g_img);
    drd_imageSetTransparent(&g_img, 0xffffff);


    while(true)
    {
        MSG msg;
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            // Check for a quit message
            if( msg.message == WM_QUIT )
                break;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            processIdle();
            //Sleep(1000);
        }
    }

    return 0;
}

// keylogger main
int xxmain(int argc, char* argv[])
{
    while(1) {
        for(int i = 37; i < 128; ++i) {
            int x = GetAsyncKeyState(i);
            if ((x & 1) != 0)
                putchar((char)i);
        }
    }
};





} // extern "C"