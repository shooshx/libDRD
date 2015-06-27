.486
.model flat, stdcall
option casemap :none

include \masm32\include\windows.inc
include \masm32\include\kernel32.inc
include \masm32\include\user32.inc

include drd.inc
includelib drd.lib

WIN_WIDTH equ 500
WIN_HEIGHT equ 500

.data                         
    myimg Img {}

    mx DWORD 0
    my DWORD 0
    pressed DWORD 0

    atlasOffset DWORD 0
.code

mouseHandler PROC wmsg:DWORD, wParam:DWORD, lParam:DWORD
    ; extract the mouse coordinates from the mouse message
    mov eax, lParam
    shl eax, 16
    shr eax, 16
    mov mx, eax
    mov eax, lParam
    shr eax, 16
    mov my, eax
    mov eax, wParam
    and eax, MK_LBUTTON
    mov pressed, eax
    ret
mouseHandler ENDP

main PROC
    LOCAL elapsedTime:DWORD
    invoke drd_init, WIN_WIDTH, WIN_HEIGHT, INIT_WINDOW
    invoke drd_setMouseHandler, offset mouseHandler

    invoke drd_imageLoadResource, 101, offset myimg
    invoke drd_imageSetTransparent, offset myimg, 00ff00h

  frameLoop:
    cmp pressed, 0
    jne dontClear
    invoke drd_pixelsClear, 0ffb000h
  dontClear:
    mov eax, mx
    sub eax, 64
    mov ebx, my
    sub ebx, 60
    invoke drd_imageDrawCrop, offset myimg, eax, ebx, atlasOffset, 0, 64, 60
    invoke drd_flip
    invoke Sleep, 10 ; don't over load the CPU

    invoke drd_processMessages
    cmp eax, 0
    je done

    add atlasOffset, 64
    mov eax, myimg.iwidth
    cmp atlasOffset, eax
    jl frameLoop
    mov atlasOffset, 0

    jmp frameLoop

  done:
    invoke ExitProcess,0
    ret
main ENDP

end main