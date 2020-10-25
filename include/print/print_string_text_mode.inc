;;;
;;; print_string_text_mode: Print a string to video memory
;;; Parms:
;;;  input 1: address of string
;;;  input 2: row to print to	 (address, not value)
;;;  input 3: column to print to (address, not value)
;;;  output 1: return code in AX
;;;----------------------------------------------
print_string_text_mode:
	push bp
	mov bp, sp
	sub sp, 2		; error/return code

	push es
	push di
	push si
	push bx
	push cx
	push dx

	mov ax, 0B800h	; set up ES:DI to print to vga color text mode memory
	mov es, ax

	mov bx, [bp+6]			; Cursor Y address
	imul di, [bx], 160
	mov bx, [bp+4]			; Cursor X address
	imul ax, [bx], 2		; Col to print to start printing at, in bytes
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

		inc word [bx]		; Update cursor position
		cmp word [bx], 80	; at end of line?
		jne .loop								; No, go on

		mov dx, word [bx]	; Yes, do a CR 1st, then LF
		shl dx, 1
		sub di, dx
		mov word [bx], 0

		.LF:
			mov bx, [bp+6]						; Cursor Y address
			inc word [bx]		; Go down 1 row
			cmp word [bx], 25	; At bottom of screen?
			jge .scroll_down

			add di, 160								; Go down "1 row" in video memory
			mov bx, [bp+4]						; Cursor X address
			jmp .loop
		.CR:
			mov dx, word [bx]
			shl dx, 1
			sub di, dx
			mov word [bx], 0
			jmp .loop

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
			
			mov ax, 1720h	; blue bg, light gray fg, space
			mov cx, 80
			rep stosw		; Clear line 24
			
			pop si
			pop ds
			pop di
			pop es

			dec word [bx]	; set Y = line 24
			mov bx, [bp+4]						; Cursor X address
	jmp .loop

	.end_print:
	mov word [bp-2], 0		; Return code 0 = success
	mov ax, [bp-2]

	pop dx
	pop cx
	pop bx
	pop si
	pop di
	pop es

	mov sp, bp
	pop bp

	ret
