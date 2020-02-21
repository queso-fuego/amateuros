;;;
;;; test.asm: testing file
;;;
	;; Set video mode
        mov ah, 0x00            ; int 0x10/ ah 0x00 = set video mode
        mov al, 0x03            ; 80x25 16 color text mode
        int 0x10

        ;; Change color/ palette
        mov ah, 0x0b
        mov bh, 0x00            ; change bg color
        mov bl, 0x01            ; blue
        int 0x10

	;; print out a 'T' to screen
	mov ah, 0x0e
	mov al, 'T'
	int 0x10
	
	hlt			; halt cpu
