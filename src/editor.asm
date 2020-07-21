;;;
;;; editor.asm/bin: text editor with "modes", from hexidecimal monitor to ascii editing
;;;
	;; CONSTANTS
	;; --------------------------
ENDPGM   equ '?'
RUNINPUT equ '$'
SAVEPGM  equ 'S'
CREATENEW equ 'C'
LOADCURR  equ 'L'
BINFILE   equ 'B'
OTHERFILE equ 'O'
VIDMEM   equ 0B800h
LEFTARROW equ 4Bh	; Keyboard scancodes...
RIGHTARROW equ 4Dh
UPARROW equ 48h
DOWNARROW equ 50h
ESC equ 01h
	
	;; LOGIC
	;; -------------------------
reset_editor:	
	;; Clear the screen
	call clear_screen_text_mode

	mov si, new_or_current_string
	call print_string

	xor ah, ah
	int 16h
	cmp al, CREATENEW	; Create new file?
	je create_new_file
	cmp al, LOADCURR	; or load an existing file?
	je load_existing_file

create_new_file:
	call clear_screen_text_mode
	mov si, choose_filetype_string
	call print_string
	xor ah, ah
	int 16h

	mov word [editor_filesize], 0	; Reset file size byte counter
	cmp al, BINFILE
	je new_file_hex
	cmp al, OTHERFILE
	je new_file_text

load_existing_file:
	call print_fileTable
	mov si, choose_file_msg
	call print_string	
	
	;; Restore extra segment
	mov ax, 800h
	mov es, ax

	;; Have user input desired file name to load
	call input_file_name

	;; Load file from input file name
	;; Set up parms to pass
	push word editor_filename	;; 1st parm - file name
	push word 1000h				;; 2nd parm - segment to load to
	push word 0000h				;; 3rd parm - offset to load to
	
	call load_file

	;; Reset stack
	add sp, 6

	;; Check return value/error code in AX
	cmp ax, 0
	jne load_file_error			; Error occcurred, ax return code not 0
	jmp load_file_success		; No error, return to normal

load_file_error:
	mov si, load_file_error_msg
	mov cx, 24
	call write_bottom_screen_message

	xor ax, ax
	int 16h
	call clear_screen_text_mode
	jmp load_existing_file

load_file_success:
	;; Restore extra segment
	mov ax, 800h
	mov es, ax

	;; Get file type from BX
	mov di, editor_filetype
	mov al, [bx]	
	stosb
	mov al, [bx+1]	
	stosb
	mov al, [bx+2]	
	stosb

	;; Go to editor depending on file type
	mov si, bx
	mov di, extBin
	mov cx, 3
	rep cmpsb			; Check if file extension = 'bin'
	je load_file_hex	; If so, load file bytes to screen, and go to hex editor
	jmp load_file_text	; Otherwise, go to general text editor
	
new_file_hex:
	call clear_screen_text_mode
	;; Fill out filetype (.bin)
	mov ax, 800h					; reset es to program location (8000h)
	mov es, ax
	mov di, editor_filetype
	mov al, 'b'
	stosb
	mov al, 'i'
	stosb
	mov al, 'n'
	stosb
	mov si, keybinds_hex_editor
	mov cx, 56			; # of bytes to move
	call fill_out_bottom_editor_message	; Write keybinds & filetype at bottom of screen

	jmp hex_editor
	
load_file_hex:
	call clear_screen_text_mode

	;; Load file bytes to screen
	mov ax, 1000h
	mov es, ax
	xor di, di			; ES:DI <- 1000h:0000h = 10,000h - file location
	mov cx, 512			; TODO: Change to actual file size

	mov ah, 0Eh
	.loop:
		mov al, [ES:DI]		; Read hex byte from file location - 2 nibbles!
		ror al, 4			; Get 1st nibble into al
		and al, 00001111b
		call hex_to_ascii
		int 10h
		mov al, [ES:DI]
		and al, 00001111b	; Get 2nd nibble into al
		call hex_to_ascii
		int 10h
		mov al, ' '
		int 10h
		inc di
	loop .loop

	dec di					; Fix off by one

	mov word [save_di], di	; Save off di first

	; Write keybinds at bottom of screen
	mov si, keybinds_hex_editor
	mov cx, 56			; # of bytes to move
	call fill_out_bottom_editor_message	

	mov ax, 1000h
	mov es, ax			; Reset es to file location (10000h)
	mov di, [save_di]	; Restore di value for file location
	
	jmp get_next_hex_char

