;;;
;;; delete_file.inc: Delete a file from the disk
;;;
;;;	Assume drive number is in DL
;;; Parms: 1 - File name to delete
;;;		   2 - Length of file name
;;;
use32
delete_file:
	push ebp
	mov ebp, esp
	sub esp, 4	; Local variable on stack - error/return code

	push esi
	push ebx

	mov byte [file_drive_num], dl		; Store passed in drive #

	;; Get parms off stack 
	mov bx, [ebp+8]	    ; Length of file name
	mov esi, [ebp+12]	; File name to delete

	;; Copy filename to variable
	mov cx, bx
	mov edi, file_name
	rep	movsb

	;; Copy filename length to variable
	mov word [file_name_length], bx

	;; Reset file name address
	mov esi, file_name

	call check_filename			; Check for filename in file table

	cmp word [error_code], 0	; Check error code
	je .found_file

	mov dword [ebp-4], 1			; error occurred
	jmp del_function_epilogue
	
	.found_file:
		; Get starting sector
		mov al, [EDI+14]
		mov byte [starting_sector], al

		; Get file size
		mov al, [EDI+15]
		mov byte [file_size], al

		;; Write changed file table entry first
		mov cx, 10
		mov esi, free_space	; File name
		rep movsb			; Move 10 bytes of file name in si to file table in di

		mov cx, 3
		mov al, ' '		; File type 
		rep stosb		; Move 3 bytes of file type in al to file table

		xor al, al
		stosb							; Directory entry # (0 = first entry)
		stosb							; Starting sector
		mov al, byte [file_size]
		stosb							; File size

        ;; TODO: Assuming file table is 1 sector 
		;; Write changed filetable to disk - 
		;;   change later
        mov dx, 1F6h             ; Head & drive # port
        mov al, [file_drive_num] ; Drive # - hard disk 1
        and al, 0Fh              ; Head # (low nibble)
        or al, 0A0h              ; default high nibble to 'primary' drive (drive 1), 'secondary' drive (drive 2) would be hex B or 1011b
        out dx, al               ; Send head/drive #

        mov dx, 1F2h        ; Sector count port
        mov al, 1           ; # of sectors to read
        out dx, al

        mov dx, 1F3h        ; Sector # port
        mov al, 17h         ; Sector to start writing at (sectors are 0-based!)
        out dx, al

        mov dx, 1F4h        ; Cylinder low port
        xor al, al          ; Cylinder low #
        out dx, al

        mov dx, 1F5h        ; Cylinder high port
        xor al, al          ; Cylinder high #
        out dx, al

        mov dx, 1F7h        ; Command port (writing port 1F7h)
        mov al, 30h         ; Write with retry
        out dx, al

    .loop:
        in al, dx           ; Status register (reading port 1F7h)
        test al, 8          ; Sector buffer requires servicing
        je .loop            ; Keep trying until sector buffer is ready

        mov si, 1000h       ; Memory address to write sector from (0000h:1000h)

        mov cx, 256         ; # of words to write for 1 sector
        mov dx, 1F0h        ; Data port, writing 

    .write_loop:
        outsw               ; Write bytes from (DS:)SI into DX port
        jmp $+2             ; Small delay after each word written to port
        loop .write_loop

        ;; Send cache flush command after write command is finished
        mov al, 0E7h
        mov dx, 1F7h
        out dx, al

		;; TODO: Handle disk write error for file table here...
        ;;   Check error ata pio register here...

	write_blank_file_data:
		;; Set up memory location to write NULL data to (ES:BX)
        mov di, 8000h           ; File data will be at address 8000h

		; Write nulls to memory location
		xor ax, ax
		xor ch, ch
		mov cl, byte [file_size]
		imul cx, 512
		rep stosb
		
		; Write zeroed-out data to disk
        mov dx, 1F6h             ; Head & drive # port
        mov al, [file_drive_num] ; Drive # - hard disk 1
        and al, 0Fh              ; Head # (low nibble)
        or al, 0A0h              ; default high nibble to 'primary' drive (drive 1), 'secondary' drive (drive 2) would be hex B or 1011b
        out dx, al               ; Send head/drive #

        mov dx, 1F2h            ; Sector count port
        mov al, [file_size]     ; # of sectors to read
        out dx, al

        mov dx, 1F3h                ; Sector # port
        mov al, [starting_sector]   ; Sector to start writing at (sectors are 0-based!)
        out dx, al

        mov dx, 1F4h            ; Cylinder low port
        xor al, al              ; Cylinder low #
        out dx, al

        mov dx, 1F5h            ; Cylinder high port
        xor al, al              ; Cylinder high #
        out dx, al

        mov dx, 1F7h            ; Command port (writing port 1F7h)
        mov al, 30h             ; Write with retry
        out dx, al

    .loop:
        in al, dx               ; Status register (reading port 1F7h)
        test al, 8              ; Sector buffer requires servicing
        je .loop                ; Keep trying until sector buffer is ready

        mov si, 8000h           ; Memory address to write sectors from (0000h:8000h)

        xor bh, bh
        mov bl, [starting_sector]
        imul cx, bx, 256            ; CX = # of words for 1 sector * # of sectors
        mov dx, 1F0h        ; Data port, writing 

    .write_loop:
        outsw               ; Write bytes from (DS:)SI into DX port
        jmp $+2             ; Small delay after each word written to port
        loop .write_loop

        ;; Send cache flush command after write command is finished
        mov al, 0E7h
        mov dx, 1F7h
        out dx, al

		mov dword [ebp-4], 0	; Success, normal return

        ;; TODO: Check error register here
