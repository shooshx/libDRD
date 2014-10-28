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

getPixelColor PROC xi:DWORD, yi:DWORD, fi:DWORD
;--------------------------------------------------------------
    mov eax, mx
    sub xi, eax ;400
    mov eax, my
    sub yi, eax ;300
    mov eax, xi
    mov ecx, eax
    mul ecx
    mov ebx, eax
    mov eax, yi
    mov ecx, eax
    mul ecx
    add ebx, eax

    mov eax, ebx
    mov edx, 0
    mov ebx, 50
    div ebx
    shl eax, 16
    or eax, 0ffh

;============================================================
    ret
getPixelColor ENDP


drawFrame PROC fi:DWORD
    LOCAL pp:PixelPaint
    invoke drd_pixelsBegin, ADDR pp

    mov edi, pp.bufPtr  ; start of line
    mov edx, 0          ; y coord
  lineLoop:
    mov esi, edi        ; current pixel
    mov ecx, 0          ; x coord
  pixelLoop:
    ; save before calling function
    push edx
    push ecx  
    push esi
    push edi

    invoke getPixelColor, ecx, edx, fi

    pop edi
    pop esi
    pop ecx
    pop edx
    mov dword ptr [esi], eax
    
    add esi, pp.bytesPerPixel
    inc ecx
    cmp ecx, WIN_WIDTH
    jne pixelLoop

    add edi, pp.pitch
    inc edx
    cmp edx, WIN_HEIGHT
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
    LOCAL fi
    mov inAnim, 0
    invoke drd_init, WIN_WIDTH, WIN_HEIGHT, TRUE
    invoke drd_setMouseHandler, mouseHandler

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