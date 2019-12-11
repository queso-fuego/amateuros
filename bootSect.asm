;;;
;;; Simple boot sector that prints characters using BIOS interrupts
;;;
        org 0x7c00              ; 'origin' of Boot code; helps make sure addresses don't change

        ;; Set video mode
        mov ah, 0x00            ; int 0x10/ ah 0x00 = set video mode
        mov al, 0x01            ; 40x25 text mode
        int 0x10

        ;; Change color/ palette
        mov ah, 0x0b
        mov bh, 0x00
        mov bl, 0x01
        int 0x10

        ;; Tele-type output strings
        mov bx, testString      ; moving memory address at testString into BX register

        call print_string
        mov bx, string2
        call print_string

        mov dx, 0x12AB          ; sample hex number to print
        call print_hex

        ;; end_pgm
        jmp $                   ; jump repeatedly to here; neverending loop

       ;; Included files
       include 'print_string.asm'
       include 'print_hex.asm'

        ;; Variables
testString:     db 'Char test: Testing', 0xA, 0xD, 0    ; 0/null to null terminate
string2:        db 'Hex test: ', 0

        ;; Boot sector magic
        times 510-($-$$) db 0   ; pads out 0s until we reach 510th byte

        dw 0xaa55               ; BIOS magic number; BOOT magic #
