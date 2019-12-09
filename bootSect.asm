;;; Basic Boot sector that will jump continuously
;;;

here:
        jmp here                ; jump repeatedly to label "loop"; neverending

        times 510-($-$$) db 0   ; pads out 0s until we reach 510th byte

        dw 0xaa55               ; BIOS magic number; BOOT magic #
