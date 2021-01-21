;;;
;;; get_key.inc: Returns an ascii character translated from a scancode from PS/2 controller 
;;;                data port 60h
;;; Output 1: Ascii char in AL
;;;
use32
get_key:
    jmp next_scancode

;; VARIABLES --------------
;; Scancode -> Ascii lookup table
scancode_to_ascii: db 0,1Bh,'1','2','3','4','5','6','7','8','9','0','-','=',08h,\
0,'q','w','e','r','t','y','u','i','o','p','[',']',0Dh,\
1Dh,'a','s','d','f','g','h','j','k','l',';',"'",'`',0,'\',\
'z','x','c','v','b','n','m',',','.','/',0,0,0,' '

;; Shift key pressed on number row lookup table (0-9 keys)
num_row_shifts: db ')','!','@','#','$','%','^','&','*','('

key:			db 0	; Current key pressed
oldKey:			db 0	; Previous key pressed
left_shift:		db 0	; Left shift key pressed y/n
left_ctrl:      db 0    ; Left control key pressed y/n
scancode:       db 0

;; LOGIC --------------
next_scancode:
	mov al, [key]			
	mov byte [oldKey], al	; oldKey = key

	in al, 60h				; Get keyboard scancode from data port 

    mov byte [scancode], al 

	mov byte [key], al		
	cmp al, [oldKey]		; Does new key = old key?
	je next_scancode		; Yes, skip
	cmp al, 2Ah				; Left shift make code
	je left_shift_pressed
	cmp al, 0AAh			; Left shift break code
	je left_shift_released
    cmp al, 1Dh             ; Left Ctrl make code
    je left_ctrl_pressed
    cmp al, 9Dh             ; Left Ctrl break code
    je left_ctrl_released   
	test al, 80h			; is it a break code? Bit 7 set
	jne next_scancode		; Yes, skip

    cmp al, 53h             ; Current end of scancode lookup table NEW - Delete key is new endpoint for (text) editor keys
    jg next_scancode        ; Beyond lookup table, keep going

	mov ebx, scancode_to_ascii	
    xlatb                       ; AL = data at DS:EBX + AL

	cmp byte [left_shift], 1	; Is shift pressed?
	jne return  				; No, return

	cmp al, '0'					; Yes, check which key shift is modifying
	jl check_other_chars
	cmp al, '9'
	jg check_alpha
	sub al, 30h					; key is a number, convert ascii to integer value
	mov ebx, num_row_shifts		; and get the shifted key code value
	xlatb						; AL = DS:BX + AL
	jmp return

check_alpha:
	cmp al, 'a'
	jl check_other_chars        ; Maybe it's a '+' or '"' or '?' or other things
	cmp al, 'z'
	jg check_other_chars
	sub al, 20h				    ; Convert lowercase letters to uppercase
    jmp return

left_shift_pressed:
	mov byte [left_shift], 1
	jmp next_scancode

left_shift_released:
	mov byte [left_shift], 0
	jmp next_scancode

left_ctrl_pressed:
	mov byte [left_ctrl], 1
	jmp next_scancode

left_ctrl_released:
	mov byte [left_ctrl], 0
	jmp next_scancode

check_other_chars:
    mov bx, '+'
    cmp al, '='
    cmove ax, bx    ; AL = '+' if true
    
    mov bx, '?'
    cmp al, '/'
    cmove ax, bx    ; AL = '?' if true

    ;; TODO: Get next key if invalid?

return:
    ;; Character should be in AL at this point 
    mov ah, [scancode]  

    ;; Move shift or control status to bh
    xor bh, bh                  ; Initialize to 0 for neither key pressed
    cmp byte [left_shift], 1
    je .shift_pressed
    cmp byte [left_ctrl], 1
    je .ctrl_pressed
    jmp .done   
 
    .shift_pressed:
        mov bh, 01h
        je .done

    .ctrl_pressed:
        mov bh, 02h
    
    .done:
    ;; Reset shift status just in case
    mov byte [left_shift], 0 
    ;; Reset ctrl status just in case
    mov byte [left_ctrl], 0 

    ret