new_file_text:
	call clear_screen_text_mode
	;; Fill out filetype (.txt)
	mov ax, 800h					; reset es to program location (8000h)
	mov es, ax
	mov di, editor_filetype
	mov al, 't'
	stosb
	mov al, 'x'
	stosb
	mov al, 't'
	stosb
	mov si, keybinds_text_editor
	mov cx, 50			; # of bytes to move
	call fill_out_bottom_editor_message	; Write keybinds & filetype at bottom of screen

	jmp text_editor

load_file_text:
	call clear_screen_text_mode

	;; Load file bytes to screen
	mov ax, 1000h
	mov es, ax
	xor di, di			; ES:DI <- 1000h:0000h = 10,000h - file location
	mov cx, 512			; TODO: Change to actual file size

	mov ah, 0Eh
	.loop:
		mov al, [ES:DI]		; Read ascii byte from file location
		int 10h
		inc di
	loop .loop

	dec di					; Fix off by one

	mov word [save_di], di	; Save off di first

	; Write keybinds at bottom of screen
	mov si, keybinds_text_editor
	mov cx, 50			; # of bytes to move
	call fill_out_bottom_editor_message	

	mov ax, 1000h
	mov es, ax			; Reset es to file location (10000h)
	mov di, [save_di]	; Restore di value for file location
	
	;; TODO: Put code to format hex code lines so that they all line up the same

text_editor:
	input_char_loop:
		xor ah, ah
		int 16h			; Keyboard scancode in AH, char in AL
		mov byte [save_scancode], ah	
		cmp ah, 0Eh

		;; Check for text editor keybinds
		cmp byte [save_scancode], ESC	; Return to kernel
		je end_editor	

		;; Print out user input character to screen
		xor ah, ah
		push ax					; char to print in AL
		push word [cursor_y]
		push word [cursor_x]
		call print_char_text_mode
		
		add sp, 6

		;; Move cursor 1 character forward
		inc word [cursor_x]
		push word [cursor_y]
		push word [cursor_x]
		call move_cursor

		add sp, 4
		
		;; TODO: Put newline code here (enter key)

		;; TODO: put backspace code here

		;; TODO: put arrow keys code here

	jmp input_char_loop

	;; TODO: Implement saving text file to disk here...

hex_editor:
	;; Take in user input & print to screen
	xor cx, cx			; reset byte counter
	mov ax, 1000h
	mov es, ax
	xor di, di			; ES:DI <- 1000h:0000h = 10,000h
	
	;; Reset cursor x/y
	mov word [cursor_x], 0
	mov word [cursor_y], 0

