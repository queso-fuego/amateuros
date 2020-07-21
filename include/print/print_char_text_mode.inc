;;;
;;; print_char_text_mode: Print a single character to video memory
;;; Parms:
;;;  input 1: char to print
;;;  input 2: row to print to
;;;  input 3: column to print to
;;;  output 1: return code in AX
;;;----------------------------------------------
print_char_text_mode:
	push bp
	mov bp, sp
	sub sp, 2		; error/return code

	push es
	push di

	mov ax, 0B800h	; set up ES:DI to print to
	mov es, ax

	imul di, [bp+6], 160	; Row to print to, in bytes
	imul ax, [bp+4], 2		; Col to print to, in bytes
	add di, ax				; DI = row*sizeof(row) + col*sizeof(col)

	mov ax, [bp+8]			; Char to print (in AL)

	mov ah,	17h				; Colors to print
	stosw 	

	mov word [bp-2], 0		; Return code 0 = success
	mov ax, [bp-2]

	pop di
	pop es

	mov sp, bp
	pop bp

	ret
