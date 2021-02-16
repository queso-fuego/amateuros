;;; =========================================================================== 
;;; Kernel.asm: basic 'kernel' loaded from our bootsector
;;; ===========================================================================
use32
org 2000h   
        ;; --------------------------------------------------------------------
        ;; Screen & Menu Set up
        ;; --------------------------------------------------------------------
main_menu:
    ;; Get passed drive number
    xor dh, dh 
    mov [kernel_drive_num], dx

    ;; Clear the screen
    call clear_screen

    ; Print OS boot message
    push dword menuString
    push dword kernel_cursor_y		
    push dword kernel_cursor_x
    call print_string
    add esp, 12

        ;; --------------------------------------------------------------------
        ;; Get user input, print to screen & choose menu option/run command
        ;; --------------------------------------------------------------------
get_input:
	; Reset tokens data, arrays, and variables for next input line
	mov edi, tokens
	xor ax, ax
	mov cx, 50
	rep stosb

	mov edi, tokens_length
	mov cx, 5
	rep stosw

	mov byte [token_count], 0	

	xor al, al
	mov edi, token_file_name1
	mov cx, 10
	rep stosb
	mov edi, token_file_name2
	mov cx, 10
	rep stosb

    mov edi, cmdString
    mov cx, 255
    rep stosb

	; Print prompt
	push dword prompt			; Address of string to print - input 1
	push dword kernel_cursor_y	; Row to print to - input 2
	push dword kernel_cursor_x	; Col to print to - input 3
	call print_string
	add esp, 12

	; Move cursor after prompt
	push dword [kernel_cursor_y]
	push dword [kernel_cursor_x]
	call move_cursor
	add esp, 8
	
	xor cx, cx			; reset byte counter of input
    mov esi, cmdString   ; si now pointing to command string
	
keyloop:
    call get_key            ; Get ascii char from scancode from port 60h into AL

    cmp al, 0Dh             ; user pressed enter?
    je tokenize_input_line  ; tokenize user input line
    
	cmp al, 08h				; backspace?
	jne .not_backspace

	jcxz .there
	dec esi					; yes, go back one char in cmdString
    mov byte [esi], 0       ; blank out character
	dec cx					; byte counter - 1
	
	.there:
	cmp word [kernel_cursor_x], 2	; At start of line?
	je keyloop						; Yes, skip

	; Move cursor back 1 space
	dec word [kernel_cursor_x]		; Otherwise move back

	;; Print 2 spaces at cursor
	push dword 0020h
	push dword kernel_cursor_y
	push dword kernel_cursor_x
	call print_char
	add esp, 12

	push dword 0020h
	push dword kernel_cursor_y
	push dword kernel_cursor_x
	call print_char
	add esp, 12

	; Move cursor back 2 spaces
	sub word [kernel_cursor_x], 2
	push dword [kernel_cursor_y]
	push dword [kernel_cursor_x]
	call move_cursor
	add esp, 8

	jmp keyloop

.not_backspace:
    mov [esi], al            ; store input char to string
	inc cx					; increment byte counter of input
	mov word [save_cx], cx
    inc esi                  ; go to next byte at si/cmdString

	; Print input character to screen
	push eax					; Character to print - input 1
	push dword kernel_cursor_y	; Row to print to - input 2
	push dword kernel_cursor_x	; Column to print to - input 3
	call print_char
	add esp, 12					; Clean up stack 

	; Move cursor 
	push dword [kernel_cursor_y]
	push dword [kernel_cursor_x]
	call move_cursor
	add esp, 8

	mov cx, word [save_cx]

    jmp keyloop             ; loop for next character from user

	;; Prompt/"Shell" commands
