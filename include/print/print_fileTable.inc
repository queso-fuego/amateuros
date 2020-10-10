;;;
;;; print_fileTable.asm: prints file table entries to screen
;;;
;;;	input 1: cursor Y position
;;; input 2: cursor X position
;;;	output 1: new cursor Y position in BX
;;; output 2: new cursor X position in CX
;;;	ASSUME THIS IS A KERNEL ONLY INCLUDE FILE - VARIABLES/etc. MAY BE IN KERNEL.ASM, NOT HERE
;;;
print_fileTable:
	push bp
	mov bp, sp

	mov ax, [bp+6]
	mov word [fileTable_cursor_y], ax
	mov ax, [bp+4]	
	mov word [fileTable_cursor_x], ax

	;; Print file table heading
	mov si, fileTableHeading
	push si
	push word [fileTable_cursor_y]
	push word [fileTable_cursor_x]
	call print_string_text_mode
	add sp, 6

	;; Move cursor
	mov word [fileTable_cursor_y], bx
	mov word [fileTable_cursor_x], cx
	push bx
	push cx
	call move_cursor
	add sp, 4

    ;; Load file table string from its memory location (1000h), print file
    ;;   and program names & sector numbers to screen
    ;; --------------------------------------------------------------------
	;; Load file table sector from disk to memory, to get any updates
	mov bx, 100h
	mov es, bx
	xor bx, bx		; ES:BX <- 100h:0000h = 1000h; file table memory location
	
	xor dx, dx		; DH = head #, DL = drive #
	mov cx, 000Bh	; CH = cylinder #, CL = starting sector # to read

retry_load:
	mov ax, 0201h	; AH = 02h/int13h read disk sectors, AL = # of sectors to read

	int 13h
	jc retry_load

	;; Now read file table from memory location 1000h
    xor cx, cx              ; reset counter for # of bytes at current filetable entry
    mov ax, 100h            ; file table location

