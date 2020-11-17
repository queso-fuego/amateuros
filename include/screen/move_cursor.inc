;;; 
;;; move_cursor.inc: moves hardware cursor to new row/col
;;; assuming VGA txt mode 03 - 80x25 characters, 16 colors
;;; Parms:
;;; input 1: row to move to
;;; input 2: col to move to
;;; output 1: error/return code in AX
move_cursor:
	push bp
	mov bp, sp	
	sub sp, 2		; Local stack variable - error/return code

	push dx			; save modified register
	push bx

	;; Get parm off stack
	mov dx, [bp+4]	; DX = row/col to move cursor to, DL = column value
	mov ax, [bp+6]	; AL = row value

	;; Calculate cursor bytes
	;; Y pos * size of row + X pos
	xor ah, ah
	imul bx, ax, 80 
	xor dh, dh
	add bx, dx

	;; Move cursor using CRT Microcontroller ports!!
	;; Write the cursor low byte (BL)	
	mov al, 0Fh		; CRT index register, cursor low byte
	mov dx, 03D4h
	out dx, al

	mov dx, 03D5h	; CRT data register
	mov al, bl
	out dx, al

	;; Write the cursor high byte (BH)	
	mov al, 0Eh		; CRT index register, cursor high byte
	mov dx, 03D4h
	out dx, al

	mov dx, 03D5h	; CRT data register
	mov al, bh
	out dx, al

	mov word [bp-2], 0	; return code 0 = success!

	mov ax, [bp-2]

	pop bx
	pop dx

	mov sp, bp
	pop bp

	ret
