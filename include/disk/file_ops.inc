;;;
;;; delete_file.inc: Delete a file from the disk
;;;
;;;	Assume drive number is in DL
;;; Parms: 1 - File name to delete
;;;		   2 - Length of file name
;;;
delete_file:
	push bp
	mov bp, sp
	sub sp, 2	; Local variable on stack - error/return code
	push si
	push bx

	mov byte [file_drive_num], dl		; Store passed in drive #

	;; Get parms off stack 
	mov bx, [bp+4]	; Length of file name
	mov si, [bp+6]	; File name to delete

	;; Copy filename to variable
	mov cx, bx
	mov di, file_name
	rep	movsb

	;; Copy filename length to variable
	mov word [file_name_length], bx

	;; Reset file name address
	mov si, file_name

	call check_filename			; Check for filename in file table

	cmp word [error_code], 0	; Check error code
	je .found_file

	mov word [bp-2], 1			; error occurred
	jmp del_function_epilogue
	
	.found_file:
		; Get starting sector
		mov al, [ES:DI+14]
		mov byte [starting_sector], al

		; Get file size
		mov al, [ES:DI+15]
		mov byte [file_size], al

		;; Reset disk
		xor ax, ax		; int 13h ah 00h - reset disk 
		mov dl, [file_drive_num]		; drive #
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
		mov al, byte [file_size]
		stosb							; File size

		;; Write changed filetable to disk - TODO: Assuming file table is 1 sector, 
		;;   change later
		xor ch, ch			; track #
		mov cl, 0Ch			; sector # to start writing at
		xor dh, dh			; dh = head #
		mov dl, [file_drive_num]		; drive #
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
		mov cl, byte [file_size]
		imul cx, 512
		rep stosb

		
		; Write zeroed-out data to disk
		mov ah, 03h							; int 13h ah 03h - write disk sectors	
		mov al, byte [file_size]		; # of sectors to write
		xor ch, ch							; track #
		mov cl, byte [starting_sector]	; Sector # to start writing at
		xor dx, dx							; dh = head #
		mov dl, [file_drive_num]		; drive #
		xor bx, bx							; ES:BX <- file data memory location	

		mov word [bp-2], 0	; Success, normal return

		int 13h
		jnc del_function_epilogue

		mov word [bp-2], 1	; Write file data error occurred

	del_function_epilogue:
		pop bx			; Restore registers - reverse order
		pop si

		jmp return_to_caller	; Global return for this file

;;;
;;; load_file: Read a file from filetable and its sectors into a memory location
;;;
;;; Parms: SI - File name, ES:BX - memory location (ES - memory address for segment; BX - memory address for offset) to load file data to
;;;
load_file:
	;; Function prologue - save callers stack values & registers
	push bp
	mov bp, sp
	sub sp, 2		; Make room for local stack variable - error code

	push bx		; Save modified registers
	push fs
	push si

	mov byte [file_drive_num], dl

	;; Get parms off stack - reverse order/LIFO!!
	mov bx, [bp+4]	; 1st parm off stack, offset memory address
	mov fs, [bp+6]	; 2nd parm, segment memory address
	mov si, [bp+8]	; 3rd parm, file name (address)

	;; Copy filename to variable
	mov cx, 10
	mov di, file_name
	rep movsb
		
	;; Reset file_name address
	mov si, file_name

	call check_filename			; Check for filename in file table

	cmp word [error_code], 0	; Check error code
	je .found_file

	mov word [bp-2], 1			; error occurred
	jmp load_function_epilogue

	.found_file:
		;; Get file type into variable to pass back
		mov al, [ES:DI+10]
		mov byte [file_type], al
		mov al, [ES:DI+11]
		mov byte [file_type+1], al
		mov al, [ES:DI+12]
		mov byte [file_type+2], al
		
		;; Reset disk
		xor ax, ax		; int 13h ah 00h - reset disk 
		mov dl, [file_drive_num]		; drive #
		int 13h 
		
		;; Set up parms for int 13h ah02h - read disk sectors
		mov word [bp-2], 0	; Init error code to 'success'

		mov cl, [ES:DI+14]	; Sector # to start reading at

		;; Get correct track and starting sector #s
		xor ah, ah
		mov al, cl
		mov cl, 18	; 18 = number of sectors per track for a floppy disk
		div cl		; AX / byte value (in CL); AL = quotient, AH = remainder
		mov ch, al	; CH = track #, quotient
		mov cl, ah	; CL = sector #, remainder
		cmp cl, 0
		jne .after
		mov cl, 18	

		.after:
		mov ah, 02h			; int 13h ah02h
		mov al, [ES:DI+15]	; # of sectors to read
		mov dx, fs
		mov es, dx		 	; segment address to load to; BX already set
		xor dx, dx			; dh = head #
		mov dl, [file_drive_num]		; drive #
		
		int 13h
		jnc load_function_epilogue

		;; Otherwise handle error here...
		file_load_error:	
			mov word [bp-2], 1	; Signifies error occurred

		load_function_epilogue:
			;; Restore callers stack values & registers
			pop si
			pop dx
			pop fs
			pop bx

			;; Return file type in BX
			mov bx, file_type

			jmp return_to_caller	; Global return for this file