;;		jnc del_function_epilogue

	del_function_epilogue:
		pop ebx			; Restore registers - reverse order
		pop esi

		jmp return_to_caller	; Global return for this file

;;;
;;; load_file: Read a file from filetable and its sectors into a memory location
;;;
;;; input 1: File name (address)
;;; input 2: File name length   
;;; input 3: Memory offset to load data to
;;;
load_file:
	;; Function prologue - save callers stack values & registers
	push ebp
	mov ebp, esp
	sub esp, 4		; Make room for local stack variable - error code

	push ebx		; Save modified registers
    push edx
	push esi

	mov byte [file_drive_num], dl

	;; Get parms off stack - reverse order/LIFO!!
	mov ebx, [ebp+8]	; 1st parm off stack, offset memory address
    mov ecx, [ebp+12]   ; 2nd parm, file name length
	mov esi, [ebp+16]	; 3rd parm, file name (address)

    ;; Copy filename length to variable
    mov word [file_name_length], cx     

	;; Copy filename to variable
	mov edi, file_name  
	rep movsb
		
	;; Reset file_name address
	mov esi, file_name
	call check_filename			; Check for filename in file table

	cmp word [error_code], 0	; Check error code
	je .found_file

	mov dword [ebp-4], 1			; error occurred
	jmp load_function_epilogue

	.found_file:
		;; Get file type into variable to pass back
		mov al, [DI+10]
		mov byte [file_type], al
		mov al, [DI+11]
		mov byte [file_type+1], al
		mov al, [DI+12]
		mov byte [file_type+2], al
		
		mov dword [ebp-4], 0	; Init error code to 'success'

		.after:
		mov al, [DI+15]	            ; # of sectors to read
        mov byte [file_size], al

		mov al, [DI+14]	; Sector # to start reading at 
        mov byte [starting_sector], al     

        ;; Read sectors using ATA PIO ports
        push edi
	    mov edi, [ebp+8]	; 1st parm off stack, offset memory address to read into
        mov bl, [file_size] ; Will be reading <file_size> # of sectors
        dec bl

        mov dx, 1F6h             ; Head & drive # port
        mov al, [file_drive_num] ; Drive # - hard disk 1
        and al, 0Fh              ; Head # (low nibble)
        or al, 0A0h              ; default high nibble to 'primary' drive (drive 1), 'secondary' drive (drive 2) would be hex B or 1011b
        out dx, al               ; Send head/drive #

        mov dx, 1F2h             ; Sector count port
        mov al, [file_size]      ; # of sectors to read
        out dx, al

        mov dx, 1F3h              ; Sector # port
        mov al, [starting_sector] ; Sector to start reading at (sectors are 0-based!!)
        out dx, al

        mov dx, 1F4h        ; Cylinder low port
        xor al, al          ; Cylinder low #
        out dx, al

        mov dx, 1F5h        ; Cylinder high port
        xor al, al          ; Cylinder high #
        out dx, al

        mov dx, 1F7h        ; Command port (writing port 1F7h)
        mov al, 20h         ; Read with retry
        out dx, al

    ;; Poll status port after reading 1 sector
    .load_next_sector:
        in al, dx               ; Status register (reading port 1F7h)
        test al, 8              ; Sector buffer requires servicing
        je .load_next_sector    ; Keep trying until sector buffer is ready

        mov cx, 128         ; # of double words to read for 1 sector
        mov dx, 1F0h        ; Data port, reading 
        rep insd            ; Read bytes from DX port # into EDI, CX # of times
        
        ;; 400ns delay - Read alternate status register
        mov dx, 3F6h
        in al, dx
        in al, dx
        in al, dx
        in al, dx

        cmp bl, 0
        je load_function_epilogue

        dec bl
        mov dx, 1F7h
        jmp .load_next_sector

		load_function_epilogue:
        ;; TODO: Check error register here

			;; Restore callers stack values & registers
            pop edi

			pop esi
			pop edx
			pop ebx

			;; Return file type in BX
			mov ebx, file_type

			jmp return_to_caller	; Global return for this file
