#include <windows.h>

extern "C" {
#pragma comment (lib,"drd.lib")

struct IDirectDrawSurface7;

struct CImg {
    IDirectDrawSurface7* surface;
    DWORD width;
    DWORD height;
    DWORD hasSrcKey;  // was there a transparency color set
};

struct CPixelPaint {
    DWORD* buf;
    DWORD pitch; // in bytes
    DWORD bytesPerPixel;
    DWORD cheight;
    DWORD cwidth;
};

#define INIT_WINDOW 0
#define INIT_FULLSCREEN 1
#define INIT_WINDOWFULL 2
#define INIT_INPUT_FALLTHROUGH 4

bool __stdcall drd_init(DWORD width, DWORD height, DWORD flags);
void __stdcall drd_windowSetTranslucent(BYTE alpha);

void __stdcall drd_setErrorHandler(void(__stdcall *callback)(const char*) );
void __stdcall drd_setKeyHandler(void(__stdcall *callback)(DWORD) );
void __stdcall drd_setMouseHandler(void(__stdcall *callback)(DWORD, DWORD, DWORD) );

void __stdcall drd_pixelsBegin(CPixelPaint* pp);
void __stdcall drd_pixelsEnd();
void __stdcall drd_pixelsClear(DWORD color);
void __stdcall drd_flip();

bool __stdcall drd_imageLoadFile(const char* filename, CImg* ret);
bool __stdcall drd_imageLoadResource(DWORD id, CImg* ret);
void __stdcall drd_imageDelete(CImg* img);
void __stdcall drd_imageSetTransparent(CImg* img, DWORD color);
void __stdcall drd_imageDraw(CImg* img, int dstX, int dstY);
void __stdcall drd_imageDrawCrop(CImg* img, int dstX, int dstY, int srcX, int srcY, int srcWidth, int srcHeight);

BOOL __stdcall drd_processMessages();
void __stdcall drd_printFps(const char* filename);


}