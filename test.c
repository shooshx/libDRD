
typedef unsigned int uint;

double sin(double);

// nice sinus coloring
uint __stdcall getPixelColor(uint xi, uint yi, uint fi)
{
    double fx = (double)(xi + fi) / 50.0;
    double sx = (sin(fx) + 1) * 128.0;

    double fy = (double)(yi - fi) / 50.0;
    double sy = (sin(fy) + 1) * 128.0;

    double fp = (double)(yi + xi + fi*10) / 50.0;
    double sp = (sin(fp) + 1) * 128.0;


    return (int)sx + ((int)sy << 8) + ((int)sp << 16);

}
