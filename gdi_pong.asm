.486
.model flat, stdcall
option casemap :none

include \masm32\include\windows.inc
include \masm32\include\kernel32.inc
include \masm32\include\user32.inc
include \masm32\include\gdi32.inc

include drd.inc
includelib drd.lib


.data                         ; initialised data section
    align 4
    caption BYTE "PING!",0
    oneLostMsg BYTE "LEFT LOST!",0
    twoLostMsg BYTE "RIGHT LOST!",0
    g_y2 DWORD 100  ; right
    g_y1 DWORD 150  ; left
    g_bx DWORD 320
    g_by DWORD 200
    g_bdx DWORD 5
    g_bdy DWORD 5
    g_isWindow BOOL INIT_WINDOW
.code

handleKey PROTO

main PROC
    invoke drd_init, 640, 480, g_isWindow
    ;invoke drd_setKeyHandler, handleKey

  drawLoop:
    call drd_processMessages 
    test eax, eax
    jz done
    call handleKey
    call moveBall
    ;mov eax, 0
    test eax, eax
    jz noLose
    cmp eax, 1
    je oneLost
    invoke MessageBox, 0, ADDR twoLostMsg, ADDR caption, MB_OK
    jmp yesLose
  oneLost:
    invoke MessageBox, 0, ADDR oneLostMsg, ADDR caption, MB_OK
  yesLose:
    mov g_bx, 320
    mov g_by, 200
  noLose:
    call draw
    invoke Sleep, 20
    call drd_flip
    jmp drawLoop
       
  done:
    invoke ExitProcess,0
main ENDP

isMiss PROC barY:DWORD
    mov ecx, barY
    mov eax, g_by
    cmp eax, ecx
    jl nohit
    add ecx, 100
    cmp eax, ecx
    jg nohit
    mov eax, 0
    ret
  nohit:
    mov eax, 1
    ret
isMiss ENDP

; returns 1 if 1 lost, 2 if 2 lost, 0 if OK
moveBall PROC
    mov eax, g_bdy   ; move in y axis
    add g_by, eax
    cmp g_by, 0      ; reached top ?
    jge checkBottom 
    neg g_bdy
    mov g_by, 0      ; don't want it negative
  checkBottom:
    cmp g_by, 460
    jl movex
    neg g_bdy
    mov g_by, 460
  movex:
    mov eax, g_bdx
    add g_bx, eax
    cmp g_bdx, 0   ; ball going left or right?
    jl checkLeft
    jg checkRight
  checkLeft:
    cmp g_bx, 30
    jg endmove
    invoke isMiss, g_y1
    test eax, eax
    mov eax, 1      ; return left missed
    jnz endhit
    neg g_bdx
    mov g_bx, 30
    jmp endmove
  checkRight:
    cmp g_bx, 590
    jl endmove
    invoke isMiss, g_y2
    test eax, eax
    mov eax, 2      ; return right missed
    jnz endhit
    neg g_bdx
    mov g_bx, 590  
  endmove:
    mov eax, 0
  endhit:
    ret
moveBall ENDP

checkBounds PROC vptr:DWORD
    mov eax, vptr
    mov ecx, [eax]
    cmp ecx, 0
    jge checkLow
    mov dword ptr[eax], 0
  checkLow:
    cmp ecx, 380
    jl endChk
    mov dword ptr[eax], 360
  endChk:
    ret
checkBounds ENDP

changeWindowFull PROC
    cmp g_isWindow, INIT_WINDOW
    je toFull
    mov g_isWindow, INIT_WINDOW
    ret
  toFull:
    mov g_isWindow, INIT_FULLSCREEN
    ret
changeWindowFull ENDP

handleKey PROC
    invoke GetAsyncKeyState, VK_UP
    test eax, 0ff00h
    jz @F
    sub g_y2, 15
  @@:
    invoke GetAsyncKeyState, VK_DOWN
    test eax, 0ff00h
    jz @F
    add g_y2, 15
  @@:
    invoke GetAsyncKeyState, VK_Q
    test eax, 0ff00h
    jz @F
    sub g_y1, 15
  @@:
    invoke GetAsyncKeyState, VK_A
    test eax, 0ff00h
    jz @F
    add g_y1, 15
  @@:
    invoke GetAsyncKeyState, VK_F
    test eax, 0ff00h
    jz @F
    call changeWindowFull
    invoke drd_init, 640, 480, g_isWindow
  @@:
    invoke checkBounds, ADDR g_y1
    invoke checkBounds, ADDR g_y2
 
    mov eax, 1
    ret

handleKey ENDP


draw PROC
    LOCAL hdc:DWORD
    LOCAL hpen:HPEN
    LOCAL polyPoints[SIZEOF(POINT)*4]:POINT
    LOCAL a:DWORD

    invoke drd_pixelsClear, 0
    invoke drd_beginHdc
    mov hdc, eax

    invoke CreatePen, PS_SOLID, 3, 06633ffh
    mov hpen, eax 
    invoke SelectObject, hdc, hpen

    invoke GetStockObject, DC_BRUSH
    invoke SelectObject, hdc, eax

    ; racket 1
    invoke SetDCBrushColor, hdc, 0ff0055h
    mov eax, g_y1
    add eax, 100
    invoke Ellipse, hdc, 10, g_y1, 30, eax

    ; racket 2
    invoke SetDCBrushColor, hdc, 022aa00h
    mov a, 0ddddddddh
    mov eax, g_y2
    mov polyPoints[0].x, 620
    mov polyPoints[0].y, eax
    add eax, 50
    mov polyPoints[8].x, 610
    mov polyPoints[8].y, eax
    add eax, 50
    mov polyPoints[16].x, 620
    mov polyPoints[16].y, eax
    sub eax, 50
    mov polyPoints[24].x, 630
    mov polyPoints[24].y, eax
    lea eax, polyPoints
    invoke Polygon, hdc, eax, 4

    ; ball
    invoke SetDCBrushColor, hdc, 0ffff00h
    mov eax, g_bx
    add eax, 18
    mov ebx, g_by
    add ebx, 18
    invoke Rectangle, hdc, g_bx, g_by, eax, ebx

    invoke drd_endHdc, hdc

    invoke DeleteObject, hpen

    ret
draw ENDP


end main

