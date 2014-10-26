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
;NUM_COLORS equ 6
    ;colors DWORD 0000ffh, 00ff00h, 0ff0000h, 00ffffh, 0ffff00h, 0ff00ffh
NUM_COLORS equ 10
    colors DWORD 0ffff00h, 0ffe200h, 0ffc600h, 0ffaa00h, 0ff8d00h, 0ff7100h, 0ff5500h, 0ff3800h, 0ff1c00h, 0ff0000h
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
    mov ebx, NUM_COLORS
    mov edx, 0
    div ebx
    shl edx, 2 ; each number is 4 bytes
    mov eax, colors[edx]
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
    cmp ecx, WIN_WIDTH
    jne pixelLoop

    add edi, pp.pitch
    inc edx
    cmp edx, WIN_HEIGHT
    jne lineLoop

    invoke drd_pixelsEnd
    invoke drd_flip
    ret
drawFrame ENDP

main PROC
    LOCAL inAnim:BOOL 
    LOCAL fi
    mov inAnim, 0
    invoke drd_init, WIN_WIDTH, WIN_HEIGHT, TRUE

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