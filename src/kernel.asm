;;; =========================================================================== 
;;; Kernel.asm: basic 'kernel' loaded from our bootsector
;;; ===========================================================================
        ;; --------------------------------------------------------------------
        ;; Screen & Menu Set up
        ;; --------------------------------------------------------------------
main_menu:
        ;; Reset screen state
        call clear_screen_text_mode
        
        ; print menu header & options 
        mov si, menuString
        call print_string

        ;; --------------------------------------------------------------------
        ;; Get user input, print to screen & choose menu option/run command
        ;; --------------------------------------------------------------------
get_input:
	mov si, prompt
	call print_string
	xor cx, cx			; reset byte counter of input
    mov si, cmdString   ; si now pointing to command string
	
	mov ax, 200h		; reset ES & DS segments to kernel area
	mov es, ax
	mov ds, ax
	
keyloop:
    xor ax, ax              ; ah = 00h al = 00h
    int 16h                 ; BIOS int get keystroke ah=0, al <- character

    mov ah, 0Eh
    cmp al, 0Dh             ; user pressed enter?
    je run_command			; run user input command or load file/pgm
	
	cmp al, 08h				; backspace?
	jne .not_backspace
	dec si					; yes, go back one char
	dec cx					; byte counter - 1
    int 10h                 ; else print input character to screen
	jmp keyloop

.not_backspace:
	int 10h
    mov [si], al            ; store input char to string
	inc cx					; increment byte counter of input
    inc si                  ; go to next byte at si/cmdString
    jmp keyloop             ; loop for next character from user

run_command:
	cmp cx, 0
	je input_not_found  	; handle empty input
	
    mov byte [si], 0        ; else null terminate cmdString from si
	mov si, cmdString		; reset si to point to start of user input

	;; Prompt/"Shell" commands
check_commands:
	push cx
	mov di, cmdDir
	repe cmpsb
	je fileTable_print

	pop cx
	push cx
	mov di, cmdReboot
	mov si, cmdString
	repe cmpsb
	je reboot

	pop cx
	push cx
    mov di, cmdPrtreg
	mov si, cmdString
	repe cmpsb
	je registers_print

	pop cx
	push cx
	mov di, cmdGfxtst
	mov si, cmdString
	repe cmpsb
	je graphics_test

	pop cx
	push cx
	mov di, cmdHlt
	mov si, cmdString
	repe cmpsb
	je end_program

	pop cx
	push cx
	mov di, cmdCls
	mov si, cmdString
	repe cmpsb
	je clear_screen

	pop cx
	push cx
	mov di, cmdShutdown
	mov si, cmdString
	repe cmpsb
	je shutdown

	pop cx			; reset command length
	
	;; If command not input, search file table entries for user input file
check_files:
	mov ax, 100h		; reset ES:BX to start of file table (1000h:0000h)
	mov es, ax
    xor bx, bx
	
	mov si, cmdString	; reset si to start of user input string

