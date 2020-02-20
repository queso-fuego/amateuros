;;;
;;; print_fileTable.asm: prints file table entries to screen
;;;
print_fileTable:
	pusha
	
	mov si, fileTableHeading
        call print_string

        ;; Load file table string from its memory location (0x1000), print file
        ;;   and program names & sector numbers to screen
        ;; --------------------------------------------------------------------
        xor cx, cx              ; reset counter for # of bytes at current filetable entry
        mov ax, 0x1000          ; file table location
        mov es, ax              ; ES = 0x1000
        xor bx, bx              ; ES:BX = 0x1000:0x0000 
        mov ah, 0x0e            ; get ready to print to screen

filename_loop:
        mov al, [ES:BX]
        cmp al, 0               ; is file name null? at end of filetable?
	je end_print_fileTable	; if end of filetable, done printing, return to caller
	
	int 0x10		; otherwise print char in al to screen
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
	int 0x10
	inc bx
	mov al, [ES:BX]
	int 0x10
	inc bx
	mov al, [ES:BX]
	int 0x10

dir_entry_number:
	;; 9 blanks before entry #
	mov cx, 9
	call print_blanks_loop
	
	inc bx
	mov al, [ES:BX]
	call print_hex_as_ascii

start_sector_number:
	;; 9 blanks before starting sector
	mov cx, 9
	call print_blanks_loop
	
	inc bx
	mov al, [ES:BX]
	call print_hex_as_ascii

file_size:
	;; 14 blanks before file size
	mov cx, 14
	call print_blanks_loop
	
	inc bx
	mov al, [ES:BX]
	call print_hex_as_ascii
	mov al, 0xA
	int 0x10
	mov al, 0xD
	int 0x10

	inc bx			; get first byte of next file name
	xor cx, cx		; reset counter for next file name
	jmp filename_loop
	
end_print_fileTable:
	popa
	ret
