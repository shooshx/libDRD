.486
.model flat, stdcall
option casemap :none

include \masm32\include\windows.inc
include \masm32\include\kernel32.inc
include \masm32\include\user32.inc
include \masm32\include\gdi32.inc

include drd.inc
includelib drd.lib

START_WIN_WIDTH equ 800
START_WIN_HEIGHT equ 600

.data                         
    prevx DWORD 0
    prevy DWORD 0
    winWidth DWORD START_WIN_WIDTH
    winHeight DWORD START_WIN_HEIGHT
.code

; choose a random value in [from : from+range]
randVar PROC from:DWORD, range:DWORD
    invoke mrand ; result in eax
    mov edx, 0
    div range
    mov eax, edx
    add eax, from
    ret
randVar ENDP

randColor PROC
    invoke randVar, 55, 200  ; red
    mov ebx, eax
    invoke randVar, 55, 200  ; green
    shl eax, 8
    or ebx, eax
    invoke randVar, 55, 200  ; blue
    shl eax, 16
    or ebx, eax
    mov eax, ebx
    ret
randColor ENDP


drawFrame PROC
    LOCAL hdc:HDC
    LOCAL x:DWORD, y:DWORD
    LOCAL hpen:HPEN

    invoke drd_beginHdc
    mov hdc, eax

    invoke randColor
    invoke CreatePen, PS_SOLID, 2, eax
    mov hpen, eax
    invoke SelectObject, hdc, hpen

    invoke MoveToEx, hdc, prevx, prevy, NULL

    invoke randVar, 0, winWidth
    mov x, eax
    mov prevx, eax
    invoke randVar, 0, winHeight
    mov y, eax
    mov prevy, eax

    invoke LineTo, hdc, x, y
    invoke DeleteObject, hpen


    invoke drd_endHdc, hdc

    invoke drd_flip

    ret
drawFrame ENDP

resizeHandler PROC w:DWORD, h:DWORD
    mov eax, w
    mov winWidth, eax
    mov eax, h
    mov winHeight, eax
    ret
resizeHandler ENDP



main PROC
    invoke drd_setResizeHandler, offset resizeHandler     ; init will call the resize handler
    invoke drd_init, START_WIN_WIDTH, START_WIN_HEIGHT, INIT_WINDOW OR INIT_RESIZABLE; ; can be changed to INIT_WINDOWFULL OR INIT_INPUT_FALLTHROUGH; INIT_FULLSCREEN 
    ;invoke drd_windowSetTranslucent, 70

  frameLoop:
    invoke drd_processMessages
    cmp eax, 0
    je fin

    invoke drawFrame

    jmp frameLoop

  fin:
    invoke ExitProcess,0
    ret
main ENDP

end main