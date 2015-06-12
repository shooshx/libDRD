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
    rx DWORD 0
    ry DWORD 0
    rwidth DWORD 0
    rheight DWORD 0
    rcolor DWORD 0
    
    pp PixelPaint <>

    text BYTE "a",0
    caption BYTE "b",0
    
    img Img <>
    imgx DWORD 0
    imgy DWORD 0
    imgdx DWORD 1
    imgdy DWORD 1
.code

; choose a random value in [from : from+range]
randVar PROC from:DWORD, range:DWORD
    invoke mrand ; result in eax
    mov edx, 0
    div range
    mov eax, edx
    add eax, from
    ret
randVar ENDP

; selects random coordinates and color for the rect, puts them in the global variables
randRect PROC
    invoke randVar, 0, pp.cwidth
    mov rx, eax
    mov ebx, pp.cwidth
    sub ebx, eax
    invoke randVar, 1, ebx  ; width is in [0 : WIN_WIDTH - x]
    mov rwidth, eax

    invoke randVar, 0, pp.cheight
    mov ry, eax
    mov ebx, pp.cheight
    sub ebx, eax
    invoke randVar, 1, ebx  ; height is in [0: WIN_HEIGHT - y]
    mov rheight, eax

    invoke randVar, 55, 200  ; red
    mov ebx, eax
    invoke randVar, 55, 200  ; green
    shl eax, 8
    or ebx, eax
    invoke randVar, 55, 200  ; blue
    shl eax, 16
    or ebx, eax
    mov rcolor, ebx
    ret
randRect ENDP

; x,y should be inside the window, wdth, heigt should not be 0 and should not create a rect bigger than the window
drawRect PROC x:DWORD, y:DWORD, wdth:DWORD, height:DWORD, color:DWORD
    LOCAL lineStart:DWORD 
    ; initialize to the start  offset
    mov lineStart, 0
    mov eax, pp.bufPtr
    mov lineStart, eax
    mov eax, x
    mul pp.bytesPerPixel
    add lineStart, eax
    mov eax, y
    mul pp.pitch
    add lineStart, eax
    mov eax, lineStart   ; eax is the addres where we write the color
    mov ecx, wdth        ; width of the current line
  ploop:
    mov edx, color
    mov [eax], edx
    add eax, pp.bytesPerPixel      ; next pixel
    dec ecx
    jnz ploop

    mov eax, pp.pitch ; move to next line
    add lineStart, eax
    mov eax, lineStart
    dec height
    mov ecx, wdth     ; restart x width counter
    jnz ploop
    ret
drawRect ENDP

moveImg PROC
    cmp imgx, 0
    jg @F
    mov imgdx, 1
  @@:
    mov ebx, pp.cwidth
    sub ebx, img.iwidth
    cmp imgx, ebx
    jl @F
    mov imgdx, -1
  @@:
    cmp imgy, 0
    jg @F
    mov imgdy, 1
  @@:
    mov ebx, pp.cheight
    sub ebx, img.iheight
    cmp imgy, ebx
    jl @F
    mov imgdy, -1
  @@:
    
    mov eax, imgx
    add eax, imgdx
    mov imgx, eax
    mov eax, imgy
    add eax, imgdy
    mov imgy, eax
    ret
moveImg ENDP

drawFrame PROC
    invoke drd_pixelsBegin, ADDR pp

    invoke randRect
    invoke moveImg

    invoke drawRect, rx, ry, rwidth, rheight, rcolor
    invoke drd_pixelsEnd
    invoke drd_imageDraw, addr img, imgx, imgy

    invoke drd_flip

    ret
drawFrame ENDP


main PROC
    invoke drd_init, WIN_WIDTH, WIN_HEIGHT, INIT_WINDOW

    invoke drd_windowSetTranslucent, 255

    invoke drd_imageLoadResource, 101, ADDR img
    invoke drd_imageSetTransparent, ADDR img, 00ff00h



  frameLoop:
    invoke drd_processMessages
    cmp eax, 0
    je fin
    invoke drawFrame

    invoke Sleep, 1

    jmp frameLoop

  fin:
    invoke ExitProcess,0
    ret
main ENDP

end main