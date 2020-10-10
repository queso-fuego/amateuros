;;;
;;; print_string_text_mode: Print a string to video memory
;;; Parms:
;;;  input 1: address of string
;;;  input 2: row to print to	 (value, not address)
;;;  input 3: column to print to (value, not address)
;;;  output 1: return code in AX
;;;  output 2: new cursor row value in BX
;;;	 output 3: new cursor col value in CX 
;;;----------------------------------------------
print_string_text_mode:
	push bp
	mov bp, sp
	sub sp, 2		; error/return code

	push es
	push di
	push si

	mov ax, 0B800h	; set up ES:DI to print to vga color text mode memory
	mov es, ax

	mov ax, [bp+6]	; save cursor values
	mov word [print_string_cursor_y], ax	
	mov ax, [bp+4]
	mov word [print_string_cursor_x], ax	

	imul di, [bp+6], 160	; Row to print to start printing at, in bytes
	imul ax, [bp+4], 2		; Col to print to start printing at, in bytes
	add di, ax				; DI = row*sizeof(row) + col*sizeof(col)

	mov si, [bp+8]			; Start of string to print (in SI)
	mov ah,	17h				; Colors to print (VGA 17h = blue bg, light grey fg)

	.loop:
		lodsb				; AL <- [SI] & increment SI
		cmp al, 0
		je .end_print
		cmp al, 0Ah			; Line feed?
		je .LF
		cmp al, 0Dh			; Carriage return?
		je .CR
		stosw				; Print character to string by storing AL in [ES:DI]
		inc word [print_string_cursor_x]		; Update cursor position
		cmp word [print_string_cursor_x], 80	; at end of line?
		jne .loop								; No, go on
		inc word [print_string_cursor_y]		; Yes, move cursor for CRLF
		add di, 160
		mov bx, word [print_string_cursor_x]
		shl bx, 1
		sub di, bx
		mov word [print_string_cursor_x], 0
		jmp .loop

		.LF:
			inc word [print_string_cursor_y]	; Go down 1 row
			add di, 160							; Go down "1 row" in video memory
			jmp .loop
		.CR:
			mov bx, word [print_string_cursor_x]
			shl bx, 1
			sub di, bx							; Move to "beginning of line" in vid mem
			mov word [print_string_cursor_x], 0	; Move to beginning of line
	jmp .loop

	.end_print:
	mov word [bp-2], 0		; Return code 0 = success
	mov ax, [bp-2]
	mov bx, word [print_string_cursor_y]	; new cursor row value
	mov cx, word [print_string_cursor_x]	; new cursor col value

	pop si
	pop di
	pop es

	mov sp, bp
	pop bp

	ret

;; VARIABLES
print_string_cursor_x: dw 0
print_string_cursor_y: dw 0
