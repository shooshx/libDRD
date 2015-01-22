#include <Windows.h>
#include <CommCtrl.h>

#include "drd.h"
#include <math.h>
#include <string>
#include <vector>
#include <sstream>
#include <map>

using namespace std;

void drawLine(int x1, int y1, int x2, int y2, unsigned int color);


unsigned int ip = 0;
string text;

void readSpace() {
    char nc = text[ip];
    while (ip < text.length() && (nc == ' ' || nc == '\n' || nc == '\t' || nc == '\r')) {
        ++ip;
        nc = text[ip];
    } 
}

string readChars() {
    readSpace();
    string s;
    char nc = text[ip];
    while (ip < text.length() && (nc != ' ' && nc != '\n' && nc != '\t' && nc != '\r')) {
        s += nc;
        ++ip;
        nc = text[ip];
    }
    return s;
}

HWND g_outwnd;
string outText;
void out(const string& s) {
    outText += s + "\n";
}
void error(const string& s) {
    out(s);
    throw std::exception(s.c_str());
}
    
//var isNum = function(nc) { return (nc >= "0" && nc <= "9") || nc =='-' || nc == '.'}
//var isalpha = function(nc) { return (nc >= "a" && nc <= "z") || (nc >= "A" && nc <= "Z") } 

//var funcs = {}

map<string, int> globals;  // top level vars are the globals
map<string, int> *vars = NULL;
int scrollValue[2] = {0};


int getVar(const string& name) {
    auto it = vars->find(name);
    if (it != vars->end()) 
        return it->second;
    auto git = globals.find(name);
    if (git != globals.end())
        return git->second;
    if (name == "scroll0")
        return scrollValue[0];
    if (name == "scroll1")
        return scrollValue[1];
    return INT_MAX;
}
    
int readNum() {
    string name = readChars();
    int n = getVar(name);
    if (n != INT_MAX)
        return n;
    return atoi(name.c_str()); // better - strtoul
}

double readFloat() {
    string name = readChars();
    int n = getVar(name);
    if (n != INT_MAX)
        return (double)n;
    return atof(name.c_str()); // better - strtoul
}

/*
var ipToLineNo = function(ip) {
    var lno = 1
    var n = Math.min(ip, text.length)
    for(var i = 0; i < n; ++i)
        if (text[i] == '\n')
            lno++
    return lno    
}
*/

   
struct LoopEntry {
    int timesLeft;
    int toip;
};

struct Func {
    vector<string> args;
    int startip;
};
struct Frame {
    map<string, int> *returnVars;
    int returnIp;
};

#define PI 3.14159265358979323846
   
