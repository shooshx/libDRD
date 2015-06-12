.486
.model flat, stdcall
option casemap :none

include \masm32\include\windows.inc
include \masm32\include\kernel32.inc
include \masm32\include\user32.inc
include \masm32\include\gdi32.inc

include \masm32\include\msvcrt.inc
includelib msvcrt.lib

include drd.inc
includelib drd.lib

WIN_WIDTH equ 800
WIN_HEIGHT equ 600

.data                      
    instruction BYTE "Please write something...",10,13,0
    cmdExit BYTE "exit",0
    nameArial BYTE "Arial",0
    buffer BYTE "Hello",100 dup(0)

    textY DWORD 0
    textRotation DWORD 0
    bkcol DWORD 0

.code

inputThreadProc PROC
    invoke crt_printf, offset instruction
  inputAgain:
    invoke crt_gets, offset buffer
    invoke crt_strcmp, offset buffer, offset cmdExit
    cmp eax, 0
    je inputDone
    jmp inputAgain
  inputDone:
    invoke ExitProcess, 0
    ret
inputThreadProc ENDP


main PROC
    LOCAL hdc:HDC
    LOCAL hfont:HFONT

    invoke drd_init, WIN_WIDTH, WIN_HEIGHT, INIT_WINDOW

    ; start a second thread which does the input from the console
    invoke CreateThread, NULL, 0, offset inputThreadProc, 0, 0, NULL

    ; the main thread does the graphics
  frameLoop:
    invoke drd_processMessages
    cmp eax, 0
    je fin

    invoke drd_beginHdc
    mov hdc, eax

    invoke CreateFont, 40, 20, textRotation, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, offset nameArial
    mov hfont, eax
    invoke SelectObject, hdc, hfont

    invoke SetTextColor, hdc, 0ff4400h
    mov eax, bkcol
    and eax, 0ffh
    shl eax, 8
    add eax, 0000ffh 
    invoke SetBkColor, hdc, eax

    invoke crt_strlen, offset buffer
    invoke TextOutA, hdc, WIN_WIDTH / 2, textY, offset buffer, eax

    invoke drd_endHdc, hdc
    invoke drd_flip

    invoke DeleteObject, hfont
    invoke Sleep, 10

    add textRotation, 10
    inc bkcol

    inc textY
    cmp textY, WIN_HEIGHT + 50
    jl frameLoop
    mov textY, 0

    jmp frameLoop

  fin:
    ret
main ENDP

end main