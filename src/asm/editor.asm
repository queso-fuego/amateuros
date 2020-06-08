;;;
;;; editor.asm/bin: text editor with "modes", from hexidecimal monitor to ascii editing
;;;
	;; CONSTANTS
	;; --------------------------
ENDPGM   equ '?'
ENDINPUT equ '$'
VIDMEM   equ 0B800h
	
	;; LOGIC
	;; -------------------------
reset_editor:	
	;; Clear the screen
	call clear_screen_text_mode

	;; Write keybinds at bottom of screen
	mov ax, VIDMEM
	mov es, ax			
	mov word di, 0F00h	; ES:DI <- 0B800h:80*2*24
	
	mov si, keybinds	
	mov cx, 33			; # of bytes to move
	cld					; clear direction flag (increment operands)

	rep movsw			; mov [di], [si] and increment both until cx = 0

	;; Restore extra segment
	mov ax, 8000h
	mov es, ax

	;; Take in user input & print to screen
	xor cx, cx			; reset byte counter
	mov di, hex_code	; di points to memory address of hex code
	
get_next_hex_char:
	xor ax, ax
	int 16h			; BIOS get keystroke int 16h ah 00h, al <- input char
	mov ah, 0Eh
	cmp al, ENDINPUT	; at end of user input?
	je execute_input
	cmp al, ENDPGM	    	; end program, exit back to kernel
	je end_editor
	int 10h			; print out input character BIOS int 10h ah 0Eh, al = char
	call ascii_to_hex   	; else convert al to hexidecimal first
	inc cx			; increment byte counter
	cmp cx, 2		; 2 ascii bytes = 1 hex byte
	je put_hex_byte
	mov [hex_byte], al	; put input into hex byte memory area
	
return_from_hex: jmp get_next_hex_char
	
	;; When done with input, convert to valid machine code (hex) & run	
execute_input:
	mov byte [di], 0C3h ; C3 hex = near return x86 instruction, to get back to prompt after running code
	jmp hex_code		; jump to hex code memory location to run

	jmp reset_editor	; reset for next hex input
	
put_hex_byte:
	rol byte [hex_byte], 4	; move digit 4 bits to the left, make room for 2nd digit
	or byte [hex_byte], al	; move 2nd ascii byte/hex digit into memory
	mov al, [hex_byte]
	stosb			; put hex byte(2 hex digits) into hex code memory area, and inc di/point to next byte
	xor cx, cx		; reset byte counter
	mov al, ' '		; print space to screen
	int 10h
	jmp return_from_hex

ascii_to_hex:
	cmp al, '9'		; is input ascii '0'-'9'?
	jle get_hex_num
	sub al, 37h		; else input is ascii 'A'-'F', get hex A-F/decimal 10-15
return_from_hex_num: ret	
	
get_hex_num:	
	sub al, 30h		; ascii '0' = 30h, 30 - 30 = 0
	jmp return_from_hex_num
	
end_editor:	
        mov ax, 2000h
        mov es, ax
        xor bx, bx              ; ES:BX <- 0x2000:0x0000

        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        jmp 2000h:0000h	       ; far jump back to kernel

	;; include files
	include "../screen/clear_screen_text_mode.asm"
	include "../print/print_string.asm"
	
	;; VARIABLES
	;; --------------------------
testString:	db 'Testing',0
keybinds:	db '$',17h,' ',17h,'=',17h,' ',17h,'R',17h,'u',17h,'n',17h,' ',17h,'c',17h,'o',17h,'d',17h,'e',17h,\
' ',17h,'?',17h,' ',17h,'=',17h,' ',17h,'R',17h,'e',17h,'t',17h,'u',17h,'r',17h,'n',17h,' ',17h,\
't',17h,'o',17h,' ',17h,'k',17h,'e',17h,'r',17h,'n',17h,'e',17h,'l',17h
hex_byte:	db 00h		; 1 byte/2 hex digits
hex_code:	times 255 db 00h

	;; Sector padding
	times 1024-($-$$) db 0
