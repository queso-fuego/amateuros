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
        mov di, cmdString       ; di now pointing to command string
keyloop:
        xor ax, ax              ; ah = 0x0, al = 0x0
        int 0x16                ; BIOS int get keystroke ah=0, al <- character

        mov ah, 0x0e
        cmp al, 0xD             ; user pressed enter? 
        je run_command
        int 0x10                ; print input character to screen
        mov [di], al            ; store input char to string
        inc di                  ; go to next byte at di/cmdString
        jmp keyloop             ; loop for next character from user

run_command:
        mov byte [di], 0        ; null terminate cmdString from di
        mov al, [cmdString]
        cmp al, 'F'             ; file table command / menu option
        je filebrowser
        cmp al, 'R'             ; 'warm' reboot option
        je reboot              
        cmp al, 'P'             ; print register values
        je registers_print
        cmp al, 'G'             ; graphics mode test
        je graphics_test
        cmp al, 'N'             ; e(n)d current program
        je end_program
        mov si, failure         ; command not found! boo D:
        call print_string
        jmp get_input 

        ;; --------------------------------------------------------------------
        ;; Menu F) - File/Program browser & loader   
        ;; --------------------------------------------------------------------
filebrowser: 
        ;; Reset screen state
        call resetTextScreen 

        mov si, fileTableHeading
        call print_string

        ;; Load file table string from its memory location (0x1000), print file
        ;;   and program names & sector numbers to screen, for user to choose
        ;; --------------------------------------------------------------------
        xor cx, cx              ; reset counter for # of bytes at current filetable entry
        mov ax, 0x1000          ; file table location
        mov es, ax              ; ES = 0x1000
        xor bx, bx              ; ES:BX = 0x1000:0x0000 
        mov ah, 0x0e            ; get ready to print to screen

filename_loop:
	;; TODO: Add whitespace between table entry sections
        mov al, [ES:BX]
        cmp al, 0               ; is file name null? at end of filetable?
	je check_name		; if so, stop reading file name, go to extension

	int 0x10		; otherwise print char in al to screen
	inc bx			; get next byte at file table
	inc cx			; increment file entry byte counter
	jmp filename_loop

check_name:
	cmp cx, 0		; no name! should be at end of table entries
	je get_program_name
	
check_name_loop:	
        cmp cx, 9		; else, check end of max file name length?
	je file_ext		; get file extension
	inc cx			; else keep checking
	inc bx			; skip over nulls in file table entry
	jmp check_name_loop

file_ext:
	mov al, [ES:BX]
	mov ah, 0x0e
	int 0x10
	inc bx
	mov al, [ES:BX]
	int 0x10
	inc bx
	mov al, [ES:BX]
	int 0x10

dir_entry_number:
	inc bx
	mov al, [ES:BX]
	call print_hex_as_ascii

start_sector_number:
	inc bx
	mov al, [ES:BX]
	call print_hex_as_ascii

file_size:	
	inc bx
	mov al, [ES:BX]
	call print_hex_as_ascii
	mov al, 0xA
	int 0x10
	mov al, 0xD
	int 0x10

	xor cx, cx		; reset counter for next file name
	jmp filename_loop

        ;; After file table printed to screen, user can input program to load
        ;; ------------------------------------------------------------------
	;; TODO: Change to accomadate new file table layout
get_program_name:
        mov ah, 0x0e            ; print newline...
        mov al, 0xA
        int 0x10
        mov al, 0xD
        int 0x10
        mov di, cmdString       ; di now points to command string
        mov byte [cmdLength], 0 ; reset counter & length of user input

pgm_name_loop:
        xor ax, ax              ; ah = 0x0, al = 0x0
        int 0x16                ; BIOS int get keystroke ah=0, al <- character

        mov ah, 0x0e            ; BIOS teletype output
        cmp al, 0xD             ; user pressed enter? 
        je start_search

        inc byte [cmdLength]    ; if not, add to counter
        mov [di], al            ; store input char to string
        inc di                  ; go to next byte at di/cmdString
        int 0x10                ; print input character to screen
        jmp pgm_name_loop       ; loop for next character from user

