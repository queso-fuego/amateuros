;;;
;;; test.asm: testing file
;;;
	;; print out a 'T' to screen
	mov ah, 0x0e
	mov al, 'T'
	int 0x10
	cli
	hlt