void interpret() 
{
    int x = 400; //canvas.width / 2
    int y = 300; //canvas.height / 2
    int ang = 0;
    ip = 0;

    unsigned int color = 0xffffff;
    unsigned int startTime = GetTickCount();
    unsigned int allocTime = 1000;

    map<string, Func> funcs;
    globals.clear();
    vars = &globals;
    outText.clear();

    vector<LoopEntry> loopStack; // elements are from 'do'
    vector<Frame> callStack; // elements are from function call, things that are below the current call which is in vars

    try {
        while (ip < text.length()) {
            //if (GetTickCount() - startTime > allocTime)
            //    error("too much time");
            string c = readChars();
            if (c[0] == '#') { // comment
                ip = text.find('\n', ip);
                continue;
            }
            if (c.empty()) 
                continue;
            if (c == "F" || c == "Fs" || c == "B" || c == "Bs") {
                int n = readNum();
                if (c[0] == 'B')
                    n = -n;
                int px = x, py = y;
                x += (int)(n * cos(ang * PI / 180.0));
                y += (int)(n * sin(ang * PI / 180.0));
                // 's' is silent, do nothing
                if (c.length() == 1 || c[1] != 's') {
                    drawLine(px, py, x, y, color);
                }
            }
            else if (c == "T" || c == "turn") {
                ang += readNum();
            }
            else if (c == "time") {
                allocTime = readNum();
            }
            else if (c == "=") {
                string name = readChars();
                int num = readNum();
                (*vars)[name] = num;
            }
            else if (c == "neg") {
                string name = readChars();
                (*vars)[name] = -getVar(name);
            }
            else if (c.find_first_of("+-/*") != string::npos) {
                string name = readChars();
                double num = readFloat();
                if (c == "+")
                    (*vars)[name] = getVar(name) + num;
                if (c == "-")
                    (*vars)[name] = getVar(name) - num;
                else if (c == "/")
                    (*vars)[name] = getVar(name) / num; // int division
                else if (c == "*")
                    (*vars)[name] = getVar(name) * num;
            }
            else if (c == "do") {
                int times = readNum();
                LoopEntry l = {times, ip};
                loopStack.push_back(l);
            }
            else if (c == "next") {
                if (loopStack.size() == 0)
                    error("next without do");
                auto d = loopStack.back();
                loopStack.pop_back();
                --d.timesLeft;
                if (d.timesLeft > 0) {
                    loopStack.push_back(d);
                    ip = d.toip;
                }
            }
            else if (c == "color") {
                int r = readNum();
                int g = readNum();
                int b = readNum();
                color = RGB(r, g, b);
            }
            else if (c == "func") {
                string fname = readChars();
                vector<string> args;
                while(true) {
                    string aname = readChars();
                    if (aname.empty())
                        error("func without startf");
                    if (aname == "startf")
                        break;
                    args.push_back(aname);
                }
                auto endip = text.find("endf", ip);
                if (endip == string::npos) 
                    error("no endf for function " + fname);
                Func fe = { args, ip };
                funcs[fname] = fe;
                ip = endip + 4; // skip after endf
            }
            else if (funcs.find(c) != funcs.end()) { // function call
                auto func = funcs[c];
                Frame fr = { vars, 0 };
                auto newvars = new map<string, int>; // don't change vars before arg evaluation
                for(auto aname : func.args) {
                    int v = readNum();
                    (*newvars)[aname] = v;
                }
                vars = newvars;
                fr.returnIp = ip;
                callStack.push_back(fr);
                ip = func.startip;
            }
            else if (c == "endf" || c == "return") {
                if (callStack.empty())
                    error("no call to return from");
                Frame fr = callStack.back();
                callStack.pop_back();
                delete vars;
                vars = fr.returnVars;
                ip = fr.returnIp;
            }
            else if (c == "if") {
                int n1 = readNum();
                string op = readChars();
                int n2 = readNum();
                bool result;

                if (op == "==") result = (n1 == n2);
                else if (op == ">=") result = (n1 >= n2);
                else if (op == "<=") result = (n1 <= n2);
                else if (op == ">") result = (n1 > n2);
                else if (op == "<") result = (n1 < n2);
                else if (op == "!=") result = (n1 != n2);
                else error("Unknown operator " + op);

                // now decide where to go
                if (!result) { // need to find my else or endif
                    int nestlvl = 0;
                    while (true) {
                        string r = readChars();
                        if (r.empty())
                            error("if without endif");
                        if (r == "if")
                            ++nestlvl;
                        else if (r == "else") {
                            if (nestlvl == 0) 
                                break; // found where to go
                        }
                        else if (r == "endif") {
                            if (nestlvl > 0)
                                --nestlvl;
                            else 
                                break; // found where to go
                        }
                    }
                }
            }
            else if (c == "endif") { // do nothing
            }
            else if (c == "else") {
                int nestlvl = 0;
                while (true) { // skip to the end of the endif in my nesting level
                    string r = readChars();
                    if (r.empty())
                        error("else without endif");
                    if (r == "if")
                        ++nestlvl;
                    else if (r == "endif") {
                        if (nestlvl > 0)
                            --nestlvl;
                        else 
                            break;
                    }
                }
            }
            else if (c == "print") {
                int n = readNum();
                stringstream ss;
                ss << n;
                out(ss.str());;
            }
            else {
                error("Unknown command `" + c + "`");
            }
        }
    }
    catch(const exception&)
    {}

    static string prevout;
    if (outText != prevout) {
        SetWindowTextA(g_outwnd, outText.c_str());
        prevout = outText;
    }

}



void __stdcall editHandler(int id, const char* edtext) {
    text = edtext;
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

void drawLine(int x1, int y1, int x2, int y2, unsigned int color) 
{
    float dx = (float)(x2 - x1);
    float dy = (float)(y2 - y1);
    if (abs(dx) > abs(dy)) {
        int minx = min(x1, x2);
        int maxx = max(x1, x2);
        for (int x = minx; x <= maxx; ++x ) {
            int y = y1 + (int)( dy * (float)(x - x1) / dx );
            PUT_PIXEL(x, y, color);
        }
    }
    else {
        int miny = min(y1, y2);
        int maxy = max(y1, y2);
        for (int y = miny; y <= maxy; ++y ) {
            int x = x1 + (int)( dx * (float)(y - y1) / dy );
            PUT_PIXEL(x, y, color);
        }

    }

}

void __stdcall mouseHandler(DWORD msg, DWORD wParam, DWORD lParam) {
    mx = (short)(lParam & 0xffff);
    my = (short)(lParam >> 16);
}

void __stdcall scrollChange(int id, int value) {
    scrollValue[id - 3] = value;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    drd_init(800, 600, INIT_WINDOW);

    drd_createCtrlWindow(300, 700);
    EditCtrl ed = { "EDIT", 1, 10, 10, 280, 500, ES_MULTILINE, WS_EX_CLIENTEDGE, "", editHandler };
    drd_createCtrl(&ed);

    StaticCtrl st = { "STATIC", 2, 10, 515, 280, 80, 0, WS_EX_CLIENTEDGE, "" };
    g_outwnd = drd_createCtrl(&st);

    SliderCtrl sl1 = { "SLIDER", 3, 10, 615, 280, 40,  0, 0, "", scrollChange };
    drd_createCtrl(&sl1);
    SliderCtrl sl2 = { "SLIDER", 4, 10, 665, 280, 40,  0, 0, "", scrollChange };
    drd_createCtrl(&sl2);


//    ButtonCtrl b = { "BUTTON", 2, 700, 520, 70, 30,  BS_PUSHBUTTON, 0, "Do It!", pressHandler };
//    drd_createCtrl(&b);

    drd_setMouseHandler(mouseHandler);


    while (drd_processMessages()) {

        drd_pixelsClear(0);
        drd_pixelsBegin(&pp);

        interpret();

        //drawLine(300, 300, mx, my, );

        drd_pixelsEnd();
        drd_flip();


        Sleep(1);
    }

    return 0;
}


