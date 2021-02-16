;;; 
;;; clear_screen_text_mode.inc: clears screen by writing to video memory 
use32
clear_screen:
	push eax
	push ecx
	push edi

    ;; TODO: Change all the magic numbers dude, you ain't a magician
	mov edi, [5028h]        ; Framebuffer location in VBE mode info block
    mov ecx, 1920*1080      ; # of pixels to write 
    mov eax, 000000FFh      ; ARGB pixel color value - blue
	rep	stosd

	pop edi
	pop ecx
	pop eax

	ret
