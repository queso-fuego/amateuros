;;;
;;; delete_file.inc: Delete a file from the disk
;;;
;;; Parms: 1 - File name to delete
;;;		   2 - Length of file name
;;;
delete_file:
	push bp
	mov bp, sp
	sub sp, 2	; Local variable on stack - error/return code
	push si
	push bx

	;; Get parms off stack 
	mov bx, [bp+4]	; Length of file name
	mov si, [bp+6]	; File name to delete

	;; Copy filename to variable
	mov cx, bx
	mov di, del_file_name
	rep	movsb

	;; Copy filename length to variable
	mov word [del_file_name_length], bx

	;; Reset file name address
	mov si, del_file_name

	;; Search filetable for filename
	xor ax, ax
	mov es, ax		
	mov di, 1000h		; 0000h:1000h = location of fileTable in memory
	
	check_filename_delete:
		mov al, [ES:DI]
		cmp al, [si]
		je compare_filenames_delete
		
		cmp al, 0		; at end of file table?
		jne .next_name
		mov word [bp-2], 1	; Write file data error occurred
		jmp del_return_to_caller

		.next_name:
			add di, 16		; go to next file entry in file table
			jmp check_filename_delete

	compare_filenames_delete:
		push di			; save start of file name in entry

		mov cx, word [del_file_name_length]

		rep cmpsb
		je found_file_delete
		
		mov	si, del_file_name
		pop di
		add di, 16			; go to next file table entry
		jmp check_filename_delete

	found_file_delete:
		pop di

		; Get starting sector
		mov al, [ES:DI+14]
		mov byte [del_starting_sector], al

		; Get file size
		mov al, [ES:DI+15]
		mov byte [del_file_size], al

		;; Reset disk
		xor ax, ax		; int 13h ah 00h - reset disk 
		xor dl, dl		; Disk 0
		int 13h 
		
		;; Write changed file table entry first
		mov cx, 10
		mov si, free_space	; File name
		rep movsb			; Move 10 bytes of file name in si to file table in di

		mov cx, 3
		mov al, ' '		; File type 
		rep stosb		; Move 3 bytes of file type in al to file table

		xor al, al
		stosb							; Directory entry # (0 = first entry)
		stosb							; Starting sector
		mov al, byte [del_file_size]
		stosb							; File size

		;; Write changed filetable to disk - TODO: Assuming file table is 1 sector, 
		;;   change later
		xor ch, ch			; track #
		mov cl, 07h			; sector # to start writing at
		xor dx, dx			; dh = head #, dl = drive #
		xor ax, ax
		mov es, ax
		mov bx, 1000h		; ES:BX <- 0000h:1000h (file table memory location)
		mov ah, 03h			; int 13h ah 03h - write disk sectors	
		mov al, 01h			; # of sectors to write
	
		int 13h
		jnc write_blank_file_data ; Success, now write file data

		;; TODO: Handle disk write error for file table here...

	write_blank_file_data:
		;; Set up memory location to write NULL data to (ES:BX)
		mov ax, 800h
		mov es, ax					; Segment address of file data to write (800h:0000h = 8000h)
		xor di, di

		; Write nulls to memory location
		xor ax, ax
		xor ch, ch
		mov cl, byte [del_file_size]
		imul cx, 512
		rep stosb

		
		; Write zeroed-out data to disk
		mov ah, 03h							; int 13h ah 03h - write disk sectors	
		mov al, byte [del_file_size]		; # of sectors to write
		xor ch, ch							; track #
		mov cl, byte [del_starting_sector]	; Sector # to start writing at
		xor dx, dx							; dh = head #, dl = drive #
		xor bx, bx							; ES:BX <- file data memory location	

		mov word [bp-2], 0	; Success, normal return

		int 13h
		jnc del_return_to_caller

		mov word [bp-2], 1	; Write file data error occurred

	del_return_to_caller:
		mov ax, [bp-2]	; Error/return code
		pop bx			; Restore registers - reverse order
		pop si

		mov sp, bp		; Restore stack pointer
		pop bp			; Restore caller's base pointer

		ret		; Return to caller

;; Variables
del_file_name: times 10 db 0
del_file_name_length: dw 0
del_file_size: db 0
del_starting_sector: db 0
free_space: db 'FREE SPACE'

