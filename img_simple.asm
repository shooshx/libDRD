.486
.model flat, stdcall
option casemap :none

include drd.inc
includelib drd.lib

.data                         
    myimg Img {}
    filename BYTE "rainbow_dash.bmp",0
.code
main PROC
    invoke drd_init, 800, 600, INIT_WINDOW
    invoke drd_imageLoadFile, offset filename, offset myimg
  frameLoop:
    invoke drd_imageDraw, offset myimg, 100, 100
    invoke drd_flip
    invoke drd_processMessages
    cmp eax, 0
    je done
    jmp frameLoop
  done:
    ret
main ENDP
end main
