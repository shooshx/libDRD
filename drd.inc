
; initialize a window or full screen display
; width & height in pixels
; can be called multiple times to change view
drd_init PROTO STDCALL wdth:DWORD, heght:DWORD, flags:DWORD

; - flags needs to be one of the following:
; normal window with a frame
INIT_WINDOW equ 0
; sets the screen resolution and take over the screen
; notice: in this mode, you must redraw the entire screen in each frame from scratch (since there are two separate buffers)
INIT_FULLSCREEN equ 1
; windown which covers the entive screen, don't take over the screen, width, height are ignored
INIT_WINDOWFULL equ 2
; - and can have any of the following ored
; set the created window as top most and allow mouse events to fall through it to other applications
INIT_INPUT_FALLTHROUGH equ 4

; used for drawing pixles using direct memory access
PixelPaint STRUCT
    bufPtr DWORD ?   ; address for the start of the buffer
    pitch  DWORD ?   ; how much to add to bufPtr to get to the next scan line
    bytesPerPixel DWORD ?  ; number of bytes per pixel the draw surface has (depends on windows display settings, can be 2 or 4)
    cheight DWORD ? 
    cwidth DWORD ?
PixelPaint ENDS

; lock the back surface for painting
; @param pp address to a PixelPaint instance
drd_pixelsBegin PROTO pp:DWORD 

; must be called at the end of painting the frame, before drd_flip()
drd_pixelsEnd PROTO

; fill the back buffer in black
drd_pixelsClear PROTO color:DWORD

; flip display and back buffers
drd_flip PROTO

; setup a keyboard handler, called when a keyboard key is pressed down while in drd_processMessages
; func should be a pointer to a function which takes 1 DWORD which is the vkey code of the key
drd_setKeyHandler PROTO func:DWORD

; setup a mouse message handler. 
; func should be a pointer to a function which takes 3 DWORD: msg, wParam, lParam
drd_setMouseHandler PROTO func:DWORD

; setup an error handler
; func should be a pointer to a function which takes 1 DWORD which is a pointer to a string describing the error
; if no handler is defined, an error messagebox appears 
drd_setErrorHandler PROTO func:DWORD

; processes window messages from the user, return FALSE if Escape key was clicked
; this function should be called in a loop
drd_processMessages PROTO

; set window to be translucent, alpha between 0 to 255
drd_windowSetTranslucent PROTO alpha:BYTE


; holds information about a loaded image
Img STRUCT
    surface DWORD ?
    iwidth DWORD ?
    iheight DWORD ?
    hasSrcKey DWORD ?
Img ENDS

; load an image from file `filename` into an Img struct
drd_imageLoadFile PROTO filename:DWORD, pimg:DWORD

; load an image from a resource compiled into the exe
drd_imageLoadResource PROTO id:DWORD, pimg:DWORD


; draw the image on the back buffer, starting at coord (dstX,dstY). must be outside drd_pixelsBegin
drd_imageDraw PROTO pimg:DWORD, dstX:DWORD, dstY:DWORD
; draw the image on the back buffer, take from the image the square [srcX, srcY, srcX+srcWidth, srcY+srcHeight] and put it at coord (dstX,dstY). must be outside drd_pixelsBegin
drd_imageDrawCrop PROTO pimg:DWORD, dstX:DWORD, dstY:DWORD, srcX:DWORD, srcY:DWORD, srcWidth:DWORD, srcHeight:DWORD

; set the color which will be interpreted as transparency
drd_imageSetTransparent PROTO pimg:DWORD, color:DWORD

; destroy the Img object
drd_imageDelete PROTO pimg:DWORD

; each second, print the frame-per-second count
; append text to filename. if filename is NULL, the default file is c:/temp/drd_fps.txt
drd_printFps PROTO filename:DWORD


;------- crt replacement functions -----
mrand PROTO
msrange PROTO seed:DWORD