
; initialize a window or full screen display
; width & height in pixels
; can be called multiple times to change view
drd_init PROTO STDCALL wdth:DWORD, heght:DWORD, isWindow:BOOL

; used for drawing pixles using direct memory access
PixelPaint STRUCT
    bufPtr DWORD ?   ; address for the start of the buffer
    pitch  DWORD ?   ; how much to add to bufPtr to get to the next scan line
    bytesPerPixel DWORD ?  ; number of bytes per pixel the draw surface has (depends on windows display settings, can be 2 or 4)
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


; holds information about a loaded image
Img STRUCT
    surface DWORD ?
    iwidth DWORD ?
    iheight DWORD ?
Img ENDS

; load an image from file `filename` into an Img struct
drd_imageLoadFile PROTO filename:DWORD, pimg:DWORD

; load an image from a resource compiled into the exe
drd_imageLoadResource PROTO id:DWORD, pimg:DWORD


; draw the image on the back buffer. must be outside drd_pixelsBegin
drd_imageDraw PROTO pimd:DWORD, x:DWORD, y:DWORD

; set the color which will be interpreted as transparency
drd_imageSetTransparent PROTO pimg:DWORD, color:DWORD

; destroy the Img object
drd_imageDelete PROTO pimg:DWORD