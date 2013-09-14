#include <windows.h>
#include <ddraw.h>
#include <iostream>

using namespace std;

HWND g_hMainWnd;
IDirectDraw7* g_pDD = NULL;        // DirectDraw object
IDirectDrawSurface7* g_pDDSFront = NULL;  // DirectDraw fronbuffer surface
IDirectDrawSurface7* g_pDDSBack = NULL;   // DirectDraw backbuffer surface
IDirectDrawClipper* g_clipper;
bool g_isWindowed;

struct Img {
    IDirectDrawSurface7* surface;
    DWORD width;
    DWORD height;
};

Img g_img;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if(wParam == VK_ESCAPE) {
            PostQuitMessage(0);
            return 0;
        }
        break;
    } // switch

    return DefWindowProc(hWnd, message, wParam, lParam);
} 

HWND initWindow()
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

    hWnd = CreateWindowExA(
        0, //WS_EX_TOPMOST,
        "WndClass", "WndName",
        0, //WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
        NULL, NULL, hInst, NULL);

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    SetFocus(hWnd);
    return hWnd;
}

// function to initialize DirectDraw in windowed mode
bool initDDraw(HWND hwnd)
{
    g_isWindowed = true;

    HRESULT ddrval;
    ddrval = DirectDrawCreateEx(NULL, (VOID**)&g_pDD, IID_IDirectDraw7, NULL);
    //ddrval = DirectDrawCreate( NULL, &lpDD, NULL );
    if (ddrval != DD_OK)
        return false;

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
    ddrval = g_pDDSFront->SetClipper( g_clipper );
    if( ddrval != DD_OK )
        return false;

    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = 640;
    ddsd.dwHeight = 480;

    // create the backbuffer separately
    ddrval = g_pDD->CreateSurface(&ddsd, &g_pDDSBack, NULL);
    if (ddrval != DD_OK)
        return false;

    return true;
}

bool flip()
{
    HRESULT ddrval;
    RECT rcRectSrc;
    RECT rcRectDest;
    POINT p;

    // if we're windowed do the blit, else just Flip
    if (g_isWindowed)
    {
        // first we need to figure out where on the primary surface our window lives
        p.x = 0; p.y = 0;
        ClientToScreen(g_hMainWnd, &p);
        GetClientRect(g_hMainWnd, &rcRectDest);
        OffsetRect(&rcRectDest, p.x, p.y);
        SetRect(&rcRectSrc, 0, 0, 640, 480);
        ddrval = g_pDDSFront->Blt( &rcRectDest, g_pDDSBack, &rcRectSrc, DDBLT_WAIT, NULL);
    } 
    else {
        ddrval = g_pDDSFront->Flip(NULL, DDFLIP_WAIT);
    }

    return ddrval == DD_OK;
}




void drawRandomRect()
{
    DDSURFACEDESC2 ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    HRESULT hr = g_pDDSBack->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
    if (hr != DD_OK)
        return;
    // Set pixel 57, 97 to Red (assuming RGB565 pixel fmt)

    //DWORD* pPixelOffset = (DWORD*)ddsd.lpSurface;
    //memset(pPixelOffset, 0xff, 10);
    int sx = rand() % 640;
    int sy = rand() % 480;
    int w = rand() % (640 - sx);
    int h = rand() % (480 - sy);

    int r = rand() % 200 + 55;
    int g = rand() % 200 + 55;
    int b = rand() % 200 + 55;

    for(int y = sy; y < sy + h; ++y) {
        for(int x = sx; x < sx + w; ++x) {
            DWORD * pPixelOffset = (DWORD*)ddsd.lpSurface + x * 1 + y * (ddsd.lPitch / sizeof(DWORD));
            *(DWORD*)pPixelOffset = RGB(b, g, r);
        }
    }
    hr = g_pDDSBack->Unlock(NULL);
}

void drawImage(Img* img, int x, int y) {
    RECT srcRect;
    SetRect(&srcRect, 0, 0, img->width, img->height);
    HRESULT ddrval = g_pDDSBack->BltFast(x, y, img->surface, &srcRect, DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT); //DDBLTFAST_NOCOLORKEY
}

DWORD x = 0, y = 0, dx = 1, dy = 1;


void processIdle()
{
    drawRandomRect();
    drawImage(&g_img, x, y);
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
    flip();
}


void cleanUp()
{
    if (g_pDDSBack)
        g_pDDSBack->Release();
    if (g_pDDSFront)
        g_pDDSFront->Release();
    if (g_pDD)
        g_pDD->Release();
}


bool localImage(const char* filename, Img* ret)
{
    BITMAP bm;
    IDirectDrawSurface7 *pdds;

    HBITMAP hbm = (HBITMAP)LoadImageA(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (hbm == NULL)
        return false;

    GetObject(hbm, sizeof(bm), &bm); // get size of bitmap

    // create the surface
    DDSURFACEDESC2 ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT |DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = bm.bmWidth;
    ddsd.dwHeight = bm.bmHeight;

    if (g_pDD->CreateSurface(&ddsd, &pdds, NULL) != DD_OK)
        return false;

    // copy bitmap to surface
    HDC hdcImage = CreateCompatibleDC(NULL);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcImage, hbm);
    HDC hdc;
    if (pdds->GetDC(&hdc) != DD_OK)
        return false;

    BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcImage, 0, 0, SRCCOPY);
    pdds->ReleaseDC(hdc);

    SelectObject(hdcImage, hbmOld);
    DeleteDC(hdcImage);
    DeleteObject(hbm);

    ret->surface = pdds;
    ret->height = bm.bmHeight;
    ret->width = bm.bmWidth;
    return true;
}



int main(int argc, char* argv[])
{
    g_hMainWnd = initWindow();

    if (!g_hMainWnd)
        return -1;

    if (!initDDraw(g_hMainWnd)) {
        cleanUp();
        MessageBoxA(g_hMainWnd, "Could start DirectX engine in your computer.\nMake sure you have at least version 7 of DirectX installed.", "Error", MB_OK | MB_ICONEXCLAMATION);
        return 0;
    }

    localImage("C:/projects/Gvahim/DirectDrawTest/test1.bmp", &g_img);

    DDCOLORKEY ddck;
    ddck.dwColorSpaceLowValue = 0xffffff;
    ddck.dwColorSpaceHighValue = 0xffffff;
    HRESULT hr = g_img.surface->SetColorKey( DDCKEY_SRCBLT, &ddck );

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

