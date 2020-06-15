;; --------------------------------------------------------------------
;; Print strings in SI register
;; --------------------------------------------------------------------
print_string:
        pusha                  ; store all registers onto the stack
        mov ah, 0Eh            ; int 10/ ah 0Eh BIOS teletype output
        mov bh, 00h            ; page number
        mov bl, 07h            ; foreground text color if in gfx modes

print_char:
        lodsb                  ; mov byte at si into al, incr/decr si
        cmp al, 0              ; at end of string?
        je end_print           ; end if so 
        int 10h                ; or print character in al
        jmp print_char         ; loop

end_print:
        popa                   ; restore all registers from stack
        ret