tokenize_input_line:
    cmp cx, 0
	je input_not_found  	; handle empty input
	
    mov byte [esi], 0       ; else null terminate cmdString from si
	mov esi, cmdString		; reset si to point to start of user input

	;; Tokenize input string "cmdString" into separate tokens
	mov edi, tokens			; DI <- tokens array		[5][10]
	mov ebx, tokens_length	; BX <- tokens_length array [5][2]
	.get_token_loop:
		lodsb				; mov al, [ds:si] & inc si
		cmp al, 0h			; At end of input?
		je check_commands

		; Skip whitespace between tokens
		.whitespace_loop:
			cmp al, ' ' 
			jne .alphanum_loop	
			lodsb
		jmp .whitespace_loop

		; Get all alphanumeric characters for current token
		; when not alphanumeric, skip and go to next token/whitespace check
		.alphanum_loop:
			cmp al, '0'					; Check numeric first
			jl .next_token
			cmp al, '9'
			jg .check_uppercase
			stosb
			inc word [ebx]		; increment length of current token
			lodsb
			jmp .alphanum_loop
			
			.check_uppercase:
				cmp al, 'A'
				jl .next_token
				cmp al, 'Z'
				jg .check_underscore
				stosb
				inc word [ebx]
				lodsb
				jmp .alphanum_loop

			.check_underscore:
				cmp al, '_'
				jl .next_token
				jg .check_lowercase
				stosb
				inc word [ebx]
				lodsb
				jmp .alphanum_loop

			.check_lowercase:
				cmp al, 'a'
				jl .next_token
				cmp al, 'z'
				jg .next_token
				stosb
				inc word [ebx]			
				lodsb
				jmp .alphanum_loop

			.next_token:
				inc byte [token_count]		; next token

				; Move di to next token position in tokens array
                push ebx    ; save ebx value

				xor ebx, ebx
				mov bl, byte [token_count]
				imul bx, 10
				lea edi, [tokens+bx]

				; Move to next position in tokens_length array
                pop ebx                 ; restore ebx
                inc ebx
                inc ebx
				
				dec esi					; get token loop does lodsb again, prevent error here
	jmp .get_token_loop

check_commands:	
	;; Get first token (command to run) & second token (if applicable e.g. file name)
	xor ch, ch
	mov cx, word [tokens_length]

	mov esi, tokens
	push cx
	mov edi, cmdDir
	repe cmpsb
	je fileTable_print

	pop cx
	push cx
	mov edi, cmdReboot
	mov esi, cmdString
	repe cmpsb
	je reboot

	pop cx
	push cx
    mov edi, cmdPrtreg
	mov esi, cmdString
	repe cmpsb
	je registers_print

	pop cx
	push cx
	mov edi, cmdGfxtst
	mov esi, cmdString
	repe cmpsb
	je graphics_test

	pop cx
	push cx
	mov edi, cmdHlt
	mov esi, cmdString
	repe cmpsb
	je end_program

	pop cx
	push cx
	mov edi, cmdCls
	mov esi, cmdString
	repe cmpsb
	je kernel_clear_screen

	pop cx
	push cx
	mov edi, cmdShutdown
	mov esi, cmdString
	repe cmpsb
	je shutdown

	pop cx
	push cx
	mov edi, cmdDelFile
	mov esi, cmdString
	repe cmpsb
	je del_file

	pop cx			
	push cx
	mov edi, cmdRenFile
	mov esi, cmdString
	repe cmpsb
	je ren_file

	pop cx			; reset command length

	;; If command not input, search file table entries for user input file
    ;; Call load_file function to load program/file to memory address 8000h
    ;; Return values from load_file:
    ;;   AX - Return/error code (0 = Success, !0 = error)
    ;;   BX - File type/extension (.bin/.txt, etc)
    mov dl, [kernel_drive_num]	; drive #

    push dword cmdString         ; Input 1: file name (address)
    push dword [tokens_length]   ; Input 2: file name length
    push dword 8000h             ; Input 3: memory offset to load to
    call load_file
    add esp, 12

    cmp ax, 0               ; Return code, 0 if successful
    je run_program	        

    ; Error, program did not load correctly
	;; Print string
	push dword pgmNotLoaded
	push dword kernel_cursor_y
	push dword kernel_cursor_x
    call print_string
	add esp, 12

	;; Move cursor
	push dword [kernel_cursor_y]
	push dword [kernel_cursor_x]
	call move_cursor
	add esp, 8

    jmp get_input		    ; go back to prompt for input

run_program:
	;; Check file extension in file table entry, if 'bin'/binary, then far jump & run
	;;   Else if 'txt', then print content to screen
    ;; File extension/type is returned in BX from call to load_file
    mov edi, fileExt
    mov al, [ebx]
    stosb
    mov al, [ebx+1]
    stosb
    mov al, [ebx+2]
    stosb
    
	mov cx, 3
	mov esi, fileExt
	mov edi, fileBin
	repe cmpsb
	jne print_txt_file

	;; Reset cursor positions so printing is good when returning to kernel later
	mov word [kernel_cursor_y], 0
	mov word [kernel_cursor_x], 0

	mov dl, [kernel_drive_num]	; Store drive # to use

    jmp 8000h     ; far jump to program to execute
	
print_txt_file:
    mov ebx, 8000h      ; 8000h = file location to print from
	
	;; Get size of filesize in bytes (512 byte per sector)
add_cx_size:		
    ;; TODO: Change this later - currently assuming text files are only 1 sector
;;	imul cx, word [fileSize], 512 
    mov cx, 512