;;;
;;; rename_file.inc: rename a file in the file table!!
;;;
;;; parms:
;;;		input 1: File name
;;;		input 2: File name length
;;;		input 3: New file name
;;;		input 4: New file name length
;;;
rename_file:
	push bp
	mov bp, sp
	sub sp, 2
	push bx
	push cx
	push si

	mov byte [file_drive_num], dl

	;; Get parms off stack
	mov si, [bp+4]	; New file name length
	mov dx, [bp+6]	; New file name
	mov cx, [bp+8]	; File name length
	mov bx, [bp+10] ; Old file name

	;; Put file name len into variable
	mov word [file_name_length], cx

	;; Put file name len into variable
	mov word [new_file_name_length], si

	;; Put og filename into variable
	mov si, bx
	mov di, file_name
	mov cx, word [file_name_length]
	rep movsb
	
	;; Put new g filename into variable
	mov si, dx
	mov di, new_file_name
	mov cx, word [new_file_name_length]
	rep movsb

	;; Reset file name address
	mov si, file_name

	call check_filename			; Check for filename in file table

	cmp word [error_code], 0	; Check error code
	je .found_file

	mov word [bp-2], 1			; error occurred
	jmp ren_function_epilogue

	.found_file:
	;; Found, replace with new file name - overwrite with spaces first
	mov al, ' '
	mov cx, 10
	rep stosb

	sub di, 10
	mov si, new_file_name
	mov cx, word [new_file_name_length]
	rep movsb

	;; Reset disk
	xor ax, ax
	mov dl, [file_drive_num]		; drive #
	int 13h
	
	;; Write changed file table data to disk
	mov word [bp-2], 0		; Init return code to success

	xor ch, ch			; track #
	mov cl, 0Ch			; sector # to start writing at
	xor dx, dx			; dh = head #
	mov dl, [file_drive_num]		; drive #
	xor ax, ax
	mov es, ax
	mov bx, 1000h		; ES:BX <- 0000h:1000h (file table memory location)
	mov ah, 03h			; int 13h ah 03h - write disk sectors	
	mov al, 01h			; # of sectors to write
	
	int 13h
	jnc ren_function_epilogue ; Success

	mov word [bp-2], 1		; Error on writing file table

	ren_function_epilogue:
		pop si
		pop cx
		pop bx

		jmp return_to_caller	; Global return for this file

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
	push si

	mov byte [file_drive_num], dl

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
	mov di, file_name
	rep	movsb

	;; Copy file type to variable
	mov di, file_type
	mov si, dx
	mov cx, 3
	rep movsb

	;; Reset file_name address
	mov si, file_name
	mov word [file_name_length], 10

	call check_filename			; Check for filename in file table

	cmp word [error_code], 0	; Check error code
	je new_file					; Found existing file TODO: Change to je found_file

	;; Otherwise check for free space in file table and write file at that entry
	mov si, free_space
	mov word [file_name_length], 10
	call check_filename

	;; This will write at first available FREE SPACE or at end of file table if no free
	;;  space found
	jmp new_file		

