;;; -----------------------------------------------------------
;;; Reset a graphics mode screen
;;; -----------------------------------------------------------
resetGraphicsScreen:
        ;; Set video mode
        mov ah, 00h           ; int 10h/ ah 00h = set video mode
        mov al, 13h           ; 320x200 256 color gfx mode
        int 10h

        ret
