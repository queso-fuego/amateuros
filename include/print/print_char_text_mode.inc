;;;
;;; print_char_text_mode: Print a single character to video memory
;;; Parms:
;;;  input 1: char to print
;;;  input 2: row to print to	 (value, not address)
;;;  input 3: column to print to (value, not address)
;;;  output 1: return code in AX
;;;  output 2: new cursor row value in BX
;;;	 output 3: new cursor col value in CX 
;;;----------------------------------------------
print_char_text_mode:
	push bp
	mov bp, sp
	sub sp, 2		; error/return code

	push es
	push di

	mov ax, 0B800h	; set up ES:DI to print to
	mov es, ax

	mov ax, [bp+6]	; save cursor values
	mov word [print_char_cursor_y], ax	
	mov ax, [bp+4]
	mov word [print_char_cursor_x], ax	

	imul di, [bp+6], 160	; Row to print to, in bytes
	imul ax, [bp+4], 2		; Col to print to, in bytes
	add di, ax				; DI = row*sizeof(row) + col*sizeof(col)

	mov ax, [bp+8]			; Char to print (in AL)

	mov ah,	17h				; Colors to print

	cmp al, 0Ah			; Line feed?
	je .LF
	cmp al, 0Dh			; Carriage return?
	je .CR
	stosw				; Print character to string by storing AL in [ES:DI]
	inc word [print_char_cursor_x]		; Update cursor position
	cmp word [print_char_cursor_x], 80	; at end of line?
	jne .end_print						; No, go on

	mov bx, word [print_char_cursor_x]	; Yes, do a CR 1st, then LF
	shl bx, 1
	sub di, bx	
	mov word [print_char_cursor_x], 0
	
	.LF:
		inc word [print_char_cursor_y]		; Go down 1 row
		cmp word [print_char_cursor_y], 25	; At bottom of screen?
		jge .scroll_down

		add di, 160							; Go down "1 row" in video memory
		jmp .end_print
	.CR:
		mov bx, word [print_char_cursor_x]
		shl bx, 1
		sub di, bx							; Move to "beginning of line" in video memory
		mov word [print_char_cursor_x], 0	; Move to beginning of line
		jmp .end_print

	.scroll_down:
		;; Copy screen lines 1-24 into lines 0-23 (0-based),
		;;   then clear out last line, line 24
		;;   and continue printing
		push es
		push di
		push ds
		push si
		
		mov ax, 0B800h
		mov es, ax
		mov ds, ax
		xor di, di
		mov si, 160		; Byte location of screen line 1
		mov cx, 80*24
		rep movsw		; Copy lines 1-24 into lines 0-23
		
		mov ax, 1720h
		mov cx, 80
		rep stosw		; Clear line 24
		
		pop si
		pop ds
		pop di
		pop es

		dec word [print_char_cursor_y]	; set Y = line 24

	.end_print:
	mov word [bp-2], 0		; Return code 0 = success
	mov ax, [bp-2]

	mov bx, word [print_char_cursor_y]	; new cursor row value
	mov cx, word [print_char_cursor_x]	; new cursor col value

	pop di
	pop es

	mov sp, bp
	pop bp

	ret

;; VARIABLES
print_char_cursor_x: dw 0
print_char_cursor_y: dw 0