;;;
;;; rename_file.inc: rename a file in the file table!!
;;;
;;; parms:
;;;		input 1: File name  (address)
;;;		input 2: File name length
;;;		input 3: New file name  (address)
;;;		input 4: New file name length
;;;
rename_file:
	push ebp
	mov ebp, esp
	sub esp, 4

	push ebx
	push ecx
	push esi

	mov byte [file_drive_num], dl

	;; Get parms off stack
	mov si, [ebp+8]	; New file name length
	mov edx, [ebp+12]	; New file name
	mov cx, [ebp+16]	; File name length
	mov ebx, [ebp+20]   ; Old file name

	;; Put file name len into variable
	mov word [file_name_length], cx

	;; Put file name len into variable
	mov word [new_file_name_length], si

	;; Put og filename into variable
	mov esi, ebx
	mov edi, file_name
	mov cx, word [file_name_length]
	rep movsb
	
	;; Put new g filename into variable
	mov esi, edx
	mov edi, new_file_name
	mov cx, word [new_file_name_length]
	rep movsb

	;; Reset file name address
	mov esi, file_name

	call check_filename			; Check for filename in file table

	cmp word [error_code], 0	; Check error code
	je .found_file

	mov dword [ebp-4], 1			; error occurred
	jmp ren_function_epilogue

	.found_file:
	;; Found, replace with new file name - overwrite with spaces first
	mov al, ' '
	mov cx, 10
	rep stosb

	sub edi, 10
	mov esi, new_file_name
	mov cx, word [new_file_name_length]
	rep movsb

	;; Write changed file table data to disk
	mov dword [ebp-4], 0		; Init return code to success

    ;; Write changed filetable to disk - 
    ;;   change later
    mov dx, 1F6h             ; Head & drive # port
    mov al, [file_drive_num] ; Drive # - hard disk 1
    and al, 0Fh              ; Head # (low nibble)
    or al, 0A0h              ; default high nibble to 'primary' drive (drive 1), 'secondary' drive (drive 2) would be hex B or 1011b
    out dx, al               ; Send head/drive #

    mov dx, 1F2h        ; Sector count port
    mov al, 1           ; # of sectors to write
    out dx, al

    mov dx, 1F3h        ; Sector # port
    mov al, 17h         ; Sector to start writing at (sectors are 0-based!)
    out dx, al

    mov dx, 1F4h        ; Cylinder low port
    xor al, al          ; Cylinder low #
    out dx, al

    mov dx, 1F5h        ; Cylinder high port
    xor al, al          ; Cylinder high #
    out dx, al

    mov dx, 1F7h        ; Command port (writing port 1F7h)
    mov al, 30h         ; Write with retry
    out dx, al

