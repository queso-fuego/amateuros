;;; 
;;; clear_screen_text_mode.inc: clears screen by writing to video memory 
;;; TODO: Change from clear_screen_text_mode to just 'clear_screen'
use32
clear_screen_text_mode:
	push eax
	push ecx
	push edi

    ;; TODO: Change all the magic numbers dude, you ain't a magician
	mov edi, [5028h]        ; Framebuffer location in VBE mode info block
    mov ecx, 1920*1080      ; # of pixels to write 
    mov eax, 000000FFh      ; ARGB pixel color value - blue
	rep	stosd

    ;; TODO: change to not halt here and change move_cursor file
    hlt

	;; Move hardware cursor after
	push dword 0
	push dword 0
	call move_cursor
	add esp, 8

	pop edi
	pop ecx
	pop eax

	ret
