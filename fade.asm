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

    mx DWORD 0
    my DWORD 0
.code

getPixelColor PROC xi:DWORD, yi:DWORD, fi:DWORD, prevCol:DWORD
;--------------------------------------------------------------

    mov eax, mx
    sub xi, eax ;400
    mov eax, my
    sub yi, eax ;300

    ; ebx = xi*xi + yi*yi
    mov eax, xi
    mul eax
    mov ebx, eax
    mov eax, yi
    mul eax
    add ebx, eax

    cmp ebx, 2500
    jg doBlack
    mov eax, 0ff0000h
    jmp done
  doBlack:
    mov eax, 0
  fade:
    
    cmp fi, 2
    jl done  ; only start on frame 1
    mov ebx, prevCol 
    shl ebx, 1
    add eax, ebx
  done:  

;============================================================
    ret
getPixelColor ENDP


drawFrame PROC fi:DWORD
    LOCAL pp:PixelPaint

    invoke drd_pixelsBegin, ADDR pp

    mov edi, 0          ; start of line
    mov edx, 0          ; y coord
  lineLoop:
    mov esi, edi        ; current pixel
    mov ecx, 0          ; x coord
  pixelLoop:
    ; save before calling function

    mov ebx, pp.bufPtr
    add ebx, esi
    mov ebx, dword ptr [ebx]

    push edx
    push ecx  
    push esi
    push edi

    invoke getPixelColor, ecx, edx, fi, ebx

    pop edi
    pop esi
    pop ecx
    pop edx

    mov ebx, pp.bufPtr
    add ebx, esi
    mov dword ptr [ebx], eax
    
    add esi, pp.bytesPerPixel
    inc ecx
    cmp ecx, pp.cwidth
    jne pixelLoop

    add edi, pp.pitch
    inc edx
    cmp edx, pp.cheight
    jne lineLoop

    invoke drd_pixelsEnd
    invoke drd_flip
    ret
drawFrame ENDP


mouseHandler PROC wmsg:DWORD, wParam:DWORD, lParam:DWORD
    ; get the coordinates from lParam
    mov eax, lParam
    and eax, 0ffffh
    mov mx, eax
    mov eax, lParam
    shr eax, 16
    mov my, eax
    ret
mouseHandler ENDP


main PROC
    LOCAL inAnim:BOOL 
    LOCAL fi:DWORD
    mov inAnim, 0
    invoke drd_init, WIN_WIDTH, WIN_HEIGHT, INIT_WINDOW
    invoke drd_setMouseHandler, mouseHandler

    mov fi, 0  ; frame count
  frameLoop:
    invoke drd_processMessages
    cmp eax, 0
    je fin
    invoke drawFrame, fi

    cmp inAnim, 0
    jne testAnim
    invoke MessageBox, 0, ADDR msg, ADDR caption, MB_YESNO ; MB_OK
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