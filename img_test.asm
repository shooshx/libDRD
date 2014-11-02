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

    myimg Img {}

    mx DWORD 0
    my DWORD 0
.code



drawFrame PROC fi:DWORD
    invoke drd_imageDraw, ADDR myimg, mx, my
    ;invoke drd_imageDrawCrop, ADDR myimg, mx, my, 20,0,20,100
    invoke drd_flip
    ret
drawFrame ENDP


mouseHandler PROC wmsg:DWORD, wParam:DWORD, lParam:DWORD
    mov eax, lParam
    mov ebx, eax
    and ebx, 0FFFFh
    mov mx, ebx
    shr eax, 16
    mov my, eax
    ret
mouseHandler ENDP

errorHandler PROC emsg:DWORD
    ret
errorHandler ENDP

main PROC
    LOCAL inAnim:BOOL 
    LOCAL fi:DWORD


    mov inAnim, 0
    invoke drd_init, WIN_WIDTH, WIN_HEIGHT, INIT_WINDOW

    invoke drd_imageLoadResource, 101, ADDR myimg
    invoke drd_imageSetTransparent, ADDR myimg, 000ff00h

    ;invoke drd_imageLoadResource, 101, ADDR myimg

    invoke drd_setMouseHandler, mouseHandler
    invoke drd_setErrorHandler, errorHandler

    mov fi, 0  ; frame count
  frameLoop:
    invoke drd_processMessages
    cmp eax, 0
    je fin
    invoke drawFrame, fi

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