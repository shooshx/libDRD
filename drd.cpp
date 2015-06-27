#include <windows.h>
#include <CommCtrl.h>

#include "drd.h"
#include "drd_mcrt.h"
#include "drd_arraymap.h"
#include <ddraw.h>
#include <gl/gl.h>

#define VERSION L"1.4"

// define it again here so that we would not have a dependency in ddraw.lib
const GUID MY_IID_IDirectDraw7 = { 0x15e65ec0,0x3b9c,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b };

#define MYSC_ABOUT 1001

using namespace std;

//---------- Error handling functions -------------------

extern "C"static void(__stdcall *g_errorHandler)(const char* msg) = NULL;

void format(char* buf, int len) {}

template<typename... Args>
void format(char* buf, int len, const char* first, Args... args) {
    mstrcat_s(buf, len, first);
    format(buf, len, args...);
}
template<typename... Args>
void format(char* buf, int len, DWORD first, Args... args) {
    mitoa(first, buf, 16);
    format(buf, len, args...);
}

template<typename... Args>
void msgError(Args... args) {
    char buf[300];
    mZeroMemory(buf, 200);
    format(buf, 200, args...);

    if (g_errorHandler != NULL) {
        g_errorHandler(buf);
        return;
    }
    if (MessageBoxA(g_hMainWnd, buf, "Error", MB_RETRYCANCEL | MB_ICONEXCLAMATION) == IDRETRY)
        return;
    ExitProcess(1);
}

const char* lastErrorStr() {
    DWORD v = GetLastError();
    static char buf[200];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, v, 0, buf, 200, NULL);
    return buf;
}

void msgErrorLE(const char* msg) {
    msgError(msg, lastErrorStr());
}

bool checkHr(HRESULT hr, const char* msg) {
    if (hr != DD_OK) {
        msgError(msg, hr);
        return false;
    }
    return true;
}

#define CHECK_INIT(var) if (var == NULL) { msgError("Not initialized ", #var); return; }
#define CHECK_INIT_MSG(var, msg) if (var == NULL) { msgError(msg "\nnot initialized ", #var); return; }
#define CHECKR_INIT(var, ret) if (var == NULL) { msgError("Not initialized ", #var); return ret; }
#define CHECKR_INIT_MSG(var, ret, msg) if (var == NULL) { msgError(msg "\nnot initialized ", #var); return ret; }
#define CHECK_INITDRD(var) if (var == NULL) { msgError("DRD not initialized. call drd_init first"); return; }
#define CHECKR_INITDRD(var, ret) if (var == NULL) { msgError("DRD not initialized. call drd_init first"); return ret; }



