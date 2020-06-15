        call clear_screen_text_mode
	
        mov si, testMsg
        ;; call print_string
	
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
        mov ah, 00h
        int 16h

        mov ax, 2000h
        mov es, ax
        xor bx, bx             ; ES:BX <- 2000h:0000h

        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        jmp 2000h:0000h        ; far jump back to kernel

        include "../screen/clear_screen_text_mode.asm"

testMsg:        db 'Program Loaded!',0

        times 512-($-$$) db 0   ; pad out to 1 sector


