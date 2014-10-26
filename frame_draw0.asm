.486                     ; create 32 bit code
.model flat, stdcall     ; 32 bit memory model
option casemap :none     ; case sensitive

include \masm32\include\windows.inc
include \masm32\include\kernel32.inc
include \masm32\include\user32.inc
include \masm32\macros\macros.asm

include drd.inc

.data                         
    caption BYTE "Draw",0
    msg BYTE "Done",0
.code

getPixelColor PROC x:DWORD, y:DWORD
    mov eax, x
    xor eax, y
    shl eax, 7

    ret
getPixelColor ENDP



drawFrame PROC
    LOCAL pp:PixelPaint
    invoke drd_pixelsBegin, ADDR pp

    mov edi, pp.bufPtr  ; start of line
    mov edx, 0          ; y coord
  startLine:
    mov esi, edi        ; current pixel
    mov ecx, 0          ; x coord
  pixel:
    push ecx
    push edx
    call getPixelColor
    mov dword ptr [esi], eax
    
    add esi, 4
    inc ecx
    cmp ecx, 800
    jne pixel

    add edi, pp.pitch
    inc edx
    cmp edx, 600
    jne startLine

    invoke drd_pixelsEnd
    invoke drd_flip
    ret
drawFrame ENDP

main PROC
    invoke drd_init, 800, 600, TRUE

    call drawFrame

    invoke MessageBox, 0, ADDR msg, ADDR caption, MB_OK
    invoke ExitProcess,0

main ENDP

end main