get_next_hex_char:
	xor ax, ax
	int 16h				; BIOS get keystroke int 16h ah 00h, al <- input char
	mov byte [save_scancode], ah
	mov ah, 0Eh

	;; Check for hex editor keybinds
	cmp al, RUNINPUT	; at end of user input?
	je execute_input
	cmp al, ENDPGM    	; end program, exit back to kernel
	je end_editor
	cmp al, SAVEPGM		; Does user want to save?
	je save_program

	;; Check for backspace
	cmp al, 08h
	jne check_arrow_keys	
	cmp word [cursor_x], 3
	jl get_next_hex_char

	;; Blank out 1st nibble of hex byte
	push word 0020h			; space ' ' in ascii
	push word [cursor_y]	 
	push word [cursor_x]
	call print_char_text_mode

	add sp, 6				; restore stack

	;; Blank out 2nd nibble of hex byte
	push word 0020h			; space ' ' in ascii
	push word [cursor_y]	 
	inc word [cursor_x]		; 2nd nibble of hex byte
	push word [cursor_x]
	call print_char_text_mode

	add sp, 6				; restore stack

	;; Move cursor 1 full hex byte left
	sub word [cursor_x], 4	; cursor_x was on 2nd nibble, so go 1 extra
	push word [cursor_y]	
	push word [cursor_x]
	call move_cursor
	add sp, 4				; Restore stack after call

	mov [ES:DI], byte 00h	; Make current byte 0 in file
	dec	di					; Move file data to previous byte
	jmp get_next_hex_char

	;; Check for arrow keys
	check_arrow_keys:
		cmp byte [save_scancode], LEFTARROW	; Left arrow key
		je left_arrow_pressed
		cmp byte [save_scancode], RIGHTARROW
		je right_arrow_pressed
		cmp byte [save_scancode], UPARROW
		je up_arrow_pressed
		cmp byte [save_scancode], DOWNARROW
		je down_arrow_pressed

		jmp check_valid_hex
	
	left_arrow_pressed:
		cmp word [cursor_x], 3
		jl get_next_hex_char
		sub word [cursor_x], 3
	
		push word [cursor_y]	
		push word [cursor_x]
		call move_cursor
		add sp, 4				; Restore stack after call

		dec	di					; Move file data to previous byte
		jmp get_next_hex_char

	right_arrow_pressed:
		cmp word [cursor_x], 75
		jg get_next_hex_char
		add word [cursor_x], 3

		push word [cursor_y]	
		push word [cursor_x]
		call move_cursor
		add sp, 4				; Restore stack after call

		inc	di					; Move file data to next byte
		jmp get_next_hex_char

	up_arrow_pressed:
	; TODO:

	down_arrow_pressed:
	; TODO:

	;; Check for valid hex digits
	check_valid_hex:
		cmp al, '0'
		jl get_next_hex_char	; skip input character
		cmp al, '9'
		jg check_if_athruf
		jmp convert_input	; continue on 

	check_if_AthruF:
		cmp al, 'A'
		jl get_next_hex_char
		cmp al, 'F'
		jg check_if_athruf
		jmp convert_input

	check_if_athruf:
		cmp al, 'a'
		jl get_next_hex_char
		cmp al, 'f'
		jg get_next_hex_char
		sub al, 20h			; Convert lowercase to uppercase

	convert_input:
		int 10h				; print out input character BIOS int 10h ah 0Eh, al = char
		inc word [cursor_x]
		call ascii_to_hex  	; else convert al to hexidecimal first
		inc cx				; increment byte counter
		cmp cx, 2			;; 2 ascii bytes = 1 hex byte
		je put_hex_byte
		mov [hex_byte], al	; put input into hex byte memory area
	
return_from_hex: jmp get_next_hex_char
	
	;; When done with input, convert to valid machine code (hex) & run	
execute_input:
	mov di, word [editor_filesize]	; Put current end of file into di
	mov byte [ES:DI], 0CBh ; CB hex = far return x86 instruction, to get back to prompt after running code
	xor di, di			; Reset di/hex memory location 10,000h
	call 1000h:0000h	; jump to hex code memory location to run

	jmp hex_editor		; reset for next hex input
	
put_hex_byte:
	rol byte [hex_byte], 4	; move digit 4 bits to the left, make room for 2nd digit
	or byte [hex_byte], al	; move 2nd ascii byte/hex digit into memory
	mov al, [hex_byte]
	stosb			; put hex byte(2 hex digits) into 10000h memory area, and inc di/point to next byte
	inc word [editor_filesize]  ; Increment file size byte counter
	xor cx, cx		; reset byte counter
	mov al, ' '		; print space to screen
	int 10h
	inc word [cursor_x]
	jmp return_from_hex

ascii_to_hex:
	cmp al, '9'		; is input ascii '0'-'9'?
	jle get_hex_num
	sub al, 37h		; else input is ascii 'A'-'F', get hex A-F/decimal 10-15
return_from_hex_num: ret	
	
get_hex_num:	
	sub al, 30h		; ascii '0' = 30h, 30 - 30 = 0
	jmp return_from_hex_num
	
save_program:
	;; Fill out rest of sector with data if not already
	;;   Divide file size by 512 to get # of sectors filled out and remainder of sector not filled out
	;;     Set up DX:AX for division by word value (512)
	xor dx, dx
	mov ax, word [editor_filesize]	
	mov bx, 512
	div bx							; AX = Quotient, DX = Remainder
	cmp ax, 0						; Less than 512 filled out?
	je check_if_1_sector
	imul cx, ax, 512				; Otherwise get how many sectors already filled - bytes 
	cmp dx, 0
	je enter_file_name				; No need to fill out rest of sector, good to go :)
	jmp fill_out_1_sector			; Else fill out rest of sector with 0s

	check_if_1_sector:
		cmp dx, 0
		jne fill_out_1_sector
		;; Otherwise file is empty, fill out 1 whole sector	
		mov cx, 512
		xor al, al
		rep stosb
		jmp enter_file_name

	fill_out_1_sector:
		mov cx, 512
		sub cx, dx
		xor al, al
		rep stosb

	;; Have user enter file name for new file
