;;; -----------------------------------------------------------
;;; Reset a graphics mode screen
;;; -----------------------------------------------------------
resetGraphicsScreen:
        ;; Set video mode
        mov ah, 0x00            ; int 0x10/ ah 0x00 = set video mode
        mov al, 0x13            ; 320x200 256 color gfx mode
        int 0x10

        ret
