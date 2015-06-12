.486
.model flat, stdcall
option casemap :none

include \masm32\include\windows.inc
include \masm32\include\kernel32.inc
include \masm32\include\user32.inc

include drd.inc
includelib drd.lib

WIN_WIDTH equ 800
WIN_HEIGHT equ 600

.data                         
    caption BYTE "Draw",0
    msg BYTE "Done",0
    wndTitle BYTE "Infinite Color!",0

; generated using makeGradient.py
NUM_COLORS_MASK equ 3fh  ; 64 colors
    colors  DWORD 0ff0000h, 0ff0018h, 0ff0030h, 0ff0048h, 0ff0061h, 0ff0079h, 0ff0091h, 0ff00aah, 0ff00c2h, 0ff00dah, 0ff00f2h, 0f200ffh, 0da00ffh, 0c200ffh, 0aa00ffh, 09100ffh
    colros2 DWORD 07900ffh, 06100ffh, 04800ffh, 03000ffh, 01800ffh, 00000ffh, 00018ffh, 00030ffh, 00048ffh, 00061ffh, 00079ffh, 00091ffh, 000a9ffh, 000c2ffh, 000daffh, 000f2ffh
    colors3 DWORD 000fff2h, 000ffdah, 000ffc2h, 000ffa9h, 000ff91h, 000ff79h, 000ff61h, 000ff48h, 000ff30h, 000ff18h, 000ff00h, 018ff00h, 030ff00h, 048ff00h, 061ff00h, 079ff00h 
    colors4 DWORD 091ff00h, 0a9ff00h, 0c2ff00h, 0daff00h, 0f2ff00h, 0fff200h, 0ffda00h, 0ffc200h, 0ffaa00h, 0ff9100h, 0ff7900h, 0ff6100h, 0ff4800h, 0ff3000h, 0ff1800h, 0ff0000h    

    mx DWORD 0
    my DWORD 0
.code

getPixelColor PROC xi:DWORD, yi:DWORD, fi:DWORD
;--------------------------------------------------------------
    LOCAL v:DWORD
    ; ebx = (xi-mx)^2 + (yi-my)^2
    mov eax, mx
    sub xi, eax ;400
    mov eax, my
    sub yi, eax ;300
    mov eax, xi
    mul eax
    mov ebx, eax
    mov eax, yi
    mul eax
    add ebx, eax

    ; eax = sqrt(ebx)
    mov v, ebx
    fild v
    fsqrt
    fistp v
    mov eax, v

    sub eax, fi
    and eax, NUM_COLORS_MASK  ; reminder from division by 64

    mov eax, colors[eax * 4]

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

sampleMouse PROC
    LOCAL pnt:POINT
    invoke GetCursorPos, addr pnt
    mov eax, pnt.x
    mov mx, eax
    mov eax, pnt.y
    mov my, eax
    ret
sampleMouse ENDP

main PROC
    LOCAL inAnim:BOOL 
    LOCAL fi:DWORD
    mov inAnim, 0
    invoke drd_init, WIN_WIDTH, WIN_HEIGHT, INIT_WINDOWFULL or INIT_INPUT_FALLTHROUGH
    invoke drd_windowSetTranslucent, 70
    invoke drd_setWindowTitle, offset wndTitle

    mov fi, 0  ; frame count
  frameLoop:
    invoke drd_processMessages
    cmp eax, 0
    je fin
    ; we need to sample the mouse and not rely on the handler since the input messages fall through
    invoke sampleMouse 
    invoke drawFrame, fi
    ;invoke drd_printFps

    cmp inAnim, 0
    jne testAnim
    ;invoke MessageBox, 0, ADDR msg, ADDR caption, MB_YESNO
    mov eax, IDYES
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