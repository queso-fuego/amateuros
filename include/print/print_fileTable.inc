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
	;; Load file table sector from disk to memory, to get any updates
	mov bx, 100h
	mov es, bx
	xor bx, bx		; ES:BX <- 100h:0000h = 1000h; file table memory location
	
	xor dx, dx		; DH = head #, DL = drive #
	mov cx, 0007h	; CH = cylinder #, CL = starting sector # to read

retry_load:
	mov ax, 0201h	; AH = 02h/int13h read disk sectors, AL = # of sectors to read

	int 13h
	jc retry_load

	;; Now read file table from memory location 1000h
    xor cx, cx              ; reset counter for # of bytes at current filetable entry
    mov ax, 100h            ; file table location
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

	;; Directory entry # section
	;; 9 blanks before entry #
	mov cx, 9
	call print_blanks_loop
	
	inc bx
	mov al, [ES:BX]
	call hex_to_ascii
	int 10h

	;; Starting sector section
	;; 9 blanks before starting sector
	mov cx, 9
	call print_blanks_loop
	
	inc bx
	mov al, [ES:BX]
	call hex_to_ascii
	int 10h

	;; File size section
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

	;; Small routine to print out cx # of spaces to screen
print_blanks_loop:
	mov ah, 0Eh
	mov al, ' '
	int 10h
	loop print_blanks_loop
	ret

;; --------------------------------------------------------------------
;; Variables
;; --------------------------------------------------------------------
;; CONSTANTS
nl equ 0Ah,0Dh

fileTableHeading:   db nl,\
	'---------   ---------   -------   ------------   --------------',\
	nl,'File Name   Extension   Entry #   Start Sector   Size (sectors)',\
	nl,'---------   ---------   -------   ------------   --------------',\
	nl,0
