;;; =========================================================================== 
;;; Kernel.asm: basic 'kernel' loaded from our bootsector
;;; ===========================================================================
main_menu:
        ;; --------------------------------------------------------------------
        ;; Screen & Menu Set up
        ;; --------------------------------------------------------------------
        ;; Set video mode
        mov ah, 0x00            ; int 0x10/ ah 0x00 = set video mode
        mov al, 0x01            ; 40x25 text mode
        int 0x10

        ;; Change color/ palette
        mov ah, 0x0b
        mov bh, 0x00
        mov bl, 0x01
        int 0x10

        mov si, menuString      ; print menu header & options 
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
        ;; ------------------
        ;; Set video mode
        mov ah, 0x00            ; int 0x10/ ah 0x00 = set video mode
        mov al, 0x01            ; 40x25 text mode
        int 0x10

        ;; Change color/ palette
        mov ah, 0x0b
        mov bh, 0x00
        mov bl, 0x01
        int 0x10
      
        mov si, fileTableHeading
        call print_string

        ;; Load file table string from its memory location (0x1000), print file
        ;;   and program names & sector numbers to screen, for user to choose
        ;; --------------------------------------------------------------------
        xor cx, cx              ; reset counter for # chars in file/pgm name
        mov ax, 0x1000          ; file table location
        mov es, ax              ; ES = 0x1000
        xor bx, bx              ; ES:BX = 0x1000:0x0000 
        mov ah, 0x0e            ; get ready to print to screen

fileTable_Loop:
        inc bx
        mov al, [ES:BX]
        cmp al, '}'             ; at end of file table?
        je stop
        cmp al, '-'             ; at sector number of element
        je sectorNumber_Loop
        cmp al, ','             ; between table elements?
        je next_element
        inc cx
        int 0x10
        jmp fileTable_Loop

sectorNumber_Loop:
        cmp cx, 23
        je fileTable_Loop
        mov al, ' '
        int 0x10
        inc cx
        jmp sectorNumber_Loop

next_element:
        xor cx, cx              ; reset counter
        mov al, 0xA
        int 0x10
        mov al, 0xD
        int 0x10
        mov al, 0xA
        int 0x10
        mov al, 0xD
        int 0x10
        jmp fileTable_Loop

stop:
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
        ;; Set video mode
        mov ah, 0x00            ; int 0x10/ ah 0x00 = set video mode
        mov al, 0x01            ; 40x25 text mode
        int 0x10

        ;; Change color/ palette
        mov ah, 0x0b
        mov bh, 0x00
        mov bl, 0x01
        int 0x10

        call print_registers
        mov si, goBackMsg
        call print_string
        mov ah, 0x00
        int 0x16                ; get keystroke
        jmp main_menu           ; go back to main menu

        ;; --------------------------------------------------------------------
        ;; Menu N) - End Program  
        ;; --------------------------------------------------------------------
end_program:
        cli                     ; clear interrupts
        hlt                     ; halt cpu

        ;; ====================================================================
        ;; End Main Logic
        ;; ====================================================================

        ;; --------------------------------------------------------------------
        ;; Include Files
        ;; --------------------------------------------------------------------
        include "../print/print_string.asm"
        include "../print/print_registers.asm"
      
        ;; --------------------------------------------------------------------
        ;; Variables
        ;; --------------------------------------------------------------------
menuString:     db '---------------------------------',0xA,0xD,\
        'Kernel Booted, Welcome to QuesOS!', 0xA, 0xD,\
        '---------------------------------', 0xA, 0xD, 0xA, 0xD,\
        'F) File & Program Browser/Loader', 0xA, 0xD,\
        'R) Reboot',0xA,0xD,\
        'P) Print Register Values',0xA,0xD,0

success:        db 0xA,0xD,'Command ran successfully!', 0xA,0xD,0
failure:        db 0xA,0xD,'Oops! Something went wrong :(', 0xA,0xD,0

fileTableHeading:   db '------------           ------',0xA,0xD,\
        'File/Program           Sector',0xA,0xD,\
        '------------           ------',0xA,0xD, 0
goBackMsg:      db 0xA,0xD,0xA,0xD,'Press any key to go back...', 0
cmdString:      db ''

        ;; --------------------------------------------------------------------
        ;; Sector Padding magic
        ;; --------------------------------------------------------------------
        times 1024-($-$$) db 0   ; pads out 0s until we reach 512th byte