start_search:
        mov di, cmdString       ; reset di, point to start of command string
        xor bx, bx              ; reset ES:BX, point to beginning of file table

check_next_char:
        mov al, [ES:BX]         ; get file table character
        cmp al, '}'             ; at end of file table?
        je pgm_not_found

        cmp al, [di]            ; does user input match file table character?
        je start_compare

        inc bx                  ; if not, get next char from file table and re-check
        jmp check_next_char

start_compare:
        push bx                 ; save file table position
        mov byte cl, [cmdLength]

compare_loop:
        mov al, [ES:BX]         ; get file table character
        inc bx                  ; next byte in input/filetable
        cmp al, [di]            ; does input match filetable char?
        jne restart_search      ; if not, search again from this point in filetable

        dec cl                  ; if it does match, decrement length counter
        jz found_program        ; counter = 0, all of input found in filetable
        inc di                  ; else go to next byte of input
        jmp compare_loop

restart_search:
        mov di, cmdString       ; reset to start of user input
        pop bx                  ; get saved file table position
        inc bx                  ; go to next char in file table
        jmp check_next_char     ; start checking again

pgm_not_found:
        mov si, notFoundString  ; did not find pgm name in file table
        call print_string
        mov ah, 0x00            ; get keystroke
        int 0x16
        mov ah, 0x0e
        int 0x10
        cmp al, 'Y'
        je filebrowser          ; reload file browser screen to search again
        jmp fileTable_end       ; else go back to main menu

        ;; Get sector number after pgm name in file table
        ;; ----------------------------------------------
found_program:
        inc bx
        mov cl, 10              ; use to get sector number
        xor al, al              ; reset al to 0

next_sector_number:
        mov dl, [ES:BX]         ; checking next byte of file table
        inc bx
        cmp dl, ','             ; at end of sector number?
        je load_program         ; if so, load program from that sector
        cmp dl, 48              ; else, check if al is '0'-'9' in ascii
        jl sector_not_found     ; before '0', not a number
        cmp dl, 57              
        jg sector_not_found     ; after '9', not a number
        sub dl, 48              ; convert ascii char to integer
        mul cl                  ; al * cl (al * 10), result in AH/AL (AX)
        add al, dl              ; al = al + dl
        jmp next_sector_number

sector_not_found:
        mov si, sectNotFound    ; did not find sector or partial pgm name 
        call print_string
        mov ah, 0x00            ; get keystroke, print to screen
        int 0x16
        mov ah, 0x0e
        int 0x10
        cmp al, 'Y'
        je filebrowser          ; reload file browser screen to search again
        jmp fileTable_end       ; else go back to main menu

        ;; Read disk sector of program to memory and execute it by far jumping
        ;; -------------------------------------------------------------------
load_program:
        mov cl, al              ; cl = sector # to start loading/reading at 

        mov ah, 0x00            ; int 13h ah 0 = reset disk system
        mov dl, 0x00            ; disk # 
        int 0x13

        mov ax, 0x8000          ; memory location to load pgm to
        mov es, ax
        xor bx, bx              ; ES:BX <- 0x8000:0x0000

        mov ah, 0x02            ; int 13h ah 2 = read disk sectors to memory
        mov al, 0x01            ; # of sectors to read
        mov ch, 0x00            ; track #
        mov dh, 0x00            ; head #
        mov dl, 0x00            ; drive #

        int 0x13
        jnc pgm_loaded          ; carry flag not set, success

        mov si, notLoaded       ; else error, program did not load correctly
        call print_string
        mov ah, 0x00
        int 0x16
        jmp filebrowser         ; reload file table

pgm_loaded:
        mov ax, 0x8000          ; program loaded, set segment registers to location
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax
        jmp 0x8000:0x0000       ; far jump to program

