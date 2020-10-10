;;;
;;; print_registers.asm: prints registers and memory addresses to screen
;;;
;;;	input 1: cursor Y position
;;; input 2: cursor X position
;;; output 1: new cursor Y position in BX
;;; output 2: new cursor X position in CX
print_registers:
	push bp
	mov bp, sp

	mov ax, [bp+6]
	mov word [print_registers_cursor_y], ax
	mov ax, [bp+4]
	mov word [print_registers_cursor_x], ax

    mov si, regString
	;; Print string
	push si
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx
	push bx
	push cx
	call move_cursor
	add sp, 4

	;; Print hex value
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
    call print_hex          ; print DX
	add sp, 4

	;; Update cursor position
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx

    mov byte [regString+2], 'a'
	;; Print string
	push si
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx
	push bx
	push cx
	call move_cursor
	add sp, 4

    mov dx, ax
	;; Print hex value
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
    call print_hex          ; print AX
	add sp, 4

	;; Update cursor position
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx

    mov byte [regString+2], 'b'
	;; Print string
	push si
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx
	push bx
	push cx
	call move_cursor
	add sp, 4

    mov dx, bx
	;; Print hex value
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
    call print_hex          ; print BX
	add sp, 4

	;; Update cursor position
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx

    mov byte [regString+2], 'c'
	;; Print string
	push si
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx
	push bx
	push cx
	call move_cursor
	add sp, 4

    mov dx, cx
	;; Print hex value
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
    call print_hex          ; print CX
	add sp, 4

	;; Update cursor position
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx

    mov word [regString+2], 'si'
	;; Print string
	push si
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx
	push bx
	push cx
	call move_cursor
	add sp, 4

    mov dx, si
	;; Print hex value
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
    call print_hex          ; print SI
	add sp, 4

	;; Update cursor position
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx

    mov byte [regString+2], 'd'
	;; Print string
	push si
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx
	push bx
	push cx
	call move_cursor
	add sp, 4

    mov dx, di
	;; Print hex value
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
    call print_hex          ; print DI
	add sp, 4

	;; Update cursor position
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx

    mov word [regString+2], 'cs'
	;; Print string
	push si
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx
	push bx
	push cx
	call move_cursor
	add sp, 4

    mov dx, cs
	;; Print hex value
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
    call print_hex          ; print CS
	add sp, 4

	;; Update cursor position
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx

    mov byte [regString+2], 'd'
	;; Print string
	push si
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx
	push bx
	push cx
	call move_cursor
	add sp, 4

    mov dx, ds
	;; Print hex value
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
    call print_hex          ; print DS
	add sp, 4

	;; Update cursor position
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx

    mov byte [regString+2], 'e'
	;; Print string
	push si
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx
	push bx
	push cx
	call move_cursor
	add sp, 4

    mov dx, es
	;; Print hex value
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
    call print_hex          ; print ES
	add sp, 4

	;; Update cursor position
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx

	mov byte [regString+2], 's'
	;; Print string
	push si
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx
	push bx
	push cx
	call move_cursor
	add sp, 4

    mov dx, ss
	;; Print hex value
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
    call print_hex          ; print SS
	add sp, 4

	;; Update cursor position
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx

	xor ah, ah
	mov al, 0Ah
	;; Print char
	push ax 
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
	call print_char_text_mode
	add sp, 6

	;; Move cursor
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
	call move_cursor
	add sp, 4

	xor ah, ah
	mov al, 0Dh
	;; Print char
	push ax 
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
	call print_char_text_mode
	add sp, 6

	;; Move cursor
	mov word [print_registers_cursor_y], bx
	mov word [print_registers_cursor_x], cx
	push word [print_registers_cursor_y]
	push word [print_registers_cursor_x]
	call move_cursor
	add sp, 4

	mov sp, bp
	pop bp

    ret

        ;; Variables
regString:  db 0Ah,0Dh,'dx        ',0 ; hold string of current register name and memory address
print_registers_cursor_y: dw 0
print_registers_cursor_x: dw 0
