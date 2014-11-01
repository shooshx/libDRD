.xmm                     ; create 32 bit code
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

NUM_SIN equ 64
    sinData DWORD 128, 140, 152, 165, 176, 188, 199, 209, 218, 226, 234, 240, 246, 250, 253, 255, 256, 255, 253, 250, 246, 240, 234, 226, 218, 209, 199, 188, 176, 165, 152
    sinData2 DWORD 140, 128, 115, 103, 90, 79, 67, 56, 46, 37, 29, 21, 15, 9, 5, 2, 0, 0, 0, 2, 5, 9, 15, 21, 29, 37, 46, 56, 67, 79, 90, 103, 115

    mx DWORD 0
    my DWORD 0
.code

getColElement PROC xi:DWORD, yi:DWORD, fi:DWORD, s1:DWORD, s2:DWORD
;--------------------------------------------------------------
    LOCAL v:DWORD
    LOCAL hfi:DWORD
    LOCAL rfi:DWORD

    ; setup constants
    mov eax, fi
    mov rfi, eax
    cmp s1, 1
    jne @F
    neg rfi
    add rfi, 20
  @@:
    mov eax, fi
    shr eax, 1
    mov hfi, eax
    cmp s2, 1
    jne @F
    neg hfi
    add hfi, 70
  @@:

    mov edi, 0

    ; add sin(xi/2) to blue
    mov eax, xi
      add eax, rfi
    shr eax, 1  
    and eax, 03Fh
    mov edi, sinData[eax * 4]  ; result accumulated to edi  [0-255]

    ; add sin(yi/2) to blue
    mov eax, yi
      add eax, hfi
    shr eax, 1
    and eax, 03Fh
    add edi, sinData[eax * 4]   ; edi [0-512]

    ; add sin(xi+yi)
    mov eax, xi
    add eax, yi
      sub eax, rfi
    shr eax, 1
    and eax, 03fh
    add edi, sinData[eax * 4]
   
    ; add sin(xi-yi)
    mov eax, xi
    sub eax, yi
      sub eax, hfi
    shr eax, 1
    and eax, 03fh
    add edi, sinData[eax * 4]

    ; ebx = (xi-400)^2 + (yi-300)^2
    mov eax, xi
    sub eax, mx
    sar eax, 2   ; shift with sign, scale frequecy
    mul eax
    mov ebx, eax ; ebx = xi*xi
    mov eax, yi
    sub eax, my
    sar eax, 1   ; scale frequency
    mul eax
    add ebx, eax 
    ; eax = sqrt(ebx)
    mov v, ebx
    fild v
    fsqrt
    fistp v
    mov eax, v
    and eax, 03fh
    add edi, sinData[eax * 4] ; edi [0-255*4]

    shr edi, 1   ; scale back to [0-512]

    mov eax, edi

;============================================================
    ret
getColElement ENDP

getPixelColor PROC xi:DWORD, yi:DWORD, fi:DWORD
;============================================================
    invoke getColElement, xi, yi, fi, 0, 0
    mov ebx, eax
    push ebx
      invoke getColElement, xi, yi, fi, 1, 0
    pop ebx
    shl eax, 8
    or ebx, eax
    push ebx
      invoke getColElement, xi, yi, fi, 0, 1
    pop ebx
    shl eax, 16
    or ebx, eax
    shl eax, 8
    or ebx, eax

    rol eax, 4
    mov eax, ebx

;============================================================
    ret
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
    
    add esi, pp.bytesPerPixel
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

mouseHandler PROC wmsg:DWORD, wParam:DWORD, lParam:DWORD
    ; get the coordinates from lParam
    mov eax, lParam
    and eax, 0ffffh
    mov mx, eax
    mov eax, lParam
    shr eax, 16
    mov my, eax
    ret
mouseHandler ENDP

main PROC
    LOCAL inAnim:BOOL 
    LOCAL fi:DWORD
    mov inAnim, 0
    invoke drd_init, WIN_WIDTH, WIN_HEIGHT, INIT_WINDOW
    invoke drd_setMouseHandler, mouseHandler

    mov fi, 0  ; frame count
  frameLoop:
    invoke drd_processMessages
    cmp eax, 0
    je fin
    invoke drawFrame, fi
    ;invoke drd_printFps

    cmp inAnim, 0
    jne testAnim
    invoke MessageBox, 0, ADDR msg, ADDR caption, MB_YESNO
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