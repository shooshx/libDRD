.486
.model flat, stdcall
option casemap :none

include \masm32\include\windows.inc
include \masm32\include\kernel32.inc
include \masm32\include\user32.inc
include \masm32\include\gdi32.inc
include \masm32\include\comdlg32.inc

include \masm32\macros\macros.asm

include \masm32\include\winmm.inc
includelib winmm.lib

include \masm32\include\msvcrt.inc
includelib msvcrt.lib

include drd.inc
includelib drd.lib

WIN_WIDTH equ 210
WIN_HEIGHT equ 160

.data 
    filename BYTE "sound_boing.wav", MAX_PATH dup(0)

    opfFilter BYTE "Wave File (*.wav)",0,"*.wav",0,0
    opf OPENFILENAMEA <>

    mx DWORD 0
    my DWORD 0

    buttonPlay1Rect RECT <10, 10, 100, 100>
    buttonPlay2Rect RECT <10, 110, 100, 200>
    buttonOpenRect RECT <110, 10, 150, 200>

    openFormat BYTE "open %s alias %d",0
    playFormat BYTE "play %d notify",0
    closeFormat BYTE "close %d",0

    soundAlias DWORD 1000
    strBuffer BYTE MAX_PATH+30 dup(0)

    hdc HDC 0
.code

isInsideRect PROC rectPtr:DWORD, x:DWORD, y:DWORD
    mov eax, rectPtr
    mov ebx, x
    cmp ebx, [eax].RECT.top
    jl isNot
    cmp ebx, [eax].RECT.bottom
    jg isNot
    mov ebx, y
    cmp ebx, [eax].RECT.left
    jl isNot
    cmp ebx, [eax].RECT.right
    jg isNot
    mov eax, TRUE
    jmp doneIsInside
  isNot:
    mov eax, FALSE
  doneIsInside:
    ret
isInsideRect ENDP

drawRect PROC rectPtr:DWORD
    mov eax, rectPtr
    invoke RoundRect, hdc, [eax].RECT.top, [eax].RECT.left, [eax].RECT.bottom, [eax].RECT.right, 15, 15
    ret
drawRect ENDP

openFile PROC
    mov filename[0], 0
    invoke GetOpenFileNameA, offset opf
    ret
openFile ENDP

mixPlaySound PROC namePtr:DWORD
    invoke crt_sprintf, offset strBuffer, offset openFormat, namePtr, soundAlias
    invoke mciSendString, offset strBuffer, NULL, 0, NULL
    invoke crt_sprintf, offset strBuffer, offset playFormat, soundAlias
    invoke drd_getMainHwnd
    invoke mciSendString, offset strBuffer, NULL, 0, eax
    inc soundAlias
    ret
mixPlaySound ENDP

; this function is called when a sound started above stops playing.
mciNotifyHandler PROC msg:DWORD, lParam:DWORD, wParam:DWORD
    ; this command is needed so the file will not remain open
    invoke mciSendCommand, wParam, MCI_CLOSE, 0, NULL
    ret
mciNotifyHandler ENDP

mouseHandler PROC wmsg:DWORD, wParam:DWORD, lParam:DWORD
    ; extract the mouse coordinates from the mouse message
    mov eax, lParam
    shl eax, 16
    shr eax, 16
    mov mx, eax
    mov eax, lParam
    shr eax, 16
    mov my, eax

    cmp wmsg, WM_LBUTTONDOWN
    jne doneMouse

    invoke isInsideRect, offset buttonPlay1Rect, mx, my
    cmp eax, FALSE
    je @F
    invoke PlaySound, offset filename, NULL, SND_ASYNC or SND_FILENAME
    jmp doneMouse
  @@:
    invoke isInsideRect, offset buttonPlay2Rect, mx, my
    cmp eax, FALSE
    je @F
    invoke mixPlaySound, offset filename
    jmp doneMouse
  @@:
    invoke isInsideRect, offset buttonOpenRect, mx, my
    cmp eax, FALSE
    je @F
    invoke openFile
  @@:
  doneMouse:
    ret

mouseHandler ENDP


main PROC

    invoke drd_init, WIN_WIDTH, WIN_HEIGHT, INIT_WINDOW
    invoke drd_setMouseHandler, offset mouseHandler
    invoke drd_setWinMsgHandler, MM_MCINOTIFY, offset mciNotifyHandler

    ; init the opf structure for later
    invoke crt_memset, offset opf, 0, SIZEOF(OPENFILENAMEA)
    mov opf.lStructSize, SIZEOF(OPENFILENAMEA)
    invoke drd_getMainHwnd
    mov opf.hwndOwner, eax
    mov opf.lpstrFilter, offset opfFilter
    mov opf.lpstrFile, offset filename
    mov opf.nMaxFile, MAX_PATH
    mov opf.Flags, OFN_FILEMUSTEXIST


  frameLoop:
    ;invoke drd_pixelsClear, 0069ffh
    invoke drd_beginHdc
    mov hdc, eax

    invoke drawRect, offset buttonPlay1Rect
    invoke drawRect, offset buttonPlay2Rect
    invoke drawRect, offset buttonOpenRect

    invoke drd_endHdc, hdc

    invoke drd_flip

    invoke drd_processMessages
    cmp eax, 0
    je done

  skipPlay:

    invoke Sleep, 10 ; don't overload the cpu
    jmp frameLoop

  done:
    invoke ExitProcess, 0  ; required for terminating the winmm thread
    ret
main ENDP
end main
   