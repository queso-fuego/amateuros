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

        ;; Tele-type output
        mov ah, 0x0e            ; int 10/ ah 0x0e BIOS teletype output
        mov bx, testString      ; moving memory address at testString into BX register

        call print_string
        mov bx, string2
        call print_string
        jmp end_pgm

print_string:
        mov al, [bx]            ; move character value at address in BX into al
        cmp al, 0               
        je end_print            ; jump if equal (al = 0) to halt label
        int 0x10                ; print character in al
        add bx, 1               ; move 1 byte forward/ get next character 
        jmp print_string

end_print:
        ret

        ;; Variables
testString:     db 'TEST', 0xA, 0xD, 0    ; 0/null to null terminate
string2:        db 'also a test', 0

end_pgm:
        jmp $                   ; jump repeatedly to here; neverending loop

        ;; Boot sector magic
        times 510-($-$$) db 0   ; pads out 0s until we reach 510th byte

        dw 0xaa55               ; BIOS magic number; BOOT magic #