extern "C" {

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
static DWORD g_surfaceWidth = 0, g_surfaceHeight = 0; // window can be resized separately from the surface
static void(__stdcall *g_keyHandler)(DWORD) = NULL;
static void(__stdcall *g_mouseHandler)(DWORD, DWORD, DWORD) = NULL;
static void(__stdcall *g_resizeHandler)(DWORD, DWORD) = NULL;
static bool g_switchingWindow = false; // don't quit in the closing of the window which is done when reiniting the window
static bool g_minimized = false;
static bool g_resizeStretch = false; // set by a flag from drd_init()
static bool g_backBufferAttachment = false; // this will be true in "true" full screen more that has one surface with two buffers (does flip differently)

typedef ArrayMap<UINT, int(__stdcall*)(DWORD, DWORD, DWORD), 20> MsgHandlers;
static MsgHandlers* g_msgHandlers = NULL;

// OpenGL
HDC g_glHDC = 0;
HGLRC g_glHRC = 0;



void __stdcall drd_setErrorHandler(void(__stdcall *callback)(const char*) ) {
    g_errorHandler = callback;
}

HDC __stdcall drd_beginHdc() {
    CHECKR_INITDRD(g_pDDSBack, NULL);
    HDC hdc;
    if (g_pDDSBack->GetDC(&hdc) != DD_OK) {
        msgError("Failed GetDC");
    }
    return hdc;
}

void __stdcall drd_endHdc(HDC hdc) {
    CHECK_INIT(g_pDDSBack);
    if (g_pDDSBack->ReleaseDC(hdc) != DD_OK) {
        msgError("Failed ReleaseDC");
    }
}

HWND __stdcall drd_getMainHwnd() {
    return g_hMainWnd;
}


void __stdcall drd_setWindowTitle(const char* str) {
    CHECK_INITDRD(g_hMainWnd);
    SetWindowTextA(g_hMainWnd, str);
}

#define MAX_TEMPLAET 500
class DialogTemplate {
public:
    DialogTemplate(int cx, int cy) {
        mZeroMemory(&data, MAX_TEMPLAET);
        header = (DLGTEMPLATE*)&data;
        header->style = WS_CAPTION | WS_VISIBLE | WS_SYSMENU;
        header->cx = cx; header->cy = cy;
        cur = (WORD*)(header + 1) + 3;
    }
    void addStatic(int x, int y, int cx, int cy, int id, wchar_t* str) {
        ++header->cdit;
        DLGITEMTEMPLATE* item = (DLGITEMTEMPLATE*)cur;
        item->style = WS_VISIBLE | WS_CHILD;
        item->x = x;   item->y = y;
        item->cx = cx; item->cy = cy;
        item->id = id;
        cur = (WORD*)(item + 1);
        (*cur++) = 0xffff;
        (*cur++) = 0x0082; // static
        cur += mwstrcat_s((wchar_t*)cur, (int)(data + MAX_TEMPLAET - (char*)cur), str);
        //(*cur++) = 
    }

    char data[MAX_TEMPLAET];
    DLGTEMPLATE* header;
    WORD* cur;
};

void runAboutDialog() {
    DialogTemplate dt(160, 55);
    dt.addStatic(10, 10, 150, 40, 101, L"DRD Version " VERSION L"\nhttps://github.com/shooshx/DirectDrawTest\nShy Shalom, 2015\nshooshx@gmail.com");

    HWND dlg = CreateDialogIndirectW(GetModuleHandle(NULL), dt.header, g_hMainWnd, NULL);
}


void handleWmCommand(DWORD notify, HWND hwnd, DWORD v) 
{
    static char* buf = NULL;
    static int buflen = 0;

    CtrlBase* obj = (CtrlBase*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (obj == NULL) // a child we created
        return;
    if (lstrcmpA(obj->type, "EDIT") == 0) {
        auto that = (EditCtrl*)obj;
        if (notify ==  EN_CHANGE) {
            int sz = GetWindowTextLengthA(hwnd);
            if (buf == NULL || sz > buflen) {
                LocalFree((HLOCAL)buf);
                buf = (char*)LocalAlloc(0, sz * 2);
                buflen = sz * 2;
            }
            GetWindowTextA(hwnd, buf, buflen);
            if (that->textChanged != NULL) {
                that->textChanged(that->c.id, buf);
            }
        }
    }
    else if (lstrcmpA(obj->type, "BUTTON") == 0) {
        auto that = (ButtonCtrl*)obj;
        if (notify == BN_CLICKED) {
            if (that->clicked != NULL) {
                that->clicked(that->c.id);
            }
        }
    }
    else if (lstrcmpA(obj->type, TRACKBAR_CLASSA) == 0) {
        auto that = (SliderCtrl*)obj;
        if (notify == WM_HSCROLL) {
            int req = LOWORD(v);
            int value;
            if (req == SB_THUMBPOSITION || req == SB_THUMBTRACK) {
                value = HIWORD(v);
            }
            else {
                value = SendMessage(hwnd, TBM_GETPOS, 0, 0);
            }
            that->changed(that->c.id, value);
        }

    }
}

static bool initDDrawWindow(HWND hwnd, DWORD width, DWORD height, bool fullScreen);
static void cleanSurface();

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // look for a custom handler from the user
    if (g_msgHandlers != NULL) {
        auto handler = g_msgHandlers->get(message);
        if (handler != NULL) {
            int next = handler(message, wParam, lParam);
            if (next >= 0)
                return next;
        }
    }

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
        else if (wParam == MYSC_ABOUT) {
            runAboutDialog();
        }
        break;
    case WM_SIZE: {
        int w = lParam & 0xffff;
        int h = lParam >> 16;
        if (g_isWindowed && wParam != SIZE_MINIMIZED) {
            if (w != g_wndWidth || h != g_wndHeight) {
                g_wndWidth = w;
                g_wndHeight = h;
                if (!g_resizeStretch) {
                    cleanSurface();
                    if (!initDDrawWindow(g_hMainWnd, g_wndWidth, g_wndHeight, false)) {
                        msgError("Failed recreating DirectDraw surface");
                    }
                }
            }
            // initial resize on creation of the window should to to the callback
            if (g_resizeHandler != NULL) {
                g_resizeHandler(g_wndWidth, g_wndHeight);
            }
        }
        return 0;
    }
    case WM_COMMAND:  // from controls
        handleWmCommand(HIWORD(wParam), (HWND)lParam, 0);
        break;
    case WM_CTLCOLORBTN: // cause the background of buttons to be black
        return (LRESULT)GetStockObject(BLACK_BRUSH);
    case WM_HSCROLL:
        handleWmCommand(message, (HWND)lParam, wParam);
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
void __stdcall drd_setResizeHandler(void(__stdcall *callback)(DWORD, DWORD)) {
    g_resizeHandler = callback;
}
void __stdcall drd_setWinMsgHandler(DWORD msg, int(__stdcall *callback)(DWORD, DWORD, DWORD)) {
    static MsgHandlers inst;
    if (g_msgHandlers == NULL)
        g_msgHandlers = &inst;
    if (!g_msgHandlers->add(msg, callback)) {
        msgError("too many window message handlers");
    }
}

static bool checkFlag(DWORD v, DWORD flag) {
    return ((v & flag) == flag);
}



// width, height are pointer since in INIT_WINDOWFULL they are set according to the desktop
static HWND createWindow(DWORD* width, DWORD* height, DWORD flags)
{
    bool isFullScreen = checkFlag(flags, INIT_FULLSCREEN);
    bool isWindowFull = checkFlag(flags, INIT_WINDOWFULL);
    bool isInputFall = checkFlag(flags, INIT_INPUT_FALLTHROUGH);
    bool withResize = checkFlag(flags, INIT_RESIZABLE);

    if (isFullScreen) {
        if (isWindowFull) {
            msgError("Can't create a window both full-screen and window-full-screen");
            return NULL;
        }
        if (isInputFall) {
            msgError("Can't create a full screen and with mouce fall-through");
            return NULL;
        }
        if (withResize) {
            msgError("Can't create a full screen with resize border");
            return NULL;
        }
    }

    g_isWindowed = !isFullScreen;
    g_resizeStretch = checkFlag(flags, INIT_RESIZABLE_STRETCH);

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
        style = WS_SYSMENU | WS_MINIMIZEBOX | WS_OVERLAPPED | WS_CAPTION;
        if (withResize) {
            style |= WS_SIZEBOX | WS_MAXIMIZEBOX;
        }
    }
    else {
        exstyle = WS_EX_TOPMOST;
        style = WS_POPUP;
    }

    if (isInputFall) {
        exstyle |= WS_EX_TRANSPARENT | WS_EX_TOPMOST;
    }

    // need to be set before CreateWindow since CreateWindow causes a WM_SIZE
    g_wndWidth = *width;
    g_wndHeight = *height;

    RECT adr = { 0, 0, *width, *height };
    AdjustWindowRect(&adr, style, FALSE); // WS_OVERLAPPEDWINDOW see http://www.directxtutorial.com/Lesson.aspx?lessonid=11-1-4
    DWORD cwidth = adr.right - adr.left;
    DWORD cheight = adr.bottom - adr.top;

    hWnd = CreateWindowExA(
        exstyle, //WS_EX_TOPMOST,
        "WndClass", "drd",
        style, //WS_POPUP,
        x, y, cwidth, cheight,
        NULL, NULL, hInst, NULL);

    if (g_isWindowed) {
        InsertMenuA(GetSystemMenu(hWnd, FALSE), 0, MF_BYPOSITION | MF_STRING, MYSC_ABOUT, "About...");
    }

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
static bool initDDrawWindow(HWND hwnd, DWORD width, DWORD height, bool fullScreen)
{
    HRESULT ddrval;

    if (!fullScreen) {
        // using DDSCL_NORMAL means we will coexist with GDI
        ddrval = g_pDD->SetCooperativeLevel(hwnd, DDSCL_NORMAL);
        if (ddrval != DD_OK) {
            return(false);
        }
    }
    else {
        ddrval = g_pDD->SetCooperativeLevel(hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
        if (ddrval != DD_OK)
            return false;

        ddrval = g_pDD->SetDisplayMode(width, height, 32, 0, 0);
        if (ddrval != DD_OK) {
            msgError("FullScreen: Failed SetDisplayMode", ddrval);
            return false;
        }
    }

    DDSURFACEDESC2 ddsd;
    mZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof( ddsd );
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    // The primary surface is not a page flipping surface this time
    ddrval = g_pDD->CreateSurface(&ddsd, &g_pDDSFront, NULL);
    if (!checkHr(ddrval, "init CreateSurface front"))
        return false;

    // Create a clipper to ensure that our drawing stays inside our window
    ddrval = g_pDD->CreateClipper(0, &g_clipper, NULL);
    if (!checkHr(ddrval, "init CreateClipper"))
        return false;

    // setting it to our hwnd gives the clipper the coordinates from our window
    ddrval = g_clipper->SetHWnd(0, hwnd);
    if (!checkHr(ddrval, "init SetHWnd"))
        return false;

    // attach the clipper to the primary surface
    ddrval = g_pDDSFront->SetClipper(g_clipper);
    if (!checkHr(ddrval, "init SetClipper"))
        return false;

    mZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = width;
    ddsd.dwHeight = height;

    // create the backbuffer separately
    ddrval = g_pDD->CreateSurface(&ddsd, &g_pDDSBack, NULL);
    if (!checkHr(ddrval, "init CreateSurface back"))
        return false;

    g_surfaceWidth = width;
    g_surfaceHeight = height;
    g_backBufferAttachment = false;

    return true;
}

static bool initDDrawFullScreen(HWND hwnd, DWORD width, DWORD height)
{
    HRESULT ddrval;
    ddrval = g_pDD->SetCooperativeLevel(hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    if (ddrval != DD_OK)
        return false;

    ddrval = g_pDD->SetDisplayMode(width, height, 32 ,0, 0);
    if (!checkHr(ddrval, "FullScreen: Failed SetDisplayMode")) {
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
    if (!checkHr(ddrval, "FullScreen: Failed CreateSurface")) {
        return false;
    }

    // Get the pointer to the back buffer
    DDSCAPS2 ddscaps;
    mZeroMemory(&ddscaps, sizeof(ddscaps));
    ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
    ddrval = g_pDDSFront->GetAttachedSurface(&ddscaps, &g_pDDSBack); //DDERR_INVALIDOBJECT
    if (!checkHr(ddrval, "FullScreen: GetAttachedSurface")) {
        return false;
    }

    g_surfaceWidth = width;
    g_surfaceHeight = height;
    g_backBufferAttachment = true;

    return true;
}

static void cleanSurface() {
    if (g_pDDSBack)
        g_pDDSBack->Release();
    g_pDDSBack = NULL;
    if (g_pDDSFront)
        g_pDDSFront->Release();
    g_pDDSFront = NULL;
    if (g_clipper)
        g_clipper->Release();
    g_clipper = NULL;
}
static void cleanUp()
{
    if (g_hMainWnd) 
        DestroyWindow(g_hMainWnd);
    g_hMainWnd = NULL;
    cleanSurface();
    if (g_pDD)
        g_pDD->Release();
    g_pDD = NULL;
}



bool __stdcall drd_init(DWORD width, DWORD height, DWORD flags) 
{
    if (g_ddrawModule == NULL) {
        g_ddrawModule = LoadLibraryW(L"DDRAW.DLL");
        if (g_ddrawModule == NULL) {
            msgErrorLE("Failing loading ddraw.dll ");
            return false;
        }
        pDirectDrawCreateEx = (TDirectDrawCreateEx)GetProcAddress(g_ddrawModule, "DirectDrawCreateEx");
        if (pDirectDrawCreateEx == NULL) {
            msgErrorLE("Failing GetProcAdddress DirectDrawCreateEx ");
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
 
    HRESULT ddrval = pDirectDrawCreateEx(NULL, (VOID**)&g_pDD, MY_IID_IDirectDraw7, NULL);
    if (ddrval != DD_OK) {
        msgError("Could start DirectX engine in your computer.\nMake sure you have at least version 7 of DirectX installed.");
        return false;
    }

    bool initOk = false;
    if (g_isWindowed) {
        initOk = initDDrawWindow(g_hMainWnd, width, height, false);
	}
    else {
        initOk = initDDrawWindow(g_hMainWnd, width, height, true);  
		//initDDrawFullScreen(g_hMainWnd, width, height);
	}

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

    return true;
}



#define GET_FUNC(name, type) using T##name = type; T##name name = (T##name)GetProcAddress(mod, #name);

struct GLFuncs {
    HMODULE mod = LoadLibraryA("opengl32.dll");

    GET_FUNC(wglCreateContext, HGLRC(WINAPI*)(HDC));
    GET_FUNC(wglMakeCurrent, BOOL(WINAPI*)(HDC, HGLRC));

    static GLFuncs& instance() {
        static GLFuncs f;
        return f;
    }
};


bool __stdcall drd_initGL()
{
    CHECKR_INITDRD(g_hMainWnd, false);

    auto f = GLFuncs::instance();
    if (f.mod == NULL) {
        msgError("Failed loading opengl32.dll");
        return false;
    }

    g_glHDC = GetDC(g_hMainWnd);  //get current windows device context

    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),          //size of structure
        1,                                      //default version
        PFD_DRAW_TO_WINDOW |                    //window drawing support
        PFD_SUPPORT_OPENGL |                    //opengl support
        PFD_DOUBLEBUFFER,                       //double buffering support
        PFD_TYPE_RGBA,                          //RGBA color mode
        32,                                     //32 bit color mode
        0, 0, 0, 0, 0, 0,                       //ignore color bits
        0,                                      //no alpha buffer
        0,                                      //ignore shift bit
        0,                                      //no accumulation buffer
        0, 0, 0, 0,                             //ignore accumulation bits
        16,                                     //16 bit z-buffer size
        0,                                      //no stencil buffer
        0,                                      //no aux buffer
        PFD_MAIN_PLANE,                         //main drawing plane
        0,                                      //reserved
        0, 0, 0 };                              //layer masks ignored

    int nPixelFormat = ChoosePixelFormat(g_glHDC, &pfd);
    if (nPixelFormat == 0) {
        msgErrorLE("Failed ChoosePixelFormat ");
        return false;
    }
    if (!SetPixelFormat(g_glHDC, nPixelFormat, &pfd)) {
        msgErrorLE("Failed SetPixelFormat ");
        return false;
    }

    g_glHRC = f.wglCreateContext(g_glHDC);
    if (g_glHRC == NULL) {
        msgErrorLE("Failed wglCreateContext ");
        return false;
    }
    if (!f.wglMakeCurrent(g_glHDC, g_glHRC)) {
        msgErrorLE("Failed wglMakeCurrent ");
        return false;
    }
    return true;
}

void __stdcall drd_flipGL()
{
    SwapBuffers(g_glHDC);
}

void __stdcall drd_flip()
{
    CHECK_INITDRD(g_pDDSBack);
    if (g_minimized)
        return; // it's going to fail for a minimized window.

    // if we're windowed do the blit, else just Flip
    if (!g_backBufferAttachment) { //g_isWindowed) {
        // first we need to figure out where on the primary surface our window lives
        POINT p = {0,0};
        ClientToScreen(g_hMainWnd, &p);

        RECT rcRectDest;
        
        /*if (rcRectDest.right != g_wndWidth) { // replaced with AdjustRect
            int borderWidth = p.x - gr.left;
            int extraHeight = p.y - gr.top + borderWidth;
            MoveWindow(g_hMainWnd, gr.left, gr.top, g_wndWidth + borderWidth * 2, g_wndHeight + extraHeight, FALSE);
        }*/
        //GetClientRect(g_hMainWnd, &rcRectDest);
        SetRect(&rcRectDest, 0, 0, g_wndWidth, g_wndHeight);
        OffsetRect(&rcRectDest, p.x, p.y);
        RECT rcRectSrc;
        SetRect(&rcRectSrc, 0, 0, g_surfaceWidth, g_surfaceHeight);

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
    else { // TBD - get rid of this, make it work the same as above
        HRESULT ddrval = g_pDDSFront->Flip(NULL, DDFLIP_WAIT);
        checkHr(ddrval, "Failed flip");
    }
}


void __stdcall drd_pixelsBegin(CPixelPaint* pp) {
    CHECK_INIT(pp);
    CHECK_INITDRD(g_pDDSBack);

    DDSURFACEDESC2 ddsd;
    mZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    HRESULT hr = g_pDDSBack->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
    checkHr(hr, "Failed Lock");
    pp->buf = (DWORD*)ddsd.lpSurface;
    pp->pitch = ddsd.lPitch;
    pp->bytesPerPixel = g_srfPixelBytes;
    pp->cheight = g_surfaceHeight;
    pp->cwidth = g_surfaceWidth;
    pp->wpitch = pp->pitch / pp->bytesPerPixel;
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
    CHECK_INITDRD(g_pDDSBack);
    DDBLTFX fx;
    fx.dwSize = sizeof(fx);
    fx.dwFillColor = color;
    HRESULT ddrval = g_pDDSBack->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &fx);
    checkHr(ddrval, "clear failed Blt");
}

void __stdcall drd_imageDrawCrop(CImg* img, int dstX, int dstY, int srcX, int srcY, int srcWidth, int srcHeight) 
{
    CHECK_INIT(img);
    CHECK_INIT_MSG(img->surface, "image not initialized, call drd_imageLoadFile or drd_imageLoadResource first");
    CHECK_INITDRD(g_pDDSBack);
    RECT srcRect;
    SetRect(&srcRect, srcX, srcY, srcX + srcWidth, srcY + srcHeight);

    // crop edge of window
    if (dstX >= (int)g_surfaceWidth || dstY >= (int)g_surfaceHeight || dstX + srcWidth <= 0 || dstY + srcHeight <= 0)
        return; // nothing to display

    int ddx = dstX + srcWidth - g_surfaceWidth;
    if (ddx > 0)
        srcRect.right -= ddx;
    int ddy = dstY + srcHeight - g_surfaceHeight;
    if (ddy > 0)
        srcRect.bottom -= ddy;
    if (dstX < 0) {
        srcRect.left -= dstX;
        dstX = 0;
    }
    if (dstY < 0) {
        srcRect.top -= dstY;
        dstY = 0;
    }
    if (srcRect.bottom > (int)img->height)
        srcRect.bottom = img->height;
    if (srcRect.right >(int)img->width)
        srcRect.right = img->width;
    if (srcRect.top < 0)
        srcRect.top = 0;
    if (srcRect.left < 0)
        srcRect.left = 0;

    DWORD flags = DDBLTFAST_WAIT;
    if (img->hasSrcKey)
        flags |= DDBLTFAST_SRCCOLORKEY;

    HRESULT ddrval = g_pDDSBack->BltFast(dstX, dstY, img->surface, &srcRect, flags); //DDBLTFAST_NOCOLORKEY
    checkHr(ddrval, "imageDraw failed BltFast");
}

void __stdcall drd_imageDraw(CImg* img, int dstX, int dstY) {
    CHECK_INIT(img); // need to be here since we access img
    drd_imageDrawCrop(img, dstX, dstY, 0, 0, img->width, img->height);
}


static bool imageLoad(const char* filename, DWORD id, CImg* ret)
{
    CHECKR_INIT_MSG(ret, false, "pointer to Img struct should not be NULL");
    CHECKR_INITDRD(g_pDD, false);

    BITMAP bm;
    IDirectDrawSurface7 *pdds;

    HBITMAP hbm;
    if (filename != NULL) {
        hbm = (HBITMAP)LoadImageA(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
        if (hbm == NULL) {
            msgError("imageLoadFile: Failed to load file ", filename, " ", lastErrorStr());
            return false;
        }
    }
    else {
        hbm = (HBITMAP)LoadImageA(GetModuleHandle(NULL), MAKEINTRESOURCEA(id), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        if (hbm == NULL) {
            msgError("imageLoadFile: Failed to load resoruce", id, " ", lastErrorStr());
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

    HRESULT ddrval = g_pDD->CreateSurface(&ddsd, &pdds, NULL);
    if (!checkHr(ddrval, "imageLoadFile: failed CreateSurface")) {
        return false;
    }

    // copy bitmap to surface
    HDC hdcImage = CreateCompatibleDC(NULL);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcImage, hbm);
    HDC hdc;
    ddrval = pdds->GetDC(&hdc);
    if (!checkHr(ddrval, "imageLoadFile: failed GetDC")) {
        return false;
    }

    if (BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcImage, 0, 0, SRCCOPY) == 0) {
        msgErrorLE("imageLoadFile: failed BitBlt ");
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
    CHECK_INIT_MSG(img->surface, "image not initialized, call drd_imageLoadFile or drd_imageLoadResource first");

    DDCOLORKEY ddck;
    ddck.dwColorSpaceLowValue = color;
    ddck.dwColorSpaceHighValue = color;
    HRESULT hr = img->surface->SetColorKey(DDCKEY_SRCBLT, &ddck);
    checkHr(hr, "Failed SetColorKey");
    img->hasSrcKey = TRUE;
}

void __stdcall drd_imageDelete(CImg* img) {
    if (img == NULL)
        return;
    if (img->surface != NULL) {
        img->surface->Release();
    }
    img->surface = NULL;
    img->height = 0;
    img->width = 0;
}


void __stdcall drd_printFps(const char* filename) 
{
    static int frameCount = 0;
    static DWORD lastTime = 0;
    
    if (filename == NULL)
        filename = "_drd_fps.txt";

    ++frameCount;
    DWORD time = GetTickCount();
    if (time - lastTime > 1000) 
    {
        char buf[100];
        mZeroMemory(buf, 100);
        mitoa(frameCount, buf, 10);
        lastTime = time;
        frameCount = 0;

        mstrcat_s(buf, 100, " fps\r\n");

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


HWND g_controlDlg = NULL;

HWND __stdcall drd_createCtrlWindow(int width, int height) {
    RECT r = {0, 0, width, height};
    DWORD style = WS_SYSMENU | WS_MINIMIZEBOX | WS_OVERLAPPED | WS_CAPTION;
    AdjustWindowRect(&r, style, FALSE);
    width = r.right - r.left;
    height = r.bottom - r.top;

    g_controlDlg = CreateWindowExW(0, WC_DIALOG, L"controls", style | WS_VISIBLE, 
                                   CW_USEDEFAULT, CW_USEDEFAULT, width, height, g_hMainWnd, NULL, NULL, NULL);
    SetWindowLongPtr(g_controlDlg, GWLP_WNDPROC, (LONG_PTR)WndProc);
    
    return g_controlDlg;
}

HWND __stdcall drd_createCtrl(void* vc)
{
    auto* c = (CtrlBase*)vc;
    if (lstrcmpA(c->type, "SLIDER") == 0) {
        c->type = TRACKBAR_CLASSA;
    }
    HWND hw = CreateWindowExA(c->styleex, c->type, c->initText, WS_CHILD | WS_VISIBLE | c->style, c->x, c->y, c->width, c->height, g_controlDlg, (HMENU)c->id, NULL, NULL);
    SetWindowLongPtr(hw, GWLP_USERDATA, (ULONG_PTR)c);

    SendMessage(hw, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FIXED_FONT), TRUE);

    return hw;
}

// link to the new style common controls. see http://msdn.microsoft.com/en-us/library/bb773175.aspx
#pragma comment(linker,"\"/manifestdependency:type='win32' \
       name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
       processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


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
        if (msg.message == WM_QUIT) {
            return FALSE;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
        // quit message might just have been posted
        if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
            if (msg.message == WM_QUIT) {
                return FALSE;
            }
        }
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