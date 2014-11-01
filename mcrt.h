
// implementation of some CRT functions I need so drd would not depend in the CRT dlls.

void mZeroMemory(void* dst, size_t sz) {
    __asm { // doing this in assembly prevents the optimized from replacing it with CRT memset
        mov ecx, sz
        mov edi, dst
        mov eax, 0
        cld
        rep stosb
    }
//    for(size_t i = 0; i < sz; ++i) {
//       *(((char*)dst) + i) = 0;
//   }
}

size_t mstrlen_s(const char* s, size_t mx) {
    size_t c = 0;
    while (s[c] != 0 && c < mx)
        ++c;
    return c;
}

// TBD check
void mstrcat_s(char* dst, size_t dstsz, const char* src) {
    size_t di = 0;
    while (dst[di] != 0 && di < dstsz)
        ++di;
    if (di == dstsz)
        return;
    int si = 0;
    while (src[si] != 0 && di < dstsz - 1)
        dst[di++] = src[si++];
    dst[di] = 0;
}


 void mitoa(int n, char s[], int base)
 {
    if (base > 16)
        return;
    static const char digits[] = "0123456789ABCDEF";
    int sign = 0;
    unsigned int un = (unsigned int)n;
    if (base <= 10) {
        sign = n; // record sign
        if (n < 0)  
            un = -n;
    }
    int i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = digits[un % base];   /* get next digit */
    } while ((un /= base) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';

    // reverse
    for (int ii = 0, jj = i - 1; ii < jj; ++ii, --jj) {
        char c = s[ii];
        s[ii] = s[jj];
        s[jj] = c;
    }
 }

unsigned int g_randState = 0;
void msrand(unsigned int s) {
    g_randState = s;
}
int mrand() {
    return (((g_randState * 214013L + 2531011L) >> 16) & 0x7fff);
}