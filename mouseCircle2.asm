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

NUM_COLORS equ 50
    colors DWORD 0ff0000h, 0ff001fh, 0ff003eh, 0ff005dh, 0ff007ch, 0ff009ch, 0ff00bbh, 0ff00dah, 0ff00f9h, 0e400ffh, 0c500ffh, 0a600ffh, 08700ffh, 06800ffh
    colors2 DWORD 04800ffh, 02900ffh, 00a00ffh, 00014ffh, 00034ffh, 00053ffh, 00072ffh, 00091ffh, 000b0ffh, 000d0ffh, 000efffh, 000ffefh, 000ffd0h, 000ffb0h
    colors3 DWORD 000ff91h, 000ff72h, 000ff53h, 000ff34h, 000ff14h, 00aff00h, 029ff00h, 048ff00h, 068ff00h, 087ff00h, 0a6ff00h, 0c5ff00h, 0e4ff00h, 0fff900h, 0ffda00h, 0ffbb00h, 0ff9c00h, 0ff7c00h, 0ff5d00h, 0ff3e00h, 0ff1f00h, 0ff0000h

    mx DWORD 0
    my DWORD 0
.code

getPixelColor PROC xi:DWORD, yi:DWORD, fi:DWORD
;--------------------------------------------------------------
    LOCAL v:DWORD
    mov eax, mx
    sub xi, eax ;400
    mov eax, my
    sub yi, eax ;300
    mov eax, xi
    mov ecx, eax
    mul ecx
    mov ebx, eax
    mov eax, yi
    mov ecx, eax
    mul ecx
    add ebx, eax

    mov v, ebx
    fild v
    fsqrt
    fistp v
    mov eax, v
    sub eax, fi

    mov edx, 0
    mov ebx, NUM_COLORS
    div ebx

    shl edx, 2
    mov eax, colors[edx]

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
    LOCAL fi
    mov inAnim, 0
    invoke drd_init, WIN_WIDTH, WIN_HEIGHT, TRUE
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