print_file_char:
	mov al, [BX]
	cmp al, 0Fh
	jle call_h_to_a
	
return_file_char:	
	;; Print file character to screen
	push eax
	push dword kernel_cursor_y
	push dword kernel_cursor_x
	call print_char
	add esp, 12

	;; Move cursor
	push dword [kernel_cursor_y]
	push dword [kernel_cursor_x]
	call move_cursor
	add esp, 8

	inc bx
	loop print_file_char	; Keep printing characters and decrement CX till 0

	;; Print newline after done printing file contents
	push dword 000Ah
	push dword kernel_cursor_y
	push dword kernel_cursor_x
	call print_char
	add esp, 12

	;; Move cursor
	push dword [kernel_cursor_y]
	push dword [kernel_cursor_x]
	call move_cursor
	add esp, 8
	
	push dword 000Dh
	push dword kernel_cursor_y
	push dword kernel_cursor_x
	call print_char
	add esp, 12

	;; Move cursor
	push dword [kernel_cursor_y]
	push dword [kernel_cursor_x]
	call move_cursor
	add esp, 8

	jmp get_input   		; after all printed, go back to prompt

call_h_to_a:
	call hex_to_ascii		; convert AL hex character to ASCII character 
	jmp return_file_char
	
input_not_found:
	pop cx					; In case program was not found

    ; command not found! boo D:
	;; Print string
	push dword failure
	push dword kernel_cursor_y
	push dword kernel_cursor_x
	call print_string
	add esp, 12

	;; Move cursor
	push dword [kernel_cursor_y]
	push dword [kernel_cursor_x]
	call move_cursor
	add esp, 8

    jmp get_input 

        ;; --------------------------------------------------------------------
        ;; File/Program browser & loader   
        ;; --------------------------------------------------------------------
fileTable_print: 
	mov dl, [kernel_drive_num]			; pass drive #
	push dword kernel_cursor_y
	push dword kernel_cursor_x
	call print_fileTable
	add esp, 8

	jmp get_input

        ;; --------------------------------------------------------------------
        ;; Reboot: far jump to reset vector
        ;; --------------------------------------------------------------------
reboot: 
    jmp 0FFFFh:0000h

        ;; --------------------------------------------------------------------
        ;; Print Register Values
        ;; --------------------------------------------------------------------
registers_print:
	push dword kernel_cursor_y
	push dword kernel_cursor_x
    call print_registers
	add esp, 8

	jmp get_input		; return to prompt '>:'
	
        ;; --------------------------------------------------------------------
        ;; Graphics Mode Test(s)
        ;; --------------------------------------------------------------------
graphics_test:
        ;; TODO: Fill this out later after moving to a VBE graphics mode,
        ;;   Put examples of graphics primitives here such as: Line drawing, triangles,
        ;;    squares, other polygons, circles, etc.
        jmp reboot              ; Jump to reset vector, reset errything TODO: TEMP FIX

        ;; --------------------------------------------------------------------
        ;; Clear Screen
        ;; --------------------------------------------------------------------
kernel_clear_screen:
	call clear_screen

	; Update cursor values for new position
	mov word [kernel_cursor_y], 0
	mov word [kernel_cursor_x], 0

	jmp get_input

        ;; --------------------------------------------------------------------
        ;; Delete a file from the disk
        ;; --------------------------------------------------------------------
del_file:
	;; 1 - File name to delete
	;; 2 - Length of file name
	mov esi, tokens	; File name is 2nd token in array, each token is 10 char max
	add si, 10
	mov edi, token_file_name1
	xor ch, ch
	mov cx, word [tokens_length+2]
	rep movsb
	mov cx, word [tokens_length+2]

	mov dl, [kernel_drive_num]
	push dword token_file_name1	; File name
	push ecx						; Length of file name
	call delete_file
	add esp, 8

	;; TODO: Check return code for errors
	cmp ax, 0	; 0 = Success/Normal return

	; Print newline when done
	push dword 000Ah
	push dword kernel_cursor_y
	push dword kernel_cursor_x
	call print_char
	add esp, 12

	push dword 000Dh
	push dword kernel_cursor_y
	push dword kernel_cursor_x
	call print_char
	add esp, 12

	jmp get_input

        ;; --------------------------------------------------------------------
        ;; Rename a file in the file table
        ;; --------------------------------------------------------------------
