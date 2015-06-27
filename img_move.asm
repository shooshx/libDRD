.486
.model flat, stdcall
option casemap :none

include \masm32\include\windows.inc
include \masm32\include\kernel32.inc
include \masm32\include\user32.inc

include drd.inc
includelib drd.lib

WIN_WIDTH equ 800
WIN_HEIGHT equ 600

.data                         
    myimg Img {}
    filename BYTE "rainbow_dash.bmp",0
    wndTitle BYTE "MLP Rulz",0

    mx DWORD 0
    my DWORD 0
    pressed DWORD 0

.code

mouseHandler PROC wmsg:DWORD, wParam:DWORD, lParam:DWORD
    ; extract the mouse coordinates from the mouse message in lParam
    mov eax, lParam
    shl eax, 16
    shr eax, 16
    mov mx, eax
    mov eax, lParam
    shr eax, 16
    mov my, eax
    ; buttons info in wParam
    mov eax, wParam
    and eax, MK_LBUTTON
    mov pressed, eax
    ret
mouseHandler ENDP

keyHandler PROC vkey:DWORD
    cmp vkey, VK_UP
    jne @F
    sub my, 5
  @@:
    cmp vkey, VK_DOWN
    jne @F
    add my, 5
  @@:
    cmp vkey, VK_LEFT
    jne @F
    sub mx, 5
  @@:
    cmp vkey, VK_RIGHT
    jne @F
    add mx, 5
  @@:
    ret
keyHandler ENDP


main PROC
    invoke drd_init, WIN_WIDTH, WIN_HEIGHT, INIT_WINDOW
    invoke drd_setMouseHandler, offset mouseHandler
    invoke drd_setKeyHandler, offset keyHandler

    invoke drd_imageLoadFile, offset filename, offset myimg
    invoke drd_imageSetTransparent, offset myimg, 00ff00h
    invoke drd_setWindowTitle, offset wndTitle

  frameLoop:
    cmp pressed, 0
    jne dontClear
    invoke drd_pixelsClear, 0069ffh
  dontClear:
    ; position the image center on the mouse pointer
    mov eax, myimg.iwidth
    shr eax, 1
    neg eax
    add eax, mx
    mov ebx, myimg.iheight
    shr ebx, 1
    neg ebx
    add ebx, my
    invoke drd_imageDraw, offset myimg, eax, ebx
    invoke drd_flip

    invoke drd_processMessages
    cmp eax, 0
    je done

    jmp frameLoop

  done:
    invoke ExitProcess,0
    ret
main ENDP

end main