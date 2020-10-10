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

	;; Get parm off stack
	mov dx, [bp+4]	; DX = row/col to move cursor to, DL = column value
	mov ax, [bp+6]	; row value
	mov dh, al		

	mov ah, 02h		; int 10h ah 02h = move cursor
	mov bh, 00h
	int 10h
	
	mov word [bp-2], 0	; return code 0 = success!

	mov ax, [bp-2]

	pop dx

	mov sp, bp
	pop bp

	ret