fileTable_end:
        mov si, goBackMsg
        call print_string       ; show go back message
        mov ah, 0x00            ; get keystroke
        int 0x16
        jmp main_menu           ; go back to main menu

        ;; --------------------------------------------------------------------
        ;; Menu R) - Reboot: far jump to reset vector
        ;; --------------------------------------------------------------------
reboot: 
        jmp 0xFFFF:0x0000

        ;; --------------------------------------------------------------------
        ;; Menu P) - Print Register Values
        ;; --------------------------------------------------------------------
registers_print:
        ;; Reset screen state
        call resetTextScreen 

        mov si, printRegHeading 
        call print_string

        call print_registers

        ;; Go back to main menu
        mov si, goBackMsg
        call print_string
        mov ah, 0x00
        int 0x16                ; get keystroke
        jmp main_menu           ; go back to main menu

        ;; --------------------------------------------------------------------
        ;; Menu G) - Graphics Mode Test(s)
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
        ;; Menu N) - End Program  
        ;; --------------------------------------------------------------------
end_program:
        cli                     ; clear interrupts
        hlt                     ; halt cpu

        ;; ====================================================================
        ;; End Main Logic
        ;; ====================================================================
	
	;; Small routine to convert hex byte to ascii, assume hex digit in AL
print_hex_as_ascii:
	and al, 0xFF
	add al, 0x30		; convert to ascii number
	cmp al, 0x39		; is value 0h-9h or A-F
	jle hexNum
	add al, 0x7			; add hex 7 to get ascii 'A'-'F'
	mov ah, 0x0e
	int 0x10	
hexNum:	
	ret
	
        ;; --------------------------------------------------------------------
        ;; Include Files
        ;; --------------------------------------------------------------------
        include "../print/print_string.asm"
        include "../print/print_hex.asm"
        include "../print/print_registers.asm"
        include "../screen/resetTextScreen.asm"
        include "../screen/resetGraphicsScreen.asm"
      
        ;; --------------------------------------------------------------------
        ;; Variables
        ;; --------------------------------------------------------------------
menuString:     db '---------------------------------',0xA,0xD,\
        'Kernel Booted, Welcome to QuesOS!', 0xA, 0xD,\
        '---------------------------------', 0xA, 0xD, 0xA, 0xD,\
        'F) File & Program Browser/Loader', 0xA, 0xD,\
        'R) Reboot',0xA,0xD,\
        'P) Print Register Values',0xA,0xD,\
        'G) Graphics Mode Test',0xA,0xD,0

success:        db 0xA,0xD,'Program Found!', 0xA,0xD,0
failure:        db 0xA,0xD,'Oops! Something went wrong :(', 0xA,0xD,0
notLoaded:      db 0xA,0xD,'Error! Program Not Loaded, Try Again',0xA,0xD,0

fileTableHeading:   db '---------   ---------   -------   ------------   --------------',\
	0xA,0xD,'File Name   Extension   Entry #   Start Sector   Size (sectors)',\
	0xA,0xD,'---------   ---------   -------   ------------   --------------',\
	0xA,0xD,0
        
printRegHeading:    db '--------  ------------',0xA,0xD,\
        'Register  Mem Location',0xA,0xD,\
        '--------  ------------',0xA,0xD,0

notFoundString: db 0xA,0xD,'program not found!, try again? (Y)',0xA,0xD,0
sectNotFound:   db 0xA,0xD,'sector not found!, try again? (Y)',0xA,0xD,0

cmdLength:      db 0

goBackMsg:      db 0xA,0xD,0xA,0xD,'Press any key to go back...', 0
dbgTest:        db 'Test',0
cmdString:      db ''

        ;; --------------------------------------------------------------------
        ;; Sector Padding magic
        ;; --------------------------------------------------------------------
        times 1536-($-$$) db 0   ; pads out 0s until we reach 512th byte

