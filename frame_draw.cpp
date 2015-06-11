#include <Windows.h>
#include "drd.h"
#include "CommCtrl.h"


DWORD getPixelColor(DWORD xi, DWORD yi, DWORD fi)
{
    return xi + yi;
};

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    drd_init(800, 600, INIT_WINDOW);

    CPixelPaint pp;
    DWORD f = 0;
    bool isAnim = false;


    while(true) {

        drd_pixelsBegin(&pp);
        DWORD* lineStart = pp.buf;
        for(DWORD y = 0; y < pp.cheight; ++y) {
            DWORD* p = lineStart;
            for(DWORD x = 0; x < pp.cwidth; ++x) {
                DWORD c = getPixelColor(x, y, f);
                *p = c;
                ++p;
            }
            lineStart = (DWORD*)(((char*)lineStart) + pp.pitch); // pitch is in bytes
        }
        drd_pixelsEnd();

        HDC hdc = drd_beginHdc();
        SelectObject(hdc, GetStockObject(WHITE_PEN));
        MoveToEx(hdc, 100, 100, NULL);
        LineTo(hdc, 500, 500);
        drd_endHdc(hdc);

        drd_flip();
        

        if (!isAnim && MessageBoxA(NULL, "DirectDraw", "Continue?", MB_YESNO) == IDNO)
            break;
        if (!drd_processMessages())
            break;
        isAnim = true;
        ++f;
    }
}