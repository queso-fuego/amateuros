;;; 
;;; clear_screen_text_mode.inc: clears screen by writing to video memory 
;;; assuming VGA txt mode 03 - 80x25 characters, 16 colors
;;;
clear_screen_text_mode:
	push es
	push ax
	push cx
	push di

	mov ax, 0B800h	; set up video memory
	mov es, ax
	xor di, di		; es:di <- B8000:0000

	mov ah, 17h		; blue background, light gray foreground
	mov al, ' '		; space
	mov cx, 80*25	; Number of characters to write
	
	rep	stosw

	;; Move hardware cursor after
	push word 0
	push word 0
	call move_cursor
	add sp, 4

	pop di
	pop cx
	pop ax
	pop es
	ret
