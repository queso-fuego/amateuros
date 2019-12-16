;;;
;;; print_registers.asm: prints registers and memory addresses to screen
;;;
print_registers:
        mov si, regString
        call print_string
        call print_hex          ; print DX

        mov byte [regString+2], 'a'
        call print_string
        mov dx, ax
        call print_hex          ; print AX

        mov byte [regString+2], 'b'
        call print_string
        mov dx, bx
        call print_hex          ; print BX

        mov byte [regString+2], 'c'
        call print_string
        mov dx, cx
        call print_hex          ; print CX

        mov word [regString+2], 'si'
        call print_string
        mov dx, si
        call print_hex          ; print SI

        mov byte [regString+2], 'd'
        call print_string
        mov dx, di
        call print_hex          ; print DI

        mov word [regString+2], 'cs'
        call print_string
        mov dx, cs
        call print_hex          ; print CS

        mov byte [regString+2], 'd'
        call print_string
        mov dx, ds
        call print_hex          ; print DS

        mov byte [regString+2], 'e'
        call print_string
        mov dx, es
        call print_hex          ; print ES

        ret

        ;; Variables
regString:  db 0xA,0xD,'dx        ',0 ; hold string of current register name and memory address
