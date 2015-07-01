#include "drd.h"

#define USER_RADIUS 25


int mx = 0, my = 0;
bool mclick = false;

void __stdcall mouseHandler(DWORD msg, DWORD wParam, DWORD lParam) {
    mx = (short)(lParam & 0xffff);
    my = (short)(lParam >> 16);
    if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN)
        mclick = true;
}


bool isColliding(CPixelPaint& pp, int r) {
    int dr = (int)(0.707106f * (float)r);
    struct { int x,y; } offsets[] = { 0,-r, 0,r, r,0, -r,0, -dr,-dr, dr,-dr, -dr,dr, dr,dr };
    
    for (auto offset: offsets) 
    {
        int testx = mx + offset.x;
        int testy = my + offset.y;
        if (testx < 0 || testy < 0 || testx >= (int)pp.cwidth || testy >= (int)pp.cheight)
            continue;
        DWORD col = pp.buf[testx + (testy)* pp.wpitch];
        if ((col & 0x00ffffff) != 0x00ffffff) // mask the alpha channel
            return true;
    }
    return false;
}

int main()
{
    drd_init(800, 600, INIT_WINDOW);

    drd_setMouseHandler(mouseHandler);
    CImg img[3];
    drd_imageLoadFile("pi.png", &img[0]);
    drd_imageLoadFile("kitten.jpg", &img[1]);
    drd_imageLoadResource(101, &img[2]);
    int useImg = 0;

    CPixelPaint pp;

    while (drd_processMessages()) 
    {
        if (mclick) {
            useImg = (useImg + 1) % 3;
            mclick = false;
        }
        drd_pixelsClear(0x00ffffff);

        drd_imageDraw(&img[useImg], 100, 50);

        drd_pixelsBegin(&pp);
        bool collide = isColliding(pp, USER_RADIUS);
        drd_pixelsEnd();

        HDC hdc = drd_beginHdc();
        SelectObject(hdc, GetStockObject(DC_BRUSH));
        SetDCBrushColor(hdc, collide ? RGB(255, 20, 20) : RGB(80, 80, 255));
        Ellipse(hdc, mx - USER_RADIUS, my - USER_RADIUS, mx + USER_RADIUS, my + USER_RADIUS);
        drd_endHdc(hdc);

        drd_flip();
        Sleep(10);
    }
    return 0;
}


// in case this is compiled as a windows application
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    return main();
}