#include <windows.h>

#include "drd.h"
#include "mcrt.h"
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
static bool g_switchingWindow = false; // don't quit in the closing of the window which is done when reiniting the window
static bool g_minimized = false;


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
    char buf[200];
    mZeroMemory(buf, 200);
    mstrcat_s(buf, 200, msg);
    mstrcat_s(buf, 200, msg2);
    msgError(buf);
}

void msgErrorV(const char* msg, DWORD v) {
    char buf[200];
    mZeroMemory(buf, 200);
    mitoa(v, buf, 16);
    mstrcat_s(buf, 200, " ");
    mstrcat_s(buf, 200, msg);
    msgError(buf);
}

bool checkHr(HRESULT hr, const char* msg) {
    if (hr != DD_OK) {
        msgErrorV(msg, hr);
        return false;
    }
    return true;
}

#define CHECK_INIT(var) if (var == NULL) { msgError2("Not initialized ", #var); return; }
#define CHECKR_INIT(var, ret) if (var == NULL) { msgError2("Not initialized ", #var); return ret; }


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
    case WM_SYSCOMMAND:
        if (wParam == SC_MINIMIZE) {
            g_minimized = true;
        }
        else if (wParam == SC_MAXIMIZE || wParam == SC_RESTORE) {
            g_minimized = false;
        }
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

bool checkFlag(DWORD v, DWORD flag) {
    return ((v & flag) == flag);
}


HWND createWindow(DWORD* width, DWORD* height, DWORD flags)
{
    bool isFullScreen = checkFlag(flags, INIT_FULLSCREEN);
    bool isWindowFull = checkFlag(flags, INIT_WINDOWFULL);
    bool isInputFall = checkFlag(flags, INIT_INPUT_FALLTHROUGH);

    if (isFullScreen) {
        if (isWindowFull) {
            msgError("Can't create a window both full-screen and window-full-screen");
            return NULL;
        }
        if (isInputFall) {
            msgError("Can't create a full screen and with mouce fall-through");
            return NULL;
        }
    }

    g_isWindowed = !isFullScreen;

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

    DWORD exstyle = 0, style = 0, x = CW_USEDEFAULT, y = CW_USEDEFAULT;

    if (isWindowFull) {
        x = 0; y = 0;
        RECT rect;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
        *width = rect.right;
        *height = rect.bottom;
        style = WS_POPUP;
    }
    else if (g_isWindowed) {
        style = WS_SYSMENU | WS_MINIMIZEBOX;
    }
    else {
        exstyle = WS_EX_TOPMOST;
        style = WS_POPUP;
    }

    if (isInputFall) {
        exstyle |= WS_EX_TRANSPARENT | WS_EX_TOPMOST;
    }

    hWnd = CreateWindowExA(
        exstyle, //WS_EX_TOPMOST,
        "WndClass", "WndName",
        style, //WS_POPUP,
        x, y, *width, *height,
        NULL, NULL, hInst, NULL);


    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    SetFocus(hWnd);
    return hWnd;
}

