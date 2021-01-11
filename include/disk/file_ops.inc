;;;
;;; delete_file.inc: Delete a file from the disk
;;;
;;;	Assume drive number is in DL
;;; Parms: 1 - File name to delete
;;;		   2 - Length of file name
;;;
use32
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
        mov al, 0Dh         ; Sector to start writing at (sectors are 0-based!)
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

        push ds             ; Save DS
        xor ax, ax
        mov ds, ax
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

        pop ds              ; Restore DS

		;; TODO: Handle disk write error for file table here...
        ;;   Check error ata pio register here...

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

        push ds                 ; Save DS
        xor ax, ax
        mov ds, ax
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

        pop ds              ; Restore DS

		mov word [bp-2], 0	; Success, normal return

        ;; TODO: Check error register here
;;		jnc del_function_epilogue

	del_function_epilogue:
		pop bx			; Restore registers - reverse order
		pop si

		jmp return_to_caller	; Global return for this file

;;;
;;; load_file: Read a file from filetable and its sectors into a memory location
;;;
;;; input 1: File name (address)
;;; input 2: File name length   NEW
;;; input 3: Memory segment to load data to
;;; input 4: Memory offset to load data to
;;;
load_file:
	;; Function prologue - save callers stack values & registers
	push bp
	mov bp, sp
	sub sp, 2		; Make room for local stack variable - error code

	push bx		; Save modified registers
	push fs
    push dx
	push si

	mov byte [file_drive_num], dl

	;; Get parms off stack - reverse order/LIFO!!
	mov bx, [bp+4]	; 1st parm off stack, offset memory address
	mov fs, [bp+6]	; 2nd parm, segment memory address
    mov cx, [bp+8]  ; 3rd parm, file name length
	mov si, [bp+10]	; 4th parm, file name (address)

    ;; Copy filename length to variable
    mov word [file_name_length], cx     

	;; Copy filename to variable
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
		
		mov word [bp-2], 0	; Init error code to 'success'

		.after:
		mov al, [ES:DI+15]	            ; # of sectors to read
        mov byte [file_size], al

		mov al, [ES:DI+14]	; Sector # to start reading at 
        mov byte [starting_sector], al     

        ;; Read sectors using ATA PIO ports
        push es             ; Save values
        push di
	    mov es, [bp+6]	    ; 2nd parm off stack, segment memory address to read into
	    mov di, [bp+4]	    ; 1st parm off stack, offset memory address to read into
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

        mov cx, 256         ; # of words to read for 1 sector
        mov dx, 1F0h        ; Data port, reading 
        rep insw            ; Read bytes from DX port # into (ES:)DI, CX # of times
        
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
            pop di
            pop es

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

	;; Write changed file table data to disk
	mov word [bp-2], 0		; Init return code to success

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
    mov al, 0Dh         ; Sector to start writing at (sectors are 0-based!)
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

    push ds             ; Save DS
    xor ax, ax
    mov ds, ax
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

    pop ds              ; Restore DS

    jmp ren_function_epilogue

    ;; TODO: Check error registers here

	mov word [bp-2], 1		; Error on writing file table

	ren_function_epilogue:
		pop si
		pop cx
		pop bx

		jmp return_to_caller	; Global return for this file

;;;
;;; save_file.inc: Save file data to disk
;;;
;;; Input 1: File name (address)
;;; Input 2: File type (.bin,.txt,etc)
;;; Input 3: File size (in hex sectors)
;;; Input 4: Memory segment to save from
;;; Input 5: Memory offset to save from
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
		;; Grab previous file table entry's starting sector + file size
		;;  for new starting sector
		sub di, 16
		mov al, [ES:DI+14]	; Starting sector
		add al, [ES:DI+15]	; File size	
		mov byte [last_saved_sector], al
		add di, 16

		;; Create new file table entry at end of current entries
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
        mov al, 0Dh         ; Sector to start writing at (sectors are 0-based!)
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

        push ds             ; Save DS
        xor ax, ax
        mov ds, ax
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

        pop ds              ; Restore DS

		;; TODO: Handle disk write error for file table here...

	write_file_data:
		mov word [bp-2], 0					; Init error code to 'success'

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

        push ds                 ; Save DS
        mov ds, [bp+6]          ; Memory segment to save data from 
        mov si, [bp+4]          ; Memory offset to save data from

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

        pop ds              ; Restore DS

    jmp save_function_epilogue

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
