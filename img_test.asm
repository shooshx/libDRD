.xmm
.model flat, stdcall
option casemap :none

include \masm32\include\windows.inc
include \masm32\include\kernel32.inc
include \masm32\include\user32.inc
include \masm32\macros\macros.asm

include drd.inc
includelib drd.lib

WIN_WIDTH equ 800
WIN_HEIGHT equ 600

.data                         
    myimg Img {}
    filename BYTE "C:\\projects\\Gvahim\\DirectDrawTest\\rainbow_dash.bmp",0

    mx DWORD 0
    my DWORD 0
.code


mouseHandler PROC wmsg:DWORD, wParam:DWORD, lParam:DWORD
    mov eax, lParam
    shl eax, 16
    shr eax, 16
    mov mx, eax
    mov eax, lParam
    shr eax, 16
    mov my, eax
    ret
mouseHandler ENDP


main PROC
    invoke drd_init, WIN_WIDTH, WIN_HEIGHT, INIT_WINDOW
    invoke drd_imageLoadFile, offset filename, offset myimg
    invoke drd_setMouseHandler, offset mouseHandler

  frameLoop:

    ;invoke drd_pixelsClear, 0069ffh
    ;invoke drd_imageDraw, offset myimg, mx, my
    ;invoke drd_flip

    invoke drd_processMessages
    cmp eax, 0
    je done

    jmp frameLoop

  done:
    invoke ExitProcess,0
    ret
main ENDP

end main