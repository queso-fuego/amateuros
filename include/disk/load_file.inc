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

	;; Get parms off stack - reverse order/LIFO!!
	mov bx, [bp+4]	; 1st parm off stack, offset memory address
	mov fs, [bp+6]	; 2nd parm, segment memory address
	mov si, [bp+8]	; 3rd parm, file name (address)

	;; Copy filename to variable
	mov cx, 10
	mov di, file_name_loading
	rep movsb
		
	;; Reset file_name address
	mov si, file_name_loading
	
	;; Search filetable for filename
	mov ax, 100h
	mov es, ax		
	xor di, di		; 100h:0000h = 1000h; location of fileTable in memory
	
	check_filename_loading:
		mov al, [ES:DI]
		cmp al, 0		; End of file table?
		je file_load_error

		cmp al, [si]
		je compare_filenames_loading
		
		add di, 16		; go to next file entry in file table
		jmp check_filename_loading

	compare_filenames_loading:
		push di

		mov cx, 10

		rep cmpsb	
		jne restart_check_loading
		je found_file_loading
	
		restart_check_loading:
			mov	si, file_name_loading 
			pop di
			add di, 16		; go to start of next file table entry
			jmp check_filename_loading

	found_file_loading:
		;; Get file type into variable to pass back
		mov al, [ES:DI]
		mov byte [file_type_loading], al
		mov al, [ES:DI+1]
		mov byte [file_type_loading+1], al
		mov al, [ES:DI+2]
		mov byte [file_type_loading+2], al
		
		;; Reset disk
		xor ax, ax		; int 13h ah 00h - reset disk 
		xor dl, dl		; Disk 0
		int 13h 
		
		;; Set up parms for int 13h ah02h - read disk sectors
		mov word [bp-2], 0	; Init error code to 'success'

		xor ch, ch			; Track #
		mov cl, [ES:DI+4]	; Sector # to start reading at
		mov ah, 02h			; int 13h ah02h
		mov al, [ES:DI+5]	; # of sectors to read
		mov dx, fs
		mov es, dx		 	; segment address to load to; BX already set
		xor dx, dx			; dh = head #, dl = drive # 
		
		int 13h
		jnc return_to_caller_loading

		;; Otherwise handle error here...
		file_load_error:	
			mov word [bp-2], 1	; Signifies error occurred

		return_to_caller_loading:
			;; Function epilogue - Restore callers stack values & registers
			mov ax, [bp-2]	; Error/return code
			pop si
			pop dx
			pop fs
			pop bx

			;; Return file type in BX
			mov bx, file_type_loading

			mov sp, bp		; Reset stack pointer
			pop bp			; Reset caller's base pointer
			ret

;;; Variables
file_name_loading: times 10 db 0
file_type_loading: times 3 db 0
