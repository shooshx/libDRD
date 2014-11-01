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

.code

getPixelColor PROC xi:DWORD, yi:DWORD, fi:DWORD
;--------------------------------------------------------------
    mov eax, xi
    add eax, yi

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

main PROC
    LOCAL inAnim:BOOL 
    LOCAL fi:DWORD

    mov inAnim, 0
    invoke drd_init, WIN_WIDTH, WIN_HEIGHT, INIT_WINDOW

    mov fi, 0  ; frame count
  frameLoop:
    invoke drd_processMessages
    cmp eax, 0
    je fin
    invoke drawFrame, fi

    cmp inAnim, 0
    jne testAnim
    invoke MessageBox, 0, ADDR msg, ADDR caption, MB_OK
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