ren_file:
	;; 1 - File to rename
	;; 2 - Length of name to rename
	;; 3 - New file name
	;; 4 - New file name length
	mov esi, tokens	; File name is 2nd token in array, each token is 10 char max
	add si, 10
	mov edi, token_file_name1
	mov cx, word [tokens_length+2]
	rep movsb
	push dword token_file_name1	    ; File name			- input 1
	push dword [tokens_length+2]	; File name length  - input 2

	mov esi, tokens	; New file name is 3rd token in array, each token is 10 char max
	add si, 20
	mov edi, token_file_name2
	mov cx, word [tokens_length+4]
	rep movsb

	mov dl, [kernel_drive_num]
	push dword token_file_name2	; New file name		   - input 3
	push dword [tokens_length+4] ; New file name length - input 4
	call rename_file
	add esp, 8

	;; TODO: Check return code for errors
	cmp ax, 0	; 0 = Success/Normal return

	; Print newline when done
	push dword 000Ah
	push dword kernel_cursor_y
	push dword kernel_cursor_x
	call print_char
	add esp, 12

	push dword 000Dh
	push dword kernel_cursor_y
	push dword kernel_cursor_x
	call print_char
	add esp, 12

	jmp get_input

	    ;; --------------------------------------------------------------------
        ;; Shutdown (QEMU)
        ;; --------------------------------------------------------------------
shutdown:
	mov ax, 2000h
	mov dx, 604h
	out dx, ax

        ;; --------------------------------------------------------------------
        ;; End Program  
        ;; --------------------------------------------------------------------
end_program:
;;    cli                   ; clear interrupts
    hlt                     ; halt cpu

        ;; ====================================================================
        ;; End Main Logic
        ;; ====================================================================
	
        ;; --------------------------------------------------------------------
        ;; Include Files
        ;; --------------------------------------------------------------------
		include "../include/print/print_char.inc"
		include "../include/print/print_string.inc"
        include "../include/print/print_hex.inc"
        include "../include/print/print_registers.inc"
		include "../include/print/print_fileTable.inc"
		include "../include/screen/clear_screen.inc"
		include "../include/screen/move_cursor.inc"
		include "../include/type_conversions/hex_to_ascii.inc"
		include "../include/disk/file_ops.inc"
        include "../include/keyboard/get_key.inc"
	
        ;; --------------------------------------------------------------------
        ;; Variables
        ;; --------------------------------------------------------------------
;; CONSTANTS
nl equ 0Ah,0Dh	; Carriage return/linefeed; "newline"
SPACE equ 20h	; ASCII space character	

;; REGULAR VARIABLES
menuString: db '--------------------------------------------------',nl,\
        'Kernel Booted, Welcome to QuesOS - 32 Bit Edition!',nl,\
        '--------------------------------------------------',nl,nl,0
prompt:	db '>:',0
	
success:        db nl,'Program Found!',nl,0
failure:	db nl,'Command/Program not found, Try again',nl,0
	
windowsMsg:     db nl,'Oops! Something went wrong :(',nl,0
pgmNotLoaded:   db nl,'Program found but not loaded, Try Again',nl,0
	
	;; Prompt commands
cmdDir:		db 'dir',0	; directory command; list all files/pgms on disk
cmdReboot:	db 'reboot',0   ; 'warm' reboot option
cmdPrtreg:	db 'prtreg',0	; print register values
cmdGfxtst:	db 'gfxtst',0   ; graphics mode test
cmdHlt:		db 'hlt',0      ; e(n)d current program by halting cpu
cmdCls:		db 'cls',0	; clear screen by scrolling
cmdShutdown: db 'shutdown',0 ; Close QEMU emulator
cmdEditor:	db 'editor',0	; launch editor program
cmdDelFile: db 'del',0		; Delete a file from disk
cmdRenFile: db 'ren',0		; Rename a file in the file table
        
notFoundString: db nl,'Program/file not found!, Try again? (Y)',nl,0
sectNotFound:   db nl,'Sector not found!, Try again? (Y)',nl,0

cmdLength:      db 0

goBackMsg:      db nl,nl,'Press any key to go back...', 0
dbgTest:        db nl,'Test',nl,0
fileExt:	db '   ',0
fileSize:	db 0
fileBin:	db 'bin',0
fileTxt:	db 'txt',0
tokens: times 50 db 0		; tokens array, equivalent-ish to tokens[5][10]
tokens_length: times 5 dw 0	; length of each token in tokens array (0-10)
token_count: db 0			; How many tokens did the user enter?
kernel_save_bx:	dw 0
token_file_name1: times 10 db 0
token_file_name2: times 10 db 0
kernel_cursor_x: dw 0
kernel_cursor_y: dw 0
kernel_drive_num: dw 0
cmdString: times 255 db 0

        ;; --------------------------------------------------------------------
        ;; Sector Padding magic
        ;; --------------------------------------------------------------------
        times 7168-($-$$) db 0   ; pads out the rest with 0s
