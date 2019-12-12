;;; 
;;; Kernel.asm: basic 'kernel' loaded from our bootsector
;;;

      ;; Set video mode
        mov ah, 0x00            ; int 0x10/ ah 0x00 = set video mode
        mov al, 0x01            ; 40x25 text mode
        int 0x10

        ;; Change color/ palette
        mov ah, 0x0b
        mov bh, 0x00
        mov bl, 0x01
        int 0x10

        mov si, testString     
        call print_string

        hlt                     ; halt cpu

print_string:
        mov ah, 0x0e            ; int 10/ ah 0x0e BIOS teletype output
        mov bh, 0x0             ; page number
        mov bl, 0x07            ; color

print_char:
        mov al, [si]
        cmp al, 0
        je end_print            ; jump if equal (al = 0) to halt label
        int 0x10                ; print character in al
        add si, 1               ; move 1 byte forward/ get next character
        jmp print_char          ; loop

end_print:
        ret

testString:     db 'Kernel Booted, Welcome to QuesOS!', 0xA, 0xD, 0

        ;; Sector Padding magic
        times 512-($-$$) db 0   ; pads out 0s until we reach 512th byte

