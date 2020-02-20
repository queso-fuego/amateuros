        ;; call resetTextScreen
	
        ;; Set video mode
        mov ah, 0x00            ; int 0x10/ ah 0x00 = set video mode
        mov al, 0x03            ; 80x25 16 color text mode
        int 0x10

        ;; Change color/ palette
        mov ah, 0x0b
        mov bh, 0x00            ; change bg color
        mov bl, 0x01            ; blue
        int 0x10

        mov si, testMsg
        ;; call print_string
	
	mov ah, 0x0e            ; int 10/ ah 0x0e BIOS teletype output
        mov bh, 0x00            ; page number
        mov bl, 0x07            ; foreground text color if in gfx modes

print_char:
        lodsb                   ; mov byte at si into al, incr/decr si
        cmp al, 0               ; at end of string?
        je end_print            ; end if so 
        int 0x10                ; or print character in al
        jmp print_char          ; loop

end_print:	
        mov ah, 0x00
        int 0x16

        mov ax, 0x2000
        mov es, ax
        xor bx, bx              ; ES:BX <- 0x2000:0x0000

        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax
        jmp 0x2000:0x0000       ; far jump back to kernel

        ;; include "../print/print_string.asm"
        ;; include "../screen/resetTextScreen.asm"

testMsg:        db 'Program Loaded!',0

        times 512-($-$$) db 0   ; pad out to 1 sector


