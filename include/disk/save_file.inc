;;;
;;; save_file.inc: Save file data to disk
;;;
;;; Parms: File name, file type (bin or other), file size (in hex sectors), 
;;;        memory location to save data from
;;;
save_file:
	;; Function prologue - save callers stack values & registers
	push bp
	mov bp, sp
	sub sp, 2		; Make room for local stack variable - error code
	push bx
	push fs			
	push cx		
	push dx	
	push si

	;; Get parms off stack - reverse order
	mov bx, [bp+4]
	mov fs, [bp+6]		; FS:BX memory location to save data from
	mov cx, [bp+8]		; File size
	mov dx, [bp+10]		; File type
	mov si, [bp+12]		; File name
	
	;; Copy file size to variable
	mov byte [file_size], cl

	;; Copy filename to variable
	mov cx, 10
	mov di, file_name_saving
	rep	movsb

	;; Copy file type to variable
	mov di, file_type
	mov si, dx
	mov cx, 3
	rep movsb

	;; Reset file_name address
	mov si, file_name_saving

	;; Search filetable for filename
	xor ax, ax
	mov es, ax		
	mov di, 1000h		; 0000h:1000h = location of fileTable in memory
	
	check_filename_saving:
		;; Get starting sector of file + file size in sectors 
		mov al, byte [ES:DI+14]			; Starting sector
		cmp al, 0
		je new_file					; No starting sector, file doesn't exist, create new file

		add al, byte [ES:DI+15]		; File size in hex sectors 
		mov byte [last_saved_sector], al

		mov al, [ES:DI]
		cmp al, [si]
		je compare_filenames_saving
		
		add di, 16		; go to next file entry in file table
		jmp check_filename_saving

	compare_filenames_saving:
		push di

		mov cx, 10

		rep cmpsb
		jne restart_check_saving
		je found_file_saving
		
		restart_check_saving:
			mov	si, file_name_saving
			pop di
			add di, 16			; go to next file table entry
			jmp check_filename_saving

	found_file_saving:
		;; Reset disk
		xor ax, ax		; int 13h ah 00h - reset disk 
		xor dl, dl		; Disk 0
		int 13h 
		
		;; File exists in filetable and on disk
		;;   TODO: Ask user to overwrite current file or copy to new file
	
		;; TODO: Overwrite current file

		;; TODO: Create copy and save to new file

		;; TODO: Otherwise handle write sectors error here...

	new_file:
		;; Create new file table entry at end of current entries
		;; File name
		mov cx, 10
		mov si, file_name_saving
		rep movsb			; Move 10 bytes of file name in si to file table in di
		mov cx, 3
		mov si, file_type
		rep movsb						; Move 3 bytes of file type in si to file table
		xor al, al
		stosb							; Directory entry # (0 = first entry)
		mov al, byte [last_saved_sector]	
		stosb							; Starting sector
		mov al, byte [file_size]
		stosb							; File size

		;; Write changed filetable to disk - TODO: Assuming file table is 1 sector, 
		;;   change later
		xor ch, ch			; track #
		mov cl, 06h			; sector # to start writing at
		xor dx, dx			; dh = head #, dl = drive #
		mov word [saved_bx], bx	
		xor ax, ax
		mov es, ax
		mov bx, 1000h		; ES:BX <- 0000h:1000h (file table memory location)
		mov ah, 03h			; int 13h ah 03h - write disk sectors	
		mov al, 01h			; # of sectors to write
	
		int 13h
		jnc write_file_data ; Success, now write file data

		;; TODO: Handle disk write error for file table here...

	write_file_data:
		mov word [bp-2], 0					; Init error code to 'success'

		mov ax, fs
		mov es, ax							; Segment address of file data to write

		mov ah, 03h							; int 13h ah 03h - write disk sectors	
		mov al, byte [file_size]			; # of sectors to write
		xor ch, ch							; track #
		mov cl, byte [last_saved_sector]	; Sector # to start writing at
		xor dx, dx							; dh = head #, dl = drive #
		mov bx, word [saved_bx]				; ES:BX <- file data memory location	

		int 13h
		jnc return_to_caller_saving

		;; TODO: Handle disk write error for file data here...
		mov word [bp-2], 1	; Signifies error occurred

	return_to_caller_saving:
		;; Function epilogue - Restore callers stack values & registers
		mov ax, [bp-2]	; Error/return code
		pop si			; Restore changed registers
		pop dx
		pop cx
		pop fs
		pop bx
		mov sp, bp		; Reset stack pointer
		pop bp			; Reset caller's base pointer
		ret

;; Variables
file_name_saving: times 10 db 0
file_type: times 3 db 0
file_size: db 0
last_saved_sector: db 0
saved_bx: dw 0
error_code: dw 0
