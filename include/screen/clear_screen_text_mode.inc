;;; 
;;; clear_screen_text_mode.inc: clears screen by writing to video memory 
;;; assuming VGA txt mode 03 - 80x25 characters, 16 colors
;;;
use32
clear_screen_text_mode:
	push eax
	push ecx
	push edi

	mov edi, 0B8000h

    mov ax, 1720h   ; blue bg, light gray fg, space
	mov cx, 80*25	; Number of characters to write
	
	rep	stosw

	;; Move hardware cursor after
	push dword 0
	push dword 0
	call move_cursor
	add esp, 8

	pop edi
	pop ecx
	pop eax

	ret
