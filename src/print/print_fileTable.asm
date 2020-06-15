;;;
;;; print_fileTable.asm: prints file table entries to screen
;;;
print_fileTable:
	pusha
	
	mov si, fileTableHeading
    call print_string

    ;; Load file table string from its memory location (1000h), print file
    ;;   and program names & sector numbers to screen
    ;; --------------------------------------------------------------------
    xor cx, cx              ; reset counter for # of bytes at current filetable entry
    mov ax, 1000h           ; file table location
    mov es, ax              ; ES = 1000h
    xor bx, bx              ; ES:BX = 1000h:0000h 
    mov ah, 0Eh             ; get ready to print to screen

filename_loop:
    mov al, [ES:BX]
    cmp al, 0               ; is file name null? at end of filetable?
	je end_print_fileTable	; if end of filetable, done printing, return to caller
	
	int 10h			; otherwise print char in al to screen
	cmp cx, 9		; if at end of name, go on
	je file_ext
	inc cx			; increment file entry byte counter
	inc bx			; get next byte at file table
	jmp filename_loop

file_ext:
	;; 2 blanks before file extension
	mov cx, 2
	call print_blanks_loop

	inc bx
	mov al, [ES:BX]
	int 10h
	inc bx
	mov al, [ES:BX]
	int 10h
	inc bx
	mov al, [ES:BX]
	int 10h

dir_entry_number:
	;; 9 blanks before entry #
	mov cx, 9
	call print_blanks_loop
	
	inc bx
	mov al, [ES:BX]
	call hex_to_ascii
	int 10h

start_sector_number:
	;; 9 blanks before starting sector
	mov cx, 9
	call print_blanks_loop
	
	inc bx
	mov al, [ES:BX]
	call hex_to_ascii
	int 10h

file_size:
	;; 14 blanks before file size
	mov cx, 14
	call print_blanks_loop
	
	inc bx
	mov al, [ES:BX]
	call hex_to_ascii
	int 10h
	mov al, 0Ah
	int 10h
	mov al, 0Dh
	int 10h

	inc bx			; get first byte of next file name
	xor cx, cx		; reset counter for next file name
	jmp filename_loop
	
end_print_fileTable:
	popa
	ret
