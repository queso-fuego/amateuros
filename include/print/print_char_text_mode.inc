;;;
;;; print_char_text_mode: Print a single character to video memory
;;; Parms:
;;;  input 1: char to print
;;;  input 2: row to print to	 (address, not value)
;;;  input 3: column to print to (address, not value)
;;;  output 1: return code in AX
;;;----------------------------------------------
print_char_text_mode:
	push bp
	mov bp, sp
	sub sp, 2		; error/return code

	push es
	push di
	push bx
	push dx

	mov ax, 0B800h	; set up ES:DI to print to
	mov es, ax

	mov bx, [bp+6]			; Cursor Y address
	imul di, [bx], 160		; Row to print to, in bytes
	mov bx, [bp+4]			; Cursor X address
	imul ax, [bx], 2		; Col to print to, in bytes
	add di, ax				; DI = row*sizeof(row) + col*sizeof(col)

	mov ax, [bp+8]			; Char to print (in AL)

	mov ah,	17h				; Colors to print

	cmp al, 0Ah			; Line feed?
	je .LF
	cmp al, 0Dh			; Carriage return?
	je .CR
	stosw				; Print character to string by storing AL in [ES:DI]
	inc word [bx]		; Update cursor position
	cmp word [bx], 80	; at end of line?
	jne .end_print						; No, go on

	mov dx, word [bx]	; Yes, do a CR 1st, then LF
	shl dx, 1
	sub di, dx	
	mov word [bx], 0
	
	.LF:
		mov bx, [bp+6]						; Cursor Y address
		inc word [bx]		; Go down 1 row
		cmp word [bx], 25	; At bottom of screen?
		jge .scroll_down

		add di, 160							; Go down "1 row" in video memory
		mov bx, [bp+4]						; Cursor X address
		jmp .end_print

	.CR:
		mov dx, word [bx]
		shl dx, 1
		sub di, dx					; Move to "beginning of line" in video memory
		mov word [bx], 0			; Move to beginning of line
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
		
		mov ax, 1720h	; blue bg, light gray fg, space
		mov cx, 80		; # of chars in line
		rep stosw		; Clear line 24
		
		pop si
		pop ds
		pop di
		pop es

		dec word [bx]	; set Y = line 24
		mov bx, [bp+4]	; Cursor X address

	.end_print:
	mov word [bp-2], 0		; Return code 0 = success
	mov ax, [bp-2]

	pop dx
	pop bx
	pop di
	pop es

	mov sp, bp
	pop bp

	ret
