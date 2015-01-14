#include <Windows.h>
#include "drd.h"
#include <math.h>


void __stdcall editHandler(int id, const char* text) {
    int i = 0;
}

void __stdcall pressHandler(int id) {
    int i = 0;
}

void swap(int& x, int& y) {
    int c = x;
    x = y;
    y = c;
}

CPixelPaint pp;
int mx = 0, my = 0;

#define PUT_PIXEL(x, y, c) *(pp.buf + x + y * pp.wpitch) = c

void drawLine(int x1, int y1, int x2, int y2) 
{
    float dx = (float)(x2 - x1);
    float dy = (float)(y2 - y1);
    if (abs(dx) > abs(dy)) {
        int minx = min(x1, x2);
        int maxx = max(x1, x2);
        for (int x = minx; x <= maxx; ++x ) {
            int y = y1 + (int)( dy * (float)(x - x1) / dx );
            PUT_PIXEL(x, y, RGB(255, 255, 255));
        }
    }
    else {
        int miny = min(y1, y2);
        int maxy = max(y1, y2);
        for (int y = miny; y <= maxy; ++y ) {
            int x = x1 + (int)( dx * (float)(y - y1) / dy );
            PUT_PIXEL(x, y, RGB(255, 255, 255));
        }

    }

}

void __stdcall mouseHandler(DWORD msg, DWORD wParam, DWORD lParam) {
    mx = (short)(lParam & 0xffff);
    my = (short)(lParam >> 16);
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    drd_init(800, 600, INIT_WINDOW);

//    EditCtrl ed = { "EDIT", 1, 700, 10, 190, 500, ES_MULTILINE, WS_EX_CLIENTEDGE, "", editHandler };
//    drd_createCtrl(&ed);
//    ButtonCtrl b = { "BUTTON", 2, 700, 520, 70, 30,  BS_PUSHBUTTON, 0, "Do It!", pressHandler };
//    drd_createCtrl(&b);

    drd_setMouseHandler(mouseHandler);


    while (drd_processMessages()) {

        drd_pixelsClear(0);
        drd_pixelsBegin(&pp);

        int x = 100, y = 100;
        DWORD* ptr = pp.buf + y * pp.wpitch + x;
        *ptr = 0xFFFFFF;

        drawLine(300, 300, mx, my);

        drd_pixelsEnd();
        drd_flip();


        Sleep(1);
    }

    return 0;
}


