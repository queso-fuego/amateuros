;; --------------------------------------------------------------------
;; Print strings in SI register
;; --------------------------------------------------------------------
print_string:
        pusha                   ; store all registers onto the stack
        mov ah, 0x0e            ; int 10/ ah 0x0e BIOS teletype output
        mov bh, 0x00             ; page number
        mov bl, 0x07            ; foreground text color if in gfx modes

print_char:
        lodsb                   ; mov byte at si into al, incr/decr si
        cmp al, 0               ; at end of string?
        je end_print            ; end if so 
        int 0x10                ; or print character in al
        jmp print_char          ; loop

end_print:
        popa                    ; restore all registers from stack
        ret
