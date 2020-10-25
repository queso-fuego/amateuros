;;;
;;; print_registers.asm: prints registers and memory addresses to screen
;;;
;;;	input 1: cursor Y position (address, not value)
;;; input 2: cursor X position (address, not value)
print_registers:
	push bp
	mov bp, sp

	mov ax, [bp+6]
	mov word [csr_y_addr], ax
	mov ax, [bp+4]
	mov word [csr_x_addr], ax

    mov si, printRegHeading 
	;; Print string
	push si
	push word [csr_y_addr]
	push word [csr_x_addr]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov bx, [csr_y_addr]
	push word [bx]			; Cursor Y value
	mov bx, [csr_x_addr]
	push word [bx]			; Cursor X value
	call move_cursor
	add sp, 4

	;; Print string for DX
    mov si, regString
	push si
	push word [csr_y_addr]
	push word [csr_x_addr]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov bx, [csr_y_addr]
	push word [bx]			; Cursor Y value
	mov bx, [csr_x_addr]
	push word [bx]			; Cursor X value
	call move_cursor
	add sp, 4

	;; Print hex value
	push word [csr_y_addr]
	push word [csr_x_addr]
    call print_hex          ; print DX
	add sp, 4

	;; Print string for AX
    mov byte [regString+2], 'a'
	push si
	push word [csr_y_addr]
	push word [csr_x_addr]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov bx, [csr_y_addr]
	push word [bx]			; Cursor Y value
	mov bx, [csr_x_addr]
	push word [bx]			; Cursor X value
	call move_cursor
	add sp, 4

	;; Print hex value
    mov dx, ax
	push word [csr_y_addr]
	push word [csr_x_addr]
    call print_hex          ; print AX value
	add sp, 4

	;; Print string for BX
    mov byte [regString+2], 'b'
	push si
	push word [csr_y_addr]
	push word [csr_x_addr]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov bx, [csr_y_addr]
	push word [bx]			; Cursor Y value
	mov bx, [csr_x_addr]
	push word [bx]			; Cursor X value
	call move_cursor
	add sp, 4

	;; Print hex value
    mov dx, bx
	push word [csr_y_addr]
	push word [csr_x_addr]
    call print_hex          ; print BX value
	add sp, 4

	;; Print string
    mov byte [regString+2], 'c'
	push si
	push word [csr_y_addr]
	push word [csr_x_addr]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov bx, [csr_y_addr]
	push word [bx]			; Cursor Y value
	mov bx, [csr_x_addr]
	push word [bx]			; Cursor X value
	call move_cursor
	add sp, 4

	;; Print hex value
    mov dx, cx
	push word [csr_y_addr]
	push word [csr_x_addr]
    call print_hex          ; print CX value
	add sp, 4

	;; Print string
    mov word [regString+2], 'si'
	push si
	push word [csr_y_addr]
	push word [csr_x_addr]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov bx, [csr_y_addr]
	push word [bx]			; Cursor Y value
	mov bx, [csr_x_addr]
	push word [bx]			; Cursor X value
	call move_cursor
	add sp, 4

	;; Print hex value
    mov dx, si
	push word [csr_y_addr]
	push word [csr_x_addr]
    call print_hex          ; print SI value
	add sp, 4

	;; Print string
    mov byte [regString+2], 'd'
	push si
	push word [csr_y_addr]
	push word [csr_x_addr]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov bx, [csr_y_addr]
	push word [bx]			; Cursor Y value
	mov bx, [csr_x_addr]
	push word [bx]			; Cursor X value
	call move_cursor
	add sp, 4

	;; Print hex value
    mov dx, di
	push word [csr_y_addr]
	push word [csr_x_addr]
    call print_hex          ; print DI value
	add sp, 4

	;; Print string
    mov word [regString+2], 'cs'
	push si
	push word [csr_y_addr]
	push word [csr_x_addr]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov bx, [csr_y_addr]
	push word [bx]			; Cursor Y value
	mov bx, [csr_x_addr]
	push word [bx]			; Cursor X value
	call move_cursor
	add sp, 4

	;; Print hex value
    mov dx, cs
	push word [csr_y_addr]
	push word [csr_x_addr]
    call print_hex          ; print CS value
	add sp, 4

	;; Print string
    mov byte [regString+2], 'd'
	push si
	push word [csr_y_addr]
	push word [csr_x_addr]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov bx, [csr_y_addr]
	push word [bx]			; Cursor Y value
	mov bx, [csr_x_addr]
	push word [bx]			; Cursor X value
	call move_cursor
	add sp, 4

	;; Print hex value
    mov dx, ds
	push word [csr_y_addr]
	push word [csr_x_addr]
    call print_hex          ; print DS value
	add sp, 4

	;; Print string
    mov byte [regString+2], 'e'
	push si
	push word [csr_y_addr]
	push word [csr_x_addr]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov bx, [csr_y_addr]
	push word [bx]			; Cursor Y value
	mov bx, [csr_x_addr]
	push word [bx]			; Cursor X value
	call move_cursor
	add sp, 4

	;; Print hex value
    mov dx, es
	push word [csr_y_addr]
	push word [csr_x_addr]
    call print_hex          ; print ES value
	add sp, 4

	;; Print string
	mov byte [regString+2], 's'
	push si
	push word [csr_y_addr]
	push word [csr_x_addr]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov bx, [csr_y_addr]
	push word [bx]			; Cursor Y value
	mov bx, [csr_x_addr]
	push word [bx]			; Cursor X value
	call move_cursor
	add sp, 4

	;; Print hex value
    mov dx, ss
	push word [csr_y_addr]
	push word [csr_x_addr]
    call print_hex          ; print SS value
	add sp, 4

	;; Print char
	push word 000Ah 
	push word [csr_y_addr]
	push word [csr_x_addr]
	call print_char_text_mode
	add sp, 6

	;; Move cursor
	mov bx, [csr_y_addr]
	push word [bx]			; Cursor Y value
	mov bx, [csr_x_addr]
	push word [bx]			; Cursor X value
	call move_cursor
	add sp, 4

	;; Print char
	push word 000Dh 
	push word [csr_y_addr]
	push word [csr_x_addr]
	call print_char_text_mode
	add sp, 6

	;; Move cursor
	mov bx, [csr_y_addr]
	push word [bx]			; Cursor Y value
	mov bx, [csr_x_addr]
	push word [bx]			; Cursor X value
	call move_cursor
	add sp, 4

	mov sp, bp
	pop bp

    ret

;; Variables
nl equ 0Ah,0Dh ; CR/LF

printRegHeading:    db nl,'--------  ------------',nl,\
        'Register  Mem Location',nl,\
        '--------  ------------',0

regString:  db 0Ah,0Dh,'dx        ',0 ; hold string of current register name and memory address
csr_x_addr: dw 0
csr_y_addr: dw 0