check_next_char:
        mov al, [ES:BX]         ; get file table character
        cmp al, 0               ; at end of file table?
        je input_not_found		; if so, no file/pgm found for user input :(

        cmp al, [si]            ; does user input match file table character?
        je start_compare

		add bx, 16              ; if not, go to next file entry in table
        jmp check_next_char

start_compare:
        push bx                 ; save file table position

compare_loop:
        mov al, [ES:BX]         ; get file table character
        inc bx                  ; next byte in input/filetable
        cmp al, [si]            ; does input match filetable char?
        jne restart_search      ; if not, search again from this point in filetable

        dec cl                  ; if it does match, decrement length counter
        jz found_program        ; counter = 0, all of input found in filetable
        inc si                  ; else go to next byte of input
        jmp compare_loop

restart_search:
        mov si, cmdString       ; reset to start of user input
        pop bx                  ; get saved file table position
        inc bx                  ; go to next char in file table
        jmp check_next_char     ; start checking again

	;; Read disk sector of program to memory and execute it by far jumping
    ;; -------------------------------------------------------------------
found_program:
	;; Get file extension - bytes 10-12 of file table entry
	mov al, [ES:BX]
	mov [fileExt], al
	mov al, [ES:BX+1]
	mov [fileExt+1], al
	mov al, [ES:BX+2]
	mov [fileExt+2], al
	
    add bx, 4			; go to starting sector # in file table entry
    mov cl, [ES:BX]     ; sector number to start reading at
	inc bx
	mov bl, [ES:BX]		; file size in sectors / # of sectors to read
	mov byte [fileSize], bl

	xor ax, ax
    mov dl, 00h         ; disk # 
    int 13h     		; int 13h ah 0 = reset disk system

    mov ax, 800h       ; memory location to load pgm to
    mov es, ax
	mov al, bl  		; # of sectors to read
    xor bx, bx          ; ES:BX <- 800h:0000h = 8000h

    mov ah, 02h         ; int 13h ah 2 = read disk sectors to memory
    mov ch, 00h         ; track #
    mov dh, 00h         ; head #
    mov dl, 00h         ; drive #

    int 13h
    jnc run_program	        ; carry flag not set, success

    mov si, pgmNotLoaded    ; else error, program did not load correctly
    call print_string
    jmp get_input		    ; go back to prompt for input

run_program:
	;; Check file extension in file table entry, if 'bin'/binary, then far jump & run
	;;   Else if 'txt', then print content to screen
	mov cx, 3
	mov si, fileExt
	mov ax, 200h  		; Reset es to kernel space for comparison (ES = DS)
	mov es, ax	    	; ES <- 2000h
	mov di, fileBin
	repe cmpsb
	jne print_txt_file
	
    mov ax, 800h       ; program loaded, set segment registers to location
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    jmp 800h:0000h     ; far jump to program to execute
	
print_txt_file:
	mov ax, 800h 		; Set ES back to file memory location
	mov es, ax	    	; ES <- 8000h
	xor cx, cx
	mov ah, 0Eh
	
	;; Get size of filesize in bytes (512 byte per sector)
add_cx_size:		
	imul cx, word [fileSize], 512 

print_file_char:
	mov al, [ES:BX]
	cmp al, 0Fh
	jle call_h_to_a
	
return_file_char:	
	int 10h     			; Print file character to screen
	inc bx
	loop print_file_char	; Keep printing characters and decrement CX till 0

	mov ax, 0E0Ah   		; Print newline after done printing file contents
	int 10h
	mov al, 0Dh
	int 10h
	jmp get_input   		; after all printed, go back to prompt

call_h_to_a:
	call hex_to_ascii
	jmp return_file_char
	
input_not_found:
    mov si, failure         ; command not found! boo D:
    call print_string
    jmp get_input 

        ;; --------------------------------------------------------------------
        ;; File/Program browser & loader   
        ;; --------------------------------------------------------------------
fileTable_print: 
	call print_fileTable
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
    mov si, printRegHeading 
    call print_string

    call print_registers
	jmp get_input		; return to prompt '>:'
	
        ;; --------------------------------------------------------------------
        ;; Graphics Mode Test(s)
        ;; --------------------------------------------------------------------
graphics_test:
        call resetGraphicsScreen

        ;; Test Square
        mov ah, 0Ch           ; int 10h ah 0Ch - write gfx pixel
        mov al, 02h           ; green
        mov bh, 00h           ; page number

        ;; Starting pixel of square
        mov cx, 100           ; column #
        mov dx, 100           ; row #
        int 10h

        ;; Pixels for columns 
squareColLoop:
        inc cx
        int 10h
        cmp cx, 150
        jne squareColLoop

        ;; Go down one row
        inc dx
        int 10h
        mov cx, 99 
        cmp dx, 150
        jne squareColLoop       ; pixels for next row

        mov ah, 00h
        int 16h                 ; get keystroke
        jmp main_menu

        ;; --------------------------------------------------------------------
        ;; Clear Screen
        ;; --------------------------------------------------------------------
clear_screen:
	call clear_screen_text_mode
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
    cli                     ; clear interrupts
    hlt                     ; halt cpu

        ;; ====================================================================
        ;; End Main Logic
        ;; ====================================================================
	
	;; Small routine to convert hex byte to ascii, assume hex digit in AL
hex_to_ascii:
	mov ah, 0Eh
	add al, 30h		; convert to ascii number
	cmp al, 39h		; is value 0h-9h or A-F
	jle hexNum
	add al, 07h		; add hex 7 to get ascii 'A'-'F'
hexNum:
	ret

	;; Small routine to print out cx # of spaces to screen
print_blanks_loop:
	mov ah, 0Eh
	mov al, ' '
	int 10h
	loop print_blanks_loop
	ret
	
        ;; --------------------------------------------------------------------
        ;; Include Files
        ;; --------------------------------------------------------------------
        include "../include/print/print_string.inc"
        include "../include/print/print_hex.inc"
        include "../include/print/print_registers.inc"
		include "../include/print/print_fileTable.inc"
		include "../include/screen/clear_screen_text_mode.inc"
        include "../include/screen/resetGraphicsScreen.inc"
	
        ;; --------------------------------------------------------------------
        ;; Variables
        ;; --------------------------------------------------------------------
	;; Carriage return/linefeed; "newline"
	nl equ 0Ah,0Dh
	
menuString:     db '---------------------------------',nl,\
        'Kernel Booted, Welcome to QuesOS!',nl,\
        '---------------------------------',nl,nl,0
	
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
	
fileTableHeading:   db nl,\
	'---------   ---------   -------   ------------   --------------',\
	nl,'File Name   Extension   Entry #   Start Sector   Size (sectors)',\
	nl,'---------   ---------   -------   ------------   --------------',\
	nl,0
        
printRegHeading:    db nl,'--------  ------------',nl,\
        'Register  Mem Location',nl,\
        '--------  ------------',nl,0

notFoundString: db nl,'Program/file not found!, Try again? (Y)',nl,0
sectNotFound:   db nl,'Sector not found!, Try again? (Y)',nl,0

cmdLength:      db 0

goBackMsg:      db nl,nl,'Press any key to go back...', 0
dbgTest:        db nl,'Test',nl,0
fileExt:	db '   ',0
fileSize:	db 0
fileBin:	db 'bin',0
fileTxt:	db 'txt',0
cmdString:      db ''

        ;; --------------------------------------------------------------------
        ;; Sector Padding magic
        ;; --------------------------------------------------------------------
        times 2048-($-$$) db 0   ; pads out 0s until we reach 1536th byte

