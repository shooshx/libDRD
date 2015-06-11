.xmm
.model flat, stdcall
option casemap :none

include \masm32\include\windows.inc
include \masm32\include\kernel32.inc
include \masm32\include\user32.inc
include \masm32\macros\macros.asm
include \masm32\include\msvcrt.inc
includelib msvcrt.lib

include drd.inc
includelib drd.lib

WIN_WIDTH equ 800
WIN_HEIGHT equ 600

.data                         
    myimg Img {}
    filename BYTE "C:\\projects\\Gvahim\\DirectDrawTest\\rainbow_dash.bmp",0


    mx DWORD 0
    my DWORD 0

    caption BYTE "caption",0
    word1 BYTE "aaaa", 0
    word2 BYTE "bbb", 0
    word3 BYTE "cccc", 0

    wordArray DWORD offset word1, offset word2, offset word3

    board DWORD 1000000 dup(1)

.code

get2d PROC x2d:DWORD, y2d:DWORD ;return index at edx
    mov ebx,8
    mov eax,x2d
    mul ebx
    add eax,y2d
    mov edx,eax
ret
get2d ENDP


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
    ; initialization - do only one time
    invoke GetTickCount
    invoke crt_srand, eax

    ; generate the number
    invoke crt_rand
    mov edx, 0  ; prepare edx for div
    mov ebx, 3
    div ebx
    ; result now is in edx
    shl edx, 2 ; multiply by 4
    mov ebx,  wordArray[edx]
    invoke MessageBox, 0, ebx, offset caption, MB_OK


    ret


    invoke drd_init, WIN_WIDTH, WIN_HEIGHT, INIT_WINDOW
    invoke drd_imageLoadFile, offset filename, offset myimg
    invoke drd_setMouseHandler, offset mouseHandler

    mov eax, WS_CHILD OR WS_VISIBLE

  frameLoop:

    invoke GetAsyncKeyState, VK_UP
    and eax, 8000h
    cmp eax, 0
    je upNotPressed
    invoke MessageBoxA, 0, offset filename, offset filename, MB_OK
  upNotPressed:

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