;;	.found_file:
		;; File exists in filetable and on disk
		;;   TODO: Ask user to overwrite current file or copy to new file
	
		;; TODO: Overwrite current file

		;; TODO: Create copy and save to new file

		;; TODO: Otherwise handle write sectors error here...

	new_file:
		;; Reset disk
		xor ax, ax		; int 13h ah 00h - reset disk 
		mov dl, [file_drive_num]		; drive #
		int 13h 

		;; Grab previous file table entry's starting sector + file size
		;;  for new starting sector
		sub di, 16
		mov al, [ES:DI+14]	; Starting sector
		add al, [ES:DI+15]	; File size	
		mov byte [last_saved_sector], al
		add di, 16

		;; Create new file table entry at end of current entries
		;; File name
		mov cx, 10
		mov si, file_name
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
		mov cl, 0Ch			; sector # to start writing at
		xor dx, dx			; dh = head #
		mov dl, [file_drive_num]		; drive #
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
		mov cl, byte [last_saved_sector]	; Sector # to start writing at

		;; Get correct track and starting sector #s
		xor ah, ah
		mov al, cl
		mov cl, 18	; 18 = number of sectors per track for a floppy disk
		div cl		; AX / byte value (in CL); AL = quotient, AH = remainder
		mov ch, al	; CH = track #, quotient
		mov cl, ah	; CL = sector #, remainder
		cmp cl, 0
		jne .after
		mov cl, 18	

		.after:
		mov ah, 03h							; int 13h ah 03h - write disk sectors	
		mov al, byte [file_size]			; # of sectors to write
		xor dx, dx							; dh = head #
		mov dl, [file_drive_num]			; drive #
		mov bx, word [saved_bx]				; ES:BX <- file data memory location	

		int 13h
		jnc save_function_epilogue

		;; TODO: Handle disk write error for file data here...
		mov word [bp-2], 1	; Signifies error occurred

	save_function_epilogue:
		;; Restore callers stack values & registers
		pop si			; Restore changed registers
		pop cx
		pop fs
		pop bx

		jmp return_to_caller

	;;;-------------------------------------
	;;; Check for filename in filetable
	;;;-------------------------------------
	check_filename:
		;; Init error code
		mov word [error_code], 0

		xor ax, ax
		mov es, ax		
		mov di, 1000h	; 0000h:1000h = location of fileTable in memory

		;; Assume filename is in SI register
		check_next_name:
			mov al, [ES:DI]
			cmp al, [si]
			je compare_filenames

			cmp al, 0
			jne .next_name	
			mov word [error_code], 1	; Reached end of filetable, name not found
			push di						; Maybe hacky edit, add push di to fix bugs
			jmp check_filename_return

			.next_name:
				add di, 16
				jmp check_next_name

		compare_filenames:
			push di		; save start of file name in entry
			mov cx, word [file_name_length]

			rep cmpsb
			je check_filename_return	; File name found, return
			
			mov	si, file_name
			pop di
			add di, 16			; go to next file table entry
			jmp check_next_name

		check_filename_return:
			pop di				; restore di pos from stack
			ret
	;;; Global return to caller
	;;;-------------------------------------
	return_to_caller:
		mov ax, [bp-2]	; Error/return code
		mov sp, bp		; Reset stack pointer
		pop bp			; Reset caller's base pointer
		ret

;; "Global" variables
file_name: times 10 db 0
file_name_length: dw 0
new_file_name: times 10 db 0
new_file_name_length: dw 0
file_type: times 3 db 0
file_size: db 0
starting_sector: db 0
free_space: db 'FREE SPACE'
last_saved_sector: db 0
saved_bx: dw 0
error_code: dw 0
file_drive_num: db 0
