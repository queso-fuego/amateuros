;;; 
;;; clear_screen_text_mode.inc: clears screen by writing to video memory 
;;; assuming VGA txt mode 03 - 80x25 characters, 16 colors
;;;
clear_screen_text_mode:
	pusha

	mov ax, 0B800h	; set up video memory
	mov es, ax
	xor di, di		; es:di <- B8000:0000

	mov ah, 17h		; blue background, light gray foreground
	mov al, ' '		; space
	mov cx, 80*25	; Number of characters to write
	
	rep	stosw

	;; Move hardware cursor after
	mov ah, 02h		; int 10h ah 02h = move hardware cursor
	xor bh, bh		; page #
	mov dh, 2
	mov dl, 2
	xor dx, dx		; dh = row to move to, dl = col
	int 10h


	popa
	ret
