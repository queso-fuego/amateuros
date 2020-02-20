;;; =========================================================================== 
;;; Kernel.asm: basic 'kernel' loaded from our bootsector
;;; ===========================================================================
        ;; --------------------------------------------------------------------
        ;; Screen & Menu Set up
        ;; --------------------------------------------------------------------
main_menu:
        ;; Reset screen state
        call resetTextScreen
        
        ; print menu header & options 
        mov si, menuString
        call print_string

        ;; --------------------------------------------------------------------
        ;; Get user input, print to screen & choose menu option/run command
        ;; --------------------------------------------------------------------
get_input:
	mov si, prompt
	call print_string
	xor cx, cx		; reset byte counter of input
        mov si, cmdString       ; si now pointing to command string
	
	mov ax, 0x2000		; reset ES & DS segments to kernel area
	mov es, ax
	mov ds, ax
	
keyloop:
        xor ax, ax              ; ah = 0x0, al = 0x0
        int 0x16                ; BIOS int get keystroke ah=0, al <- character

        mov ah, 0x0e
        cmp al, 0xD             ; user pressed enter?
        je run_command		; run user input command or load file/pgm
	
        int 0x10                ; else print input character to screen
        mov [si], al            ; store input char to string
	inc cx			; increment byte counter of input
        inc si                  ; go to next byte at si/cmdString
        jmp keyloop             ; loop for next character from user

run_command:
	cmp cx, 0
	je input_not_found  	; handle empty input
	
        mov byte [si], 0        ; else null terminate cmdString from di
	mov si, cmdString	; reset si to point to start of user input

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
	
	pop cx			; reset command length
	
	;; If command not input, search file table entries for user input file
check_files:
	mov ax, 1000h		; reset ES:BX to start of file table (0x1000:0x0000)
	mov es, ax
        xor bx, bx
	
	mov si, cmdString	; reset si to start of user input string

check_next_char:
        mov al, [ES:BX]         ; get file table character
        cmp al, 0               ; at end of file table?
        je input_not_found	; if so, no file/pgm found for user input :(

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
	
        add bx, 4		; go to starting sector # in file table entry
        mov cl, [ES:BX]         ; sector number to start reading at
	inc bx
	mov bl, [ES:BX]		; file size in sectors / # of sectors to read
	mov byte [fileSize], bl

	xor ax, ax
        mov dl, 0x00            ; disk # 
        int 0x13		; int 13h ah 0 = reset disk system

        mov ax, 0x8000          ; memory location to load pgm to
        mov es, ax
	mov al, bl		; # of sectors to read
        xor bx, bx              ; ES:BX <- 0x8000:0x0000

        mov ah, 0x02            ; int 13h ah 2 = read disk sectors to memory
        mov ch, 0x00            ; track #
        mov dh, 0x00            ; head #
        mov dl, 0x00            ; drive #

        int 0x13
        jnc run_program	        ; carry flag not set, success

        mov si, pgmNotLoaded    ; else error, program did not load correctly
        call print_string
        jmp get_input		; go back to prompt for input

run_program:
	;; Check file extension in file table entry, if 'bin'/binary, then far jump & run
	;;   Else if 'txt', then print content to screen
	mov cx, 3
	mov si, fileExt
	mov ax, 2000h  		; Reset es to kernel space for comparison (ES = DS)
	mov es, ax		; ES <- 0x2000
	mov di, fileBin
	repe cmpsb
	jne print_txt_file
	
        mov ax, 0x8000          ; program loaded, set segment registers to location
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax
        jmp 0x8000:0x0000       ; far jump to program to execute
	
print_txt_file:
	mov ax, 8000h 		; Set ES back to file memory location
	mov es, ax		; ES <- 0x8000
	xor cx, cx
	mov ah, 0Eh
	;; Get size of filesize in bytes (512 byte per sector)
	;; TODO: File size in sectors is in hex - convert to decimal!
add_cx_size:		
	cmp byte [fileSize], 0
	je print_file_char
	add cx, 512
	dec byte [fileSize]
	jne add_cx_size	

print_file_char:
	mov al, [ES:BX]
	int 10h			; Print file character to screen
	inc bx
	loop print_file_char	; Keep printing characters and decrement CX till 0
	jmp get_input		; after all printed, go back to prompt

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
        jmp 0xFFFF:0x0000

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
        mov ah, 0x0c            ; int 0x10 ah 0x0C - write gfx pixel
        mov al, 0x02            ; green
        mov bh, 0x00            ; page number

        ;; Starting pixel of square
        mov cx, 100             ; column #
        mov dx, 100             ; row #
        int 0x10

        ;; Pixels for columns 
squareColLoop:
        inc cx
        int 0x10
        cmp cx, 150
        jne squareColLoop

        ;; Go down one row
        inc dx
        int 0x10
        mov cx, 99 
        cmp dx, 150
        jne squareColLoop       ; pixels for next row

        mov ah, 0x00
        int 0x16                ; get keystroke
        jmp main_menu

        ;; --------------------------------------------------------------------
        ;; Clear Screen
        ;; --------------------------------------------------------------------
clear_screen:
	call resetTextScreen
	jmp get_input
	
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
print_hex_as_ascii:
	mov ah, 0x0e
	add al, 0x30		; convert to ascii number
	cmp al, 0x39		; is value 0h-9h or A-F
	jle hexNum
	add al, 0x7		; add hex 7 to get ascii 'A'-'F'
hexNum:
	int 0x10
	ret

	;; Small routine to print out cx # of spaces to screen
print_blanks_loop:
	mov ah, 0x0e
	mov al, ' '
	int 0x10
	loop print_blanks_loop
	ret
	
        ;; --------------------------------------------------------------------
        ;; Include Files
        ;; --------------------------------------------------------------------
        include "../print/print_string.asm"
        include "../print/print_hex.asm"
        include "../print/print_registers.asm"
	include "../print/print_fileTable.asm"
	;; include "../screen/clearScreen.asm"
        include "../screen/resetTextScreen.asm"
        include "../screen/resetGraphicsScreen.asm"
      
        ;; --------------------------------------------------------------------
        ;; Variables
        ;; --------------------------------------------------------------------
	;; Carriage return/linefeed; "newline"
	nl equ 0xA,0xD
	
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
        times 1536-($-$$) db 0   ; pads out 0s until we reach 1536th byte