void __stdcall drd_windowSetTranslucent(BYTE alpha) { // (255 * 50) / 100
    if (!g_isWindowed) {
        return;
    }
    if (alpha == 255) {
        SetWindowLong(g_hMainWnd, GWL_EXSTYLE, GetWindowLong(g_hMainWnd, GWL_EXSTYLE) & (~WS_EX_LAYERED));
    }
    else {
        SetWindowLong(g_hMainWnd, GWL_EXSTYLE, GetWindowLong(g_hMainWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    }
    SetLayeredWindowAttributes(g_hMainWnd, 0, alpha, LWA_ALPHA);
}


// function to initialize DirectDraw in windowed mode
bool initDDrawWindow(HWND hwnd, DWORD width, DWORD height)
{
    HRESULT ddrval;
    // using DDSCL_NORMAL means we will coexist with GDI
    ddrval = g_pDD->SetCooperativeLevel(hwnd, DDSCL_NORMAL);
    if (ddrval != DD_OK) {
        return(false);
    }

    DDSURFACEDESC2 ddsd;
    mZeroMemory(&ddsd, sizeof(ddsd));
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

    mZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = width;
    ddsd.dwHeight = height;

    // create the backbuffer separately
    ddrval = g_pDD->CreateSurface(&ddsd, &g_pDDSBack, NULL);
    if (ddrval != DD_OK)
        return false;

    return true;
}

bool initDDrawFullScreen(HWND hwnd, DWORD width, DWORD height)
{
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
    mZeroMemory(&ddsd, sizeof(ddsd));
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
    mZeroMemory(&ddscaps, sizeof(ddscaps));
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



bool __stdcall drd_init(DWORD width, DWORD height, DWORD flags) 
{
    if (g_ddrawModule == NULL) {
        g_ddrawModule = LoadLibraryW(L"DDRAW.DLL");
        if (g_ddrawModule == NULL) {
            msgErrorV("Failing loading ddraw.dll", GetLastError());
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
    g_hMainWnd = createWindow(&width, &height, flags); // sets g_isWindowed
    if (!g_hMainWnd) {
        msgError("Failed creating window");
        return false;
    }
/*
    HDC dc = GetDC(g_hMainWnd);
    SetDCBrushColor(dc, 0x0000ff);
    RECT r = {0,0,100,100};
    FillRect(dc, &r, (HBRUSH)GetStockObject(DC_BRUSH));
*/
 
    HRESULT ddrval = pDirectDrawCreateEx(NULL, (VOID**)&g_pDD, MY_IID_IDirectDraw7, NULL);
    if (ddrval != DD_OK) {
        msgError("Could start DirectX engine in your computer.\nMake sure you have at least version 7 of DirectX installed.");
        return false;
    }

    bool initOk = false;
    if (g_isWindowed)
        initOk = initDDrawWindow(g_hMainWnd, width, height);
    else
        initOk = initDDrawFullScreen(g_hMainWnd, width, height);

    if (!initOk) {
        msgError("Failed initializing DirectDraw");
        cleanUp();
        return false;
    }

    DDPIXELFORMAT pf;
    pf.dwSize = sizeof(pf);
    ddrval = g_pDDSBack->GetPixelFormat(&pf);
    if (ddrval != DD_OK)
        return false;
    g_srfPixelBytes = pf.dwRGBBitCount / 8;


    g_wndWidth = width;
    g_wndHeight = height;
    return true;
}


void __stdcall drd_flip()
{
    CHECK_INIT(g_pDDSBack);
    if (g_minimized)
        return; // it's going to fail for a minimized window.

    // if we're windowed do the blit, else just Flip
    if (g_isWindowed) {
        // first we need to figure out where on the primary surface our window lives
        POINT p = {0,0};
        ClientToScreen(g_hMainWnd, &p);
        RECT gr;
        mZeroMemory(&gr, sizeof(gr));
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

       /* DDBLTFX fx; no need for this
        mZeroMemory(&fx, sizeof(fx));
        fx.dwSize = sizeof(fx);
        fx.dwDDFX = DDBLTFX_NOTEARING;*/

        // BltFast can't clip, so using Blt

        DWORD flags = DDBLT_WAIT;
        
     /*   flags != DDBLT_KEYSRC;
        DDCOLORKEY ddck;
        ddck.dwColorSpaceLowValue = 0x00ff00;
        ddck.dwColorSpaceHighValue = 0x00ff00;
        HRESULT hr = g_pDDSBack->SetColorKey(DDCKEY_SRCBLT, &ddck);
        hr = g_pDDSFront->SetColorKey(DDCKEY_SRCBLT, &ddck);
       */ 
        HRESULT ddrval = g_pDDSFront->Blt( &rcRectDest, g_pDDSBack, &rcRectSrc, flags, NULL); // | DDBLT_DDFX, &fx);
        checkHr(ddrval, "Failed win flip"); 
    } 
    else {
        HRESULT ddrval = g_pDDSFront->Flip(NULL, DDFLIP_WAIT);
        checkHr(ddrval, "Failed flip");
    }
}


void __stdcall drd_pixelsBegin(CPixelPaint* pp) {
    CHECK_INIT(pp);
    CHECK_INIT(g_pDDSBack);
    DDSURFACEDESC2 ddsd;
    mZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    HRESULT hr = g_pDDSBack->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
    checkHr(hr, "Failed Lock");
    pp->buf = (DWORD*)ddsd.lpSurface;
    pp->pitch = ddsd.lPitch;
    pp->bytesPerPixel = g_srfPixelBytes;
    pp->cheight = g_wndHeight;
    pp->cwidth = g_wndWidth;
}


// very slow, do not use
DWORD* drd_pixelsPtr(CPixelPaint* pp, DWORD x, DWORD y) {
    return pp->buf + x + y * (pp->pitch / sizeof(DWORD));
}

void __stdcall drd_pixelsEnd() {
    CHECK_INIT(g_pDDSBack);
    HRESULT hr = g_pDDSBack->Unlock(NULL);
    checkHr(hr, "Failed Unlock");
}



void __stdcall drd_pixelsClear(DWORD color) {
    CHECK_INIT(g_pDDSBack);
    DDBLTFX fx;
    fx.dwSize = sizeof(fx);
    fx.dwFillColor = color;
    HRESULT ddrval = g_pDDSBack->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &fx);
    checkHr(ddrval, "clear failed Blt");
}

void __stdcall drd_imageDrawCrop(CImg* img, int dstX, int dstY, int srcX, int srcY, int srcWidth, int srcHeight) {
    CHECK_INIT(img);
    RECT srcRect;
    SetRect(&srcRect, srcX, srcY, srcX+srcWidth, srcY+srcHeight);
    DWORD flags = DDBLTFAST_WAIT;
    if (img->hasSrcKey)
        flags |= DDBLTFAST_SRCCOLORKEY;
    HRESULT ddrval = g_pDDSBack->BltFast(dstX, dstY, img->surface, &srcRect, flags); //DDBLTFAST_NOCOLORKEY
    checkHr(ddrval, "imageDraw failed BltFast");
}

void __stdcall drd_imageDraw(CImg* img, int dstX, int dstY) {
    CHECK_INIT(img);
    drd_imageDrawCrop(img, dstX, dstY, 0, 0, img->width, img->height);
}




static bool imageLoad(const char* filename, DWORD id, CImg* ret)
{
    CHECKR_INIT(ret, false);

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
    mZeroMemory(&ddsd, sizeof(ddsd));
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
    CHECKR_INIT(filename, false);   
    return imageLoad(filename, 0, ret);
}

bool __stdcall drd_imageLoadResource(DWORD id, CImg* ret) {
    return imageLoad(NULL, id, ret);
}


void __stdcall drd_imageSetTransparent(CImg* img, DWORD color) {
    CHECK_INIT(img);
    CHECK_INIT(img->surface);

    DDCOLORKEY ddck;
    ddck.dwColorSpaceLowValue = color;
    ddck.dwColorSpaceHighValue = color;
    HRESULT hr = img->surface->SetColorKey(DDCKEY_SRCBLT, &ddck);
    checkHr(hr, "Failed SetColorKey");
    img->hasSrcKey = TRUE;
}

void __stdcall drd_imageDelete(CImg* img) {
    CHECK_INIT(img);

    img->surface->Release();
    img->surface = NULL;
    img->height = 0;
    img->width = 0;
}


void __stdcall drd_printFps(const char* filename) 
{
    static int frameCount = 0;
    static DWORD lastTime = 0;
    
    if (filename == NULL)
        filename = "c:/temp/drd_fps.txt";

    ++frameCount;
    DWORD time = GetTickCount();
    if (time - lastTime > 1000) 
    {
        char buf[100];
        mZeroMemory(buf, 100);
        mitoa(frameCount, buf, 10);
        lastTime = time;
        frameCount = 0;

        mstrcat_s(buf, 100, " fps");

        // http://msdn.microsoft.com/en-us/library/windows/desktop/aa363778(v=vs.85).aspx
        HANDLE hf = CreateFileA(filename, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (hf == INVALID_HANDLE_VALUE)
            return;
        SetFilePointer(hf, 0, NULL, FILE_END);
        DWORD written = 0;
        WriteFile(hf, buf, mstrlen_s(buf, 100), &written, NULL);
        CloseHandle(hf);
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
    sx = mrand() % 640;
    sy = mrand() % 480;
    w = mrand() % (640 - sx);
    h = mrand() % (480 - sy);

    r = mrand() % 200 + 55;
    g = mrand() % 200 + 55;
    b = mrand() % 200 + 55;
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

    if (!g_isWindowed) { // in full screen need to draw the same rect on both buffers
        drawRect();
        //drawImage(&g_img, x, y);
    }

    moveImg();

    drd_printFps(NULL);
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
    if (!drd_init(640, 480, INIT_WINDOW))
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
/*
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
*/




} // extern "C"