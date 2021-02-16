;;;
;;; print_fileTable.asm: prints file table entries to screen
;;;
;;;	input 1: cursor Y position (address, not value)
;;; input 2: cursor X position (address, not value)
;;;
use32
print_fileTable:
	push ebp
	mov ebp, esp

	mov byte [fileTable_drive_num], dl	; Get passed in drive #

	mov eax, [ebp+12]
	mov dword [fileTable_csr_y], eax	; Cursor Y address
	mov eax, [ebp+8]	
	mov dword [fileTable_csr_x], eax	; Cursor X address

	;; Print file table heading
	push fileTableHeading
	push dword [fileTable_csr_y]
	push dword [fileTable_csr_x]
	call print_string
	add esp, 12

    ;; Load file table string from its memory location (1000h), print file
    ;;   and program names & sector numbers to screen
    ;; --------------------------------------------------------------------
	;; Load file table sector from disk to memory, to get any updates
    mov dl, [fileTable_drive_num]   
    push dword fileTable_name
    push dword 10   
    push dword 1000h
    call load_file
    add esp, 12

    mov bx, 1000h   ; 1000h = file table memory location
	
	;; Now read file table from memory location 1000h
    xor cx, cx              ; reset counter for # of bytes at current filetable entry

filename_loop:
    mov al, [BX]
    cmp al, 0               ; is file name null? at end of filetable?
	je end_print_fileTable	; if end of filetable, done printing, return to caller
	
	mov word [save_bx], bx	; save values
	mov word [save_cx], cx
	
	;; Otherwise print char in al to screen
	xor ah, ah
	push eax
	push dword [fileTable_csr_y]
	push dword [fileTable_csr_x]
	call print_char
	add esp, 12

	mov bx, word [save_bx]	; restore values
	mov cx, word [save_cx]	

	cmp cx, 9		; if at end of name, go on
	je file_ext
	inc cx			; increment file entry byte counter
	inc bx			; get next byte at file table
	jmp filename_loop

file_ext:
	;; 2 blanks before file extension
	mov cx, 2
	call print_blanks_loop

	;; File ext byte 1
	inc bx
	xor ah, ah
	mov al, [BX]

	;; Print char
	mov word [save_bx], bx
	push eax
	push dword [fileTable_csr_y]
	push dword [fileTable_csr_x]
	call print_char
	add esp, 12

	;; File ext byte 2
	mov bx, word [save_bx]
	inc bx
	mov al, [BX]

	;; Print char
	mov word [save_bx], bx
	xor ah, ah
	push eax
	push dword [fileTable_csr_y]
	push dword [fileTable_csr_x]
	call print_char
	add esp, 12

	;; File ext byte 3
	mov bx, word [save_bx]
	inc bx
	mov al, [BX]

	;; Print char
	mov word [save_bx], bx
	xor ah, ah
	push eax
	push dword [fileTable_csr_y]
	push dword [fileTable_csr_x]
	call print_char
	add esp, 12

	;; Directory entry # section
	;; 9 blanks before entry #
	mov bx, word [save_bx]
	mov cx, 9
	call print_blanks_loop
	
	inc bx
	xor dx, dx		; Clear DX fully before printing 
	mov dl, [BX]
	mov word [save_bx], bx

	;; Print hex value
	push dword [fileTable_csr_y]
	push dword [fileTable_csr_x]
	call print_hex
	add esp, 8

	;; Starting sector section
	;; 9 blanks before starting sector
	mov bx, word [save_bx]
	mov cx, 4
	call print_blanks_loop
	
	inc bx
	mov dl, [BX]

	mov word [save_bx], bx

	;; Print hex value
	push dword [fileTable_csr_y]
	push dword [fileTable_csr_x]
	call print_hex
	add esp, 8

	;; File size section
	;; 14 blanks before file size
	mov bx, word [save_bx]
	mov cx, 9
	call print_blanks_loop
	
	inc bx
	mov dl, [BX]

	mov word [save_bx], bx

	;; Print hex value
	push dword [fileTable_csr_y]
	push dword [fileTable_csr_x]
	call print_hex
	add esp, 8

	mov bx, word [save_bx]
	mov al, 0Ah		; line feed

	;; Print char
	xor ah, ah
	push eax
	push dword [fileTable_csr_y]
	push dword [fileTable_csr_x]
	call print_char
	add esp, 12

	mov al, 0Dh		; carriage return

	;; Print char
	xor ah, ah
	push eax
	push dword [fileTable_csr_y]
	push dword [fileTable_csr_x]
	call print_char
	add esp, 12

	mov bx, word [save_bx]

	inc bx			; get first byte of next file name
	xor cx, cx		; reset counter for next file name
	jmp filename_loop
	
end_print_fileTable:
	mov esp, ebp
	pop ebp

	ret		; Return to calling program

	;; Small routine to print out cx # of spaces to screen
print_blanks_loop:
	mov ax, 0020h	; AL = ' '

	.loop:
		mov word [blanks_num], cx	; save values
		mov word [save_bx], bx

		;; Print char
		push eax
		push dword [fileTable_csr_y]
		push dword [fileTable_csr_x]
		call print_char
		add esp, 12

		mov cx, word [blanks_num]	; restore values
		mov bx, word [save_bx]
	loop .loop
	ret

;; --------------------------------------------------------------------
;; Variables
;; --------------------------------------------------------------------
;; CONSTANTS
nl equ 0Ah,0Dh

fileTable_csr_x: dd 0
fileTable_csr_y: dd 0
fileTable_drive_num: db 0
fileTable_name: db 'fileTable '
blanks_num: dw 0
save_bx: dw 0
save_cx: dw 0

fileTableHeading: db nl,\
	'---------   ---------   -------   ------------   --------------',\
	nl,'File Name   Extension   Entry #   Start Sector   Size (sectors)',\
	nl,'---------   ---------   -------   ------------   --------------',\
	nl,0
