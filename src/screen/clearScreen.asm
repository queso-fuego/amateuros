;;; 
;;; clearScreen.asm: clears screen by scrolling (BIOS int 10h AH 06h
;;;
clear_screen:
	pusha
	mov ah, 06h
	mov al, 00h
	
	xor cx, cx		; ch/cl = row/col of upper left corner
	mov dh, 25		; dh = row of lower right corner
	mov dl, 80		; dl = col of lower right corner

	int 10h			; call BIOS video services interrupt
	
	popa
	ret
