;;;
;;; load_file: Read a file from filetable and its sectors into a memory location
;;;
;;; Parms: SI - Filename, ES:BX - memory location (ES - memory address for segment; BX - memory address for offset) to load file data to
;;;
load_file:
	;; Get parms off stack - reverse order/LIFO!!
	pop bx
	pop fs			; FS:BX <- memory location to load data to
	pop si			; File name
		
	;; Copy filename to variable
	mov cx, 10
	mov di, file_name_loading
	.loop:
		lodsb
		stosb
	loop .loop
		
	;; Reset file_name address
	mov si, file_name_loading

	;; Search filetable for filename
	xor ax, ax
	mov es, ax		
	mov di, 1000h	; 0000h:1000h = location of fileTable in memory
	
	mov cx, 10

	check_filename_loading:
		mov al, [ES:DI]
		cmp al, 0		; End of file table?
		je return_to_caller_loading

		cmp al, [si]
		je compare_filenames_loading
		
		add di, 16		; go to next file entry in file table
		jmp check_filename_loading

	compare_filenames_loading:
		push di

		.loop:
			mov al, [ES:DI]
			inc di
			cmp al, [si]
			jne restart_check_loading

			dec cl
			jz found_file_loading
			inc si	
			jmp .loop
	
		restart_check_loading:
			mov	si, file_name_loading 
			pop di
			inc di
			jmp check_filename_loading


	found_file_loading:
		;; Reset disk
		xor ax, ax		; int 13h ah 00h - reset disk 
		xor dl, dl		; Disk 0
		int 13h 
		
		;; Set up parms for int 13h ah02h - read disk sectors
		add di, 4	
		xor ch, ch			; Track #
		mov cl, [ES:DI]		; Sector # to start reading at
		inc di
		mov ax, fs
		mov es, ax		 	; segment address to load to; BX already set
		mov ah, 02h			; int 13h ah02h
		mov al, [ES:DI]		; # of sectors to read
		xor dx, dx			; dh = head #, dl = drive # 
		
		int 13h
		jnc return_to_caller_loading
		
		;; TODO: Otherwise handle error here...

		return_to_caller_loading:
			ret

;;; Variables
file_name_loading:	times 10 db 0
