#include <Windows.h>
#include <gl/gl.h>
#include <math.h>

#include "drd.h"

#pragma comment (lib, "opengl32.lib")

#define WIN_WIDTH 800
#define WIN_HEIGHT 600

#define PI 3.14159265359f

struct Vec2 {
    float x, y;
};
int g_mouseX = 0, g_mouseY = 0;

void __stdcall mouseHandler(DWORD msg, DWORD wParam, DWORD lParam)
{
    g_mouseX = lParam & 0xffff;
    g_mouseY = WIN_HEIGHT - (lParam >> 16);
}

// 3d graphics
int main()
{
    drd_init(WIN_WIDTH, WIN_HEIGHT, INIT_WINDOW);
    drd_initGL();
    drd_setMouseHandler(mouseHandler);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glMatrixMode(GL_PROJECTION);
    float viewWidth = (float)WIN_WIDTH / (float)WIN_HEIGHT * 4.0f;
    glOrtho(-viewWidth, viewWidth, -4.0, 4.0, -4.0, 4.0); // maintain the aspect ratio 

    glMatrixMode(GL_MODELVIEW); // any further transformations go to the model view matrix

    float mouseScale = (float)WIN_HEIGHT / 8.0f;
    Vec2 mouseOffset = { -viewWidth, -4.0f };

    float vtx[] = { 1,1,1, 1,-1,1, -1,-1,1, -1,1,1,
                    1,1,-1, 1,-1,-1, -1,-1,-1, -1,1,-1};
    unsigned short idx[] = { 3,2,1,0, 4,5,6,7, 1,5,4,0, 0,4,7,3, 2,6,5,1, 3,7,6,2 };

    glEnableClientState(GL_INDEX_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE); 
    float pos[] = { 0.0, 4.0, 10.0, 0.0 };
    glLoadIdentity();
    glLightfv(GL_LIGHT0, GL_POSITION, pos);

    float ang = 0;

    while (drd_processMessages()) 
    {
        glLoadIdentity();
        float tx = (float)g_mouseX / mouseScale + mouseOffset.x;
        float ty = (float)g_mouseY / mouseScale + mouseOffset.y;
        glTranslatef(tx, ty, 0.0f);

        glRotatef(ang, 1.0f, 0.0f, 0.0f);
        glRotatef(ang * 2, 0.0f, 1.0f, 0.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glVertexPointer(3, GL_FLOAT, 0, vtx);
        glNormalPointer(GL_FLOAT, 0, vtx); // use the vertex positions as the normals 
        glDrawElements(GL_QUADS, 6*4, GL_UNSIGNED_SHORT, idx);

        glEnd();
        drd_flipGL();
        Sleep(10);

        ang += 0.5;
    }
    return 0;
}


// 2d graphics
int main2()
{
    drd_init(WIN_WIDTH, WIN_HEIGHT, INIT_WINDOW);
    drd_initGL();
    drd_setMouseHandler(mouseHandler);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glMatrixMode(GL_PROJECTION);
    glOrtho(0.0, (float)WIN_WIDTH, 0.0, (float)WIN_HEIGHT, 100.0, -100.0);

    glMatrixMode(GL_MODELVIEW);
    
    while (drd_processMessages()) 
    {
        glLoadIdentity();
        glTranslatef((float)g_mouseX, (float)g_mouseY, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex2f(0.0f, 0.0f);

        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex2f(100.0f, 0.0f);

        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex2f(100.0f, 100.0f);

        glEnd();
        drd_flipGL();
        Sleep(10);
    }
    return 0;
}


// in case this is compiled as a windows application
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    return main();
}