.loop:
    in al, dx           ; Status register (reading port 1F7h)
    test al, 8          ; Sector buffer requires servicing
    je .loop            ; Keep trying until sector buffer is ready

    mov si, 1000h       ; Memory address to write sector from (0000h:1000h)

    mov cx, 256         ; # of words to write for 1 sector
    mov dx, 1F0h        ; Data port, writing 

.write_loop:
    outsw               ; Write bytes from (DS:)SI into DX port
    jmp $+2             ; Small delay after each word written to port
    loop .write_loop

    ;; Send cache flush command after write command is finished
    mov al, 0E7h
    mov dx, 1F7h
    out dx, al

    jmp ren_function_epilogue

    ;; TODO: Check error registers here

	mov dword [ebp-4], 1		; Error on writing file table

	ren_function_epilogue:
		pop esi
		pop ecx
		pop ebx

		jmp return_to_caller	; Global return for this file

;;;
;;; save_file.inc: Save file data to disk
;;;
;;; Input 1: File name (address)
;;; Input 2: File type (.bin,.txt,etc)
;;; Input 3: File size (in hex sectors)
;;; Input 4: Memory offset to save from
;;;
save_file:
	;; Function prologue - save callers stack values & registers
	push ebp
	mov ebp, esp
	sub esp, 4		; Make room for local stack variable - error code

	push ebx
	push ecx		
	push esi

	mov byte [file_drive_num], dl

	;; Get parms off stack - reverse order
	mov ecx, [ebp+12]	; File size
	mov edx, [ebp+16]	; File type
	mov esi, [ebp+20]	; File name
	
	;; Copy file size to variable
	mov byte [file_size], cl

	;; Copy filename to variable
	mov cx, 10
	mov edi, file_name
	rep	movsb

	;; Copy file type to variable
	mov edi, file_type
	mov esi, edx
	mov cx, 3
	rep movsb

	;; Reset file_name address
	mov esi, file_name
	mov word [file_name_length], 10

	call check_filename			; Check for filename in file table

	cmp word [error_code], 0	; Check error code
	je new_file					; Found existing file TODO: Change to je found_file

	;; Otherwise check for free space in file table and write file at that entry
	mov esi, free_space
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
		;; Grab previous file table entry's starting sector + file size
		;;  for new starting sector
		sub edi, 16        
		mov al, [EDI+14]	; Starting sector
		add al, [EDI+15]	; File size	
		mov byte [last_saved_sector], al
		add edi, 16

		;; Create new file table entry at end of current entries
		mov cx, 10
		mov esi, file_name
		rep movsb			; Move 10 bytes of file name in si to file table in di

		mov cx, 3
		mov esi, file_type
		rep movsb						; Move 3 bytes of file type in si to file table

		xor al, al
		stosb							; Directory entry # (0 = first entry)
		mov al, byte [last_saved_sector]	
		stosb							; Starting sector
		mov al, byte [file_size]
		stosb							; File size

        ;; TODO: Assuming file table is 1 sector 
		;; Write changed filetable to disk - 
		;;   change later
        mov dx, 1F6h             ; Head & drive # port
        mov al, [file_drive_num] ; Drive # - hard disk 1
        and al, 0Fh              ; Head # (low nibble)
        or al, 0A0h              ; default high nibble to 'primary' drive (drive 1), 'secondary' drive (drive 2) would be hex B or 1011b
        out dx, al               ; Send head/drive #

        mov dx, 1F2h        ; Sector count port
        mov al, 1           ; # of sectors to write
        out dx, al

        mov dx, 1F3h        ; Sector # port
        mov al, 17h         ; Sector to start writing at (sectors are 1-based)
        out dx, al

        mov dx, 1F4h        ; Cylinder low port
        xor al, al          ; Cylinder low #
        out dx, al

        mov dx, 1F5h        ; Cylinder high port
        xor al, al          ; Cylinder high #
        out dx, al

        mov dx, 1F7h        ; Command port (writing port 1F7h)
        mov al, 30h         ; Write with retry
        out dx, al

    .loop:
        in al, dx           ; Status register (reading port 1F7h)
        test al, 8          ; Sector buffer requires servicing
        je .loop            ; Keep trying until sector buffer is ready

        mov esi, 1000h      ; Memory address to write sector from  

        mov cx, 256         ; # of words to write for 1 sector
        mov dx, 1F0h        ; Data port, writing 

    .write_loop:
        outsw               ; Write bytes from (DS:)SI into DX port
        jmp $+2             ; Small delay after each word written to port
        loop .write_loop

        ;; Send cache flush command after write command is finished
        mov al, 0E7h
        mov dx, 1F7h
        out dx, al

        ;; Wait until BSY bit is clear after cache flush
        .bsy_loop:
            in al, dx
            bt ax, 7
        jc .bsy_loop

		;; TODO: Handle disk write error for file table here...

	write_file_data:
		mov dword [ebp-4], 0					; Init error code to 'success'

		.after:
        ;; Write file data to disk
        mov dx, 1F6h             ; Head & drive # port
        mov al, [file_drive_num] ; Drive # - hard disk 1
        and al, 0Fh              ; Head # (low nibble)
        or al, 0A0h              ; default high nibble to 'primary' drive (drive 1), 'secondary' drive (drive 2) would be hex B or 1011b
        out dx, al               ; Send head/drive #

        mov dx, 1F2h            ; Sector count port
        mov al, [file_size]     ; # of sectors to write
        out dx, al

        mov dx, 1F3h                ; Sector # port
        mov al, [last_saved_sector]   ; Sector to start writing at (sectors are 0-based!) 
        out dx, al

        mov dx, 1F4h            ; Cylinder low port
        xor al, al              ; Cylinder low #
        out dx, al

        mov dx, 1F5h            ; Cylinder high port
        xor al, al              ; Cylinder high #
        out dx, al

        mov dx, 1F7h            ; Command port (writing port 1F7h)
        mov al, 30h             ; Write with retry
        out dx, al

    .loop:
        in al, dx               ; Status register (reading port 1F7h)
        test al, 8              ; Sector buffer requires servicing
        je .loop                ; Keep trying until sector buffer is ready

        mov esi, [ebp+8]          ; Memory offset to save data from

        xor bh, bh
        mov bl, [file_size]
        imul cx, bx, 256    ; CX = # of words for 1 sector * # of sectors
        mov dx, 1F0h        ; Data port, writing 

    .write_loop:
        outsw               ; Write bytes from (DS:)SI into DX port
        jmp $+2             ; Small delay after each word written to port
        loop .write_loop

        ;; Send cache flush command after write command is finished
        mov al, 0E7h
        mov dx, 1F7h
        out dx, al

        ;; Wait until BSY bit is clear after cache flush
        .bsy_loop:
            in al, dx
            bt ax, 7
        jc .bsy_loop

    jmp save_function_epilogue

		;; TODO: Handle disk write error for file data here...
		mov dword [ebp-4], 1	; Signifies error occurred

	save_function_epilogue:
		;; Restore callers stack values & registers
		pop esi			; Restore changed registers
		pop ecx
		pop ebx

		jmp return_to_caller

	;;;-------------------------------------
	;;; Check for filename in filetable
	;;;-------------------------------------
	check_filename:
		;; Init error code
		mov word [error_code], 0

		mov edi, 1000h	; 1000h = location of fileTable in memory

		;; Assume filename is in SI register
		check_next_name:
			mov al, [EDI]
			cmp al, [ESI]
			je compare_filenames

			cmp al, 0
			jne .next_name	
			mov word [error_code], 1	; Reached end of filetable, name not found
			push edi					; Maybe hacky edit, add push di to fix bugs
			jmp check_filename_return

			.next_name:
				add edi, 16
				jmp check_next_name

		compare_filenames:
			push edi		; save start of file name in entry
			mov cx, word [file_name_length]

			rep cmpsb
			je check_filename_return	; File name found, return
			
			mov	esi, file_name
			pop edi
			add edi, 16			; go to next file table entry
			jmp check_next_name

		check_filename_return:
			pop edi				; restore di pos from stack
			ret

	;;; Global return to caller
	;;;-------------------------------------
	return_to_caller:
		mov eax, [ebp-4]	; Error/return code

		mov esp, ebp		; Reset stack pointer
		pop ebp			; Reset caller's base pointer
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
