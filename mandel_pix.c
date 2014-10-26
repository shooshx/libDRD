
typedef unsigned int uint;

uint ccolors[] = {0x0000ff, 0x00ff00, 0xff0000 };

uint __stdcall XgetPixelColor(uint yi, uint xi)
{
    float x0 = ((float)xi / 200.0f) - 2.5f;
    float y0 = ((float)yi / 200.0f) - 1.5f;
    float x = 0.0f;
    float y = 0.0f;
    float sum = 0, xtemp = 0;

    uint iter = 0;
    do {
        xtemp = x*x - y*y + x0;
        y = 2*x*y + y0;
        x = xtemp;
        ++iter;
        sum = x*x + y*y;
    } while (iter < 100 && sum < 4.0f);
    
    if (iter == 100)
        return 0;
    return ccolors[iter % 3];

}