filename_loop:
    mov al, [ES:BX]
    cmp al, 0               ; is file name null? at end of filetable?
	je end_print_fileTable	; if end of filetable, done printing, return to caller
	
	mov word [save_bx], bx	; save values
	mov word [save_cx], cx
	
	;; Otherwise print char in al to screen
	xor ah, ah
	push ax
	push word [fileTable_cursor_y]
	push word [fileTable_cursor_x]
	call print_char_text_mode
	add sp, 6

	;; Move cursor
	mov word [fileTable_cursor_y], bx
	mov word [fileTable_cursor_x], cx
	push bx
	push cx
	call move_cursor
	add sp, 4

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
	mov al, [ES:BX]

	;; Print char
	mov word [save_bx], bx
	push ax
	push word [fileTable_cursor_y]
	push word [fileTable_cursor_x]
	call print_char_text_mode
	add sp, 6

	;; Move cursor
	mov word [fileTable_cursor_y], bx
	mov word [fileTable_cursor_x], cx
	push bx
	push cx
	call move_cursor
	add sp, 4

	;; File ext byte 2
	mov bx, word [save_bx]
	inc bx
	mov al, [ES:BX]

	;; Print char
	mov word [save_bx], bx
	xor ah, ah
	push ax
	push word [fileTable_cursor_y]
	push word [fileTable_cursor_x]
	call print_char_text_mode
	add sp, 6

	;; Move cursor
	mov word [fileTable_cursor_y], bx
	mov word [fileTable_cursor_x], cx
	push bx
	push cx
	call move_cursor
	add sp, 4

	;; File ext byte 3
	mov bx, word [save_bx]
	inc bx
	mov al, [ES:BX]

	;; Print char
	mov word [save_bx], bx
	xor ah, ah
	push ax
	push word [fileTable_cursor_y]
	push word [fileTable_cursor_x]
	call print_char_text_mode
	add sp, 6

	;; Move cursor
	mov word [fileTable_cursor_y], bx
	mov word [fileTable_cursor_x], cx
	push bx
	push cx
	call move_cursor
	add sp, 4

	;; Directory entry # section
	;; 9 blanks before entry #
	mov bx, word [save_bx]
	mov cx, 9
	call print_blanks_loop
	
	inc bx
	xor dx, dx		; Clear DX fully before printing 
	mov dl, [ES:BX]
	mov word [save_bx], bx

	;; Print hex value
	push word [fileTable_cursor_y]
	push word [fileTable_cursor_x]
	call print_hex
	add sp, 4

	;; Update cursor position
	mov word [fileTable_cursor_y], bx
	mov word [fileTable_cursor_x], cx

	;; Starting sector section
	;; 9 blanks before starting sector
	mov bx, word [save_bx]
	mov cx, 4
	call print_blanks_loop
	
	inc bx
	mov dl, [ES:BX]

	mov word [save_bx], bx

	;; Print hex value
	push word [fileTable_cursor_y]
	push word [fileTable_cursor_x]
	call print_hex
	add sp, 4

	;; Update cursor position
	mov word [fileTable_cursor_y], bx
	mov word [fileTable_cursor_x], cx

	;; File size section
	;; 14 blanks before file size
	mov bx, word [save_bx]
	mov cx, 9
	call print_blanks_loop
	
	inc bx
	mov dl, [ES:BX]

	mov word [save_bx], bx

	;; Print hex value
	push word [fileTable_cursor_y]
	push word [fileTable_cursor_x]
	call print_hex
	add sp, 4

	;; Update cursor position
	mov word [fileTable_cursor_y], bx
	mov word [fileTable_cursor_x], cx

	mov bx, word [save_bx]
	mov al, 0Ah		; line feed

	;; Print char
	xor ah, ah
	push ax
	push word [fileTable_cursor_y]
	push word [fileTable_cursor_x]
	call print_char_text_mode
	add sp, 6

	;; Move cursor
	mov word [fileTable_cursor_y], bx
	mov word [fileTable_cursor_x], cx
	push bx
	push cx
	call move_cursor
	add sp, 4

	mov al, 0Dh		; carriage return

	;; Print char
	xor ah, ah
	push ax
	push word [fileTable_cursor_y]
	push word [fileTable_cursor_x]
	call print_char_text_mode
	add sp, 6

	;; Move cursor
	mov word [fileTable_cursor_y], bx
	mov word [fileTable_cursor_x], cx
	push bx
	push cx
	call move_cursor
	add sp, 4

	mov bx, word [save_bx]

	inc bx			; get first byte of next file name
	xor cx, cx		; reset counter for next file name
	jmp filename_loop
	
end_print_fileTable:
	mov bx, word [fileTable_cursor_y]	; New cursor Y position
	mov cx, word [fileTable_cursor_x]	; New cursor X position

	mov sp, bp
	pop bp

	ret		; Return to calling program

	;; Small routine to print out cx # of spaces to screen
print_blanks_loop:
	mov ax, 0020h	; AL = ' '

	.loop:
		mov word [blanks_num], cx	; save values
		mov word [save_bx], bx

		;; Print char
		push ax
		push word [fileTable_cursor_y]
		push word [fileTable_cursor_x]
		call print_char_text_mode
		add sp, 6

		;; Move cursor
		mov word [fileTable_cursor_y], bx
		mov word [fileTable_cursor_x], cx
		push bx
		push cx
		call move_cursor
		add sp, 4

		mov cx, word [blanks_num]	; restore values
		mov bx, word [save_bx]
	loop .loop
	ret

;; --------------------------------------------------------------------
;; Variables
;; --------------------------------------------------------------------
;; CONSTANTS
nl equ 0Ah,0Dh

fileTable_cursor_x: dw 0
fileTable_cursor_y: dw 0
blanks_num: dw 0
save_bx: dw 0
save_cx: dw 0

fileTableHeading:   db nl,\
	'---------   ---------   -------   ------------   --------------',\
	nl,'File Name   Extension   Entry #   Start Sector   Size (sectors)',\
	nl,'---------   ---------   -------   ------------   --------------',\
	nl,0
