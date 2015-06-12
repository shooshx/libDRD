.XMM                     ; create 32 bit code
.model flat, stdcall     ; 32 bit memory model
option casemap :none     ; case sensitive

include \masm32\include\windows.inc
include \masm32\include\kernel32.inc
include \masm32\include\user32.inc
include \masm32\macros\macros.asm

include drd.inc
includelib drd.lib

WIN_WIDTH equ 800
WIN_HEIGHT equ 600

.data                         
    caption BYTE "Draw",0
    msg BYTE "Done",0

    zero REAL4 0.0
    twoHundred REAL4 200.0
    two REAL4 2.0
    four REAL4 4.0

NUM_COLORS_MASK equ 0fh ; 16 colors, red yellow
    colors DWORD 0ffff00h, 0ffee00h, 0ffdd00h, 0ffcc00h, 0ffbb00h, 0ffaa00h, 0ff9900h, 0ff8800h, 0ff7700h, 0ff6600h, 0ff5500h, 0ff4400h, 0ff3300h, 0ff2200h, 0ff1100h, 0ff0000h
;NUM_COLORS_MASK equ 1fh ; 32 colors, rainbow
;    colors DWORD 0ff0000h, 0ff0031h, 0ff0062h, 0ff0094h, 0ff00c5h, 0ff00f6h, 0d500ffh, 0a400ffh, 07300ffh, 04100ffh, 01000ffh, 00020ffh, 00052ffh, 00083ffh, 000b4ffh, 000e6ffh, 000ffe6h, 000ffb4h, 000ff83h, 000ff52h, 000ff20h, 010ff00h, 041ff00h, 073ff00h, 0a4ff00h, 0d5ff00h, 0fff600h, 0ffc500h, 0ff9400h, 0ff6200h, 0ff3100h, 0ff0000h
    twoPfive REAL4 2.5
    onePfive REAL4 1.5
.code

getPixelColor PROC xi:DWORD, yi:DWORD, fi:DWORD
;--------------------------------------------------------------
; algorithm from http://en.wikipedia.org/wiki/Mandelbrot_set
; SSE inscturction http://softpixel.com/~cwright/programming/simd/sse.php
    cvtsi2ss xmm0, xi  ; x0
    divss xmm0, twoHundred
    subss xmm0, twoPfive
    cvtsi2ss xmm1, yi  ; y0
    divss xmm1, twoHundred
    subss xmm1, onePfive
    movss xmm2, zero   ; x
    movss xmm3, zero   ; y

    mov eax, 0  ; iter count
  iter:
    movss xmm5, xmm2       
    mulss xmm5, xmm5   ; xmm5 = x*x
    movss xmm6, xmm3
    mulss xmm6, xmm6   ; xmm5 = y*y

    mulss xmm3, xmm2   ; y = y*x
    mulss xmm3, two    ; y = y*x*2
    addss xmm3, xmm1   ; y = y*x*2 + y0

    movss xmm2, xmm0   ; x = x0
    addss xmm2, xmm5   ; x = x0 + x*x
    subss xmm2, xmm6   ; x = x0 + x*x - y*y

    inc eax
    cmp eax, 100
    je overIter
    addss xmm5, xmm6  ; xmm5 = x*x+y*y
    comiss xmm5, four
    jbe iter
    
    ; over 4
    add eax, fi
    and eax, NUM_COLORS_MASK  ; take reminder from dividing by this number
    mov eax, colors[eax * 4] ; each number is 4 bytes
    jmp done
  overIter:
    mov eax, 0
  done:
    ret
;============================================================
getPixelColor ENDP


drawFrame PROC fi:DWORD
    LOCAL pp:PixelPaint
    invoke drd_pixelsBegin, ADDR pp

    mov edi, pp.bufPtr  ; start of line
    mov edx, 0          ; y coord
  lineLoop:
    mov esi, edi        ; current pixel
    mov ecx, 0          ; x coord
  pixelLoop:
    ; save before calling function
    push edx
    push ecx  
    push esi
    push edi

    invoke getPixelColor, ecx, edx, fi

    pop edi
    pop esi
    pop ecx
    pop edx
    mov dword ptr [esi], eax
    
    add esi, 4
    inc ecx
    cmp ecx, pp.cwidth
    jne pixelLoop

    add edi, pp.pitch
    inc edx
    cmp edx, pp.cheight
    jne lineLoop

    invoke drd_pixelsEnd
    invoke drd_flip
    ret
drawFrame ENDP

main PROC
    LOCAL inAnim:BOOL 
    LOCAL fi:DWORD
    mov inAnim, 0
    invoke drd_init, WIN_WIDTH, WIN_HEIGHT, INIT_WINDOW

    mov fi, 0  ; frame count
  frameLoop:
    invoke drd_processMessages
    cmp eax, 0
    je fin
    invoke drawFrame, fi

    cmp inAnim, 0
    jne testAnim
    invoke MessageBox, 0, ADDR msg, ADDR caption, MB_YESNO; MB_OK
    mov inAnim, eax
  testAnim:
    inc fi
    cmp inAnim, IDYES
    je frameLoop

  fin:
    invoke ExitProcess,0
    ret
main ENDP

end main