enter_file_name:
	call clear_screen_text_mode	
	mov si, filename_string
	call print_string

	;; Restore extra segment
	mov ax, 800h
	mov es, ax

	;; Save file type
	mov di, editor_filetype
	mov al, 'b'
	stosb
	mov al, 'i'
	stosb
	mov al, 'n'
	stosb

	;; Have user input desired file name to save
	call input_file_name

	;; Call save_file function
	push word editor_filename	; 1st parm, file name
	push word editor_filetype	; 2nd parm, file type
	push word 0001h				; 3rd parm, file size (in hex sectors)
	push word 1000h				; 4th parm, segment memory address for file
	push word 0000h				; 5th parm, offset memory address for file
	
	call save_file

	add sp, 10					; Restore stack pointer after returning
	cmp ax, 0
	jne save_file_error			; Error occcurred, ax return code not 0
	jmp save_file_success		; No error, return to normal

save_file_error:
	mov si, save_file_error_msg
	mov cx, 24
	call write_bottom_screen_message

	xor ax, ax
	int 16h

save_file_success:
	call clear_screen_text_mode			; Clear screen on success
	mov si, keybinds_hex_editor			; Write normal keybinds string
	mov cx, 56
	call write_bottom_screen_message
	jmp get_next_hex_char				; Return to normal hex editing

;; Subroutine: write message at bottom of screen
write_bottom_screen_message:
	;; Message to write in SI, length of message in CX
	mov ax, VIDMEM
	mov es, ax			
	mov word di, 0F00h	; ES:DI <- 0B8000h:80*2*24
	
	mov al, byte [text_color]
	.loop:
		movsb			; mov [di], [si] and increment both; store character
		stosb			; Store character attribute byte (text colors)
	loop .loop
	ret

;; Subroutine: have user input a file name
input_file_name:
	;; Save file name
	mov di, editor_filename
	mov cx, 10
	.input_filename_loop:
		xor ah, ah		; Get keystroke
		int 16h
		stosb			; Store character to filename variable
		mov ah, 0Eh
		int 10h			; Print out character to screen
	loop .input_filename_loop
	ret

;; Subroutine: fill out message at bottom of editor
fill_out_bottom_editor_message:
	; Fill string variable with message to write
	mov ax, 800h
	mov es, ax
	mov di, bottom_editor_msg
	rep movsb
	mov al, ' '			; append filetype (extension) to string
	stosb
	stosb
	mov al, '['
	stosb
	mov al, '.'
	stosb
	mov si, editor_filetype
	mov cx, 3
	rep movsb
	mov al, ']'
	stosb
	
	mov si, bottom_editor_msg
	mov cx, 80
	call write_bottom_screen_message
	ret					; return to caller

;; End program
end_editor:	
        mov ax, 200h
        mov es, ax
        xor bx, bx              ; ES:BX <- 2000h:0000h

        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        jmp 200h:0000h			; far jump back to kernel

	;; include files
	include "../include/screen/clear_screen_text_mode.inc"
	include "../include/screen/move_cursor.inc"
	include "../include/print/print_string.inc"
	include "../include/print/print_fileTable.inc"
	include "../include/print/print_char_text_mode.inc"
	include "../include/disk/save_file.inc"
	include "../include/disk/load_file.inc"
	include "../include/type_conversions/hex_to_ascii.inc"
	
	;; VARIABLES
	;; --------------------------
bottom_editor_msg:	times 80 db 0 
keybinds_hex_editor: db  ' $ = Run code ? = Return to kernel S = Save file to disk'
;; 56 length
keybinds_text_editor: db  ' Esc = Return to kernel Ctrl-S = Save file to disk'
;; 53 length
new_or_current_string: db '(C)reate new file or (L)oad existing file?', 0
;; 41 length
choose_filetype_string: db '(B)inary/hex file or (O)ther file type (.txt, etc)?', 0
;; 51 length
filename_string: db 'Enter file name: ', 0
save_file_error_msg: db 'Save file error occurred'	; length of string = 24
choose_file_msg: db 'File to load: ', 0
editor_filename: times 10 db 0
editor_filetype: times 3 db 0
editor_filesize: dw 0
extBin: db 'bin'
load_file_error_msg: db 'Load file error occurred' ; length of string = 24
hex_byte:	db 00h		; 1 byte/2 hex digits
text_color: db 17h
save_di: dw 0
save_scancode: db 0
cursor_x: dw 0
cursor_y: dw 0

	;; Sector padding
	times 2560-($-$$) db 0
