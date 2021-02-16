;;;
;;; calculator.asm: basic 4 function calculator (+,-,*,/), with nesting ()
;;;
use32
org 8000h 
calc_setup:
	mov byte [calc_drive_number], dl	; Store passed in drive number

	;; Clear screen first
	call clear_screen

    mov edi, buffer 
	jmp keyloop

;; DATA AREA ----------------------------------------
;; Include files
include "../include/screen/clear_screen.inc"
include "../include/screen/move_cursor.inc"
include "../include/print/print_char.inc"
include "../include/print/print_string.inc"
include "../include/print/print_hex.inc"
include "../include/keyboard/get_key.inc"

;; Constants
TRUE equ 1
FALSE equ 0

;; Variables
number: dw 0
scan: dw 0
buffer: times 80 db 0
error_msg: db 'Syntax Error',0
error: db 0
calc_drive_number: db 0
calc_csr_x: dw 0
calc_csr_y: dw 0
result: dw 0			
valid_input: db '0123456789+-*/()',20h,0Dh,1Bh
.len: db ($ - valid_input)        

;; LOGIC ----------------------------------------
;; Get line of input
keyloop:
    call get_key    ; Get char in AL

    push edi                    ; Save buffer position
    mov edi, valid_input
    xor ecx, ecx
    mov cl, [valid_input.len]
    repne scasb                 ; Repeat while AL != [DI] & inc DI
    pop edi                     ; Restore buffer position
    jne keyloop

	cmp al, 0Dh	; Enter key / Carriage return?
	je start_parse
	cmp al, 1Bh	; Escape key 
	je return_to_kernel
	stosb		; Store char to buffer; [ES:DI] <- AL & inc DI

	push eax		; Otherwise print char to screen
	push dword calc_csr_y	
	push dword calc_csr_x	
	call print_char
	add esp, 12

	push dword [calc_csr_y]	; And move cursor forward
	push dword [calc_csr_x]
	call move_cursor
	add esp, 8
jmp keyloop	

start_parse:
    cmp byte [buffer], 0  ; If nothing entered to buffer, return to get input
    je keyloop           

	mov word [scan], 0
	call parse_sum

	mov word [result], ax

	call print_newline

	;; Print error msg or result
	cmp byte [error], TRUE
	jne .print_num

	push dword error_msg			; print error msg
	push dword calc_csr_y
	push dword calc_csr_x
	call print_string
	add esp, 12

	jmp .clear_buffer

	; Else print num as hex
	.print_num:
		mov dx, [result]
		push dword calc_csr_y
		push dword calc_csr_x
		call print_hex
		add esp, 8

	.clear_buffer:
		mov edi, buffer
		xor al, al
		mov cx, 80
		rep stosb
		mov edi, buffer			; Reset DI to start of buffer
		mov byte [error], FALSE	; Reset error flag
	
	call print_newline

	jmp keyloop	; Parse next line of input

parse_sum:
	push ebp
	mov ebp, esp
	sub esp, 4					; Local variable

	call parse_product
	cmp byte [error], TRUE
	je .ret_err

	mov dword [ebp-4], eax		; Return code from parse_product
	.loop:
		call skip_blanks

		push dword '+'
		call match_char
		add esp, 4
		cmp ax, TRUE
		jne .check_minus

		call parse_product
		cmp byte [error], TRUE
		je .ret_err
		add dword [ebp-4], eax	; num += num2
		jmp .loop

		.check_minus:
			push dword '-'
			call match_char
			add esp, 4
			cmp ax, TRUE
			jne .ret_num

			call parse_product
			cmp byte [error], TRUE
			je .ret_err
			sub dword [ebp-4], eax	; num -= num2
			jmp .loop

	.ret_num:
		mov eax, [ebp-4]
		jmp .return

	.ret_err:
		mov byte [error], TRUE

	.return:
		mov esp, ebp
		pop ebp
		ret

parse_product:
	push ebp
	mov ebp, esp
	sub esp, 4				; Local variable

	call parse_term
	cmp byte [error], TRUE
	je .ret_err
	mov dword [ebp-4], eax		; Return code from parse_term

	.loop:
		call skip_blanks

		push dword '*'
		call match_char
		add esp, 4
		cmp ax, TRUE
		jne .check_divide

		call parse_term
		cmp byte [error], TRUE
		je .ret_err
		mov ebx, dword [ebp-4]
		imul bx, ax					
		mov dword [ebp-4], ebx	; num *= num2
		jmp .loop

		.check_divide:
			push dword '/'
			call match_char
			add esp, 4
			cmp ax, TRUE
			jne .ret_num

			call parse_term
			cmp byte [error], TRUE
			je .ret_err
			mov bx, ax					; bx = num2
			mov eax, [ebp-4]			; ax = num1
			xor dx, dx					; clear upper half of dividend
			idiv bx						; num1 / num2; quotient in AX, remainder in DX
			mov dword [ebp-4], eax		; num1 /= num2
			jmp .loop

	.ret_num:
		mov eax, [ebp-4]
		jmp .return

	.ret_err:
		mov byte [error], TRUE

	.return:
		mov esp, ebp
		pop ebp
		ret

parse_term:
	push ebp
	mov ebp, esp
	sub esp, 4					; Local variable

	call skip_blanks

	push dword '('
	call match_char
	add esp, 4
	cmp ax, TRUE
	jne .check_negative

	call parse_sum
	cmp byte [error], TRUE
	je .ret_err

	mov dword [ebp-4], eax			; store result to local stack variable

	call skip_blanks
	push dword ')'
	call match_char
	add esp, 4
	cmp ax, FALSE
	je .ret_err
	jmp .ret_num	

	.check_negative:
		push dword '-'
		call match_char
		add esp, 4
		cmp ax, TRUE
		jne .check_number

		call parse_term				; Recursion, fun!
		mov dword [ebp-4], eax
		xor ax, ax
		sub eax, [ebp-4]				; Negate number (num = 0 - num)
		mov dword [ebp-4], eax			; Store new num value
		jmp .ret_num

	.check_number:
		call parse_number
		cmp byte [error], TRUE
		je .ret_err
		mov dword [ebp-4], eax			; store result to local stack variable
		jmp .ret_num
		
	.ret_err:
		mov byte [error], TRUE
		jmp .return

	.ret_num:
		mov eax, dword [ebp-4]			; Return local variable

	.return:
		mov esp, ebp
		pop ebp
		ret

skip_blanks:
	.loop:
		push dword 20h	; ASCII space (' ')
		call match_char
		add esp, 4		; Clean up stack
		cmp ax, TRUE	; Return code from match_char
	je .loop

	ret

parse_number:
	mov word [number], 0

	push dword number
	call is_digit
	add esp, 4

	cmp ax, TRUE
	jne .ret_err
	.loop:
		push dword number
		call is_digit
		add esp, 4
		cmp ax, TRUE
	je .loop

	.ret_num:
		mov ax, word [number]
		ret

	.ret_err:
		mov byte [error], TRUE
		ret

is_digit:
	push ebp
	mov ebp, esp

	push esi

	mov esi, buffer
	add si, word [scan]			; bx = buffer[scan]
	mov al, [si]				; al <- [si]
	cmp al, '0'
	jl .ret_false
	cmp al, '9'
	jg .ret_false

	xor ch, ch
	mov cl, al					; store buffer[scan] value

	mov ebx, [ebp+8]			; Get address of passed in number parm
	mov ax, [ebx]				; Get value of number
	imul ax, 10					; *num * 10

	sub cl, '0'					; buffer[scan] - '0', convert ascii to int
	add ax, cx					; (*num * 10) + (buffer[scan] - '0')

	mov ebx, [ebp+8]			; Reset bx to address of number
	mov [ebx], ax				; set data at number to new value

	inc word [scan]				; scan++
	mov ax, TRUE
	jmp .return

	.ret_false:
		mov ax, FALSE

	.return:
		pop esi

		mov esp, ebp
		pop ebp

		ret

match_char:
	push ebp
	mov ebp, esp

	push esi

	mov esi, buffer
	add si, word [scan]			; si = buffer[scan]
	mov al, [si]				; al <- [si]
	mov bl, [ebp+8]             
	cmp al, bl					; check if buffer[scan] == passed in char
	jne .ret_false
	inc word [scan]				; scan++
	mov ax, TRUE
	jmp .return

	.ret_false:
		mov ax, FALSE

	.return:
		pop esi

		mov esp, ebp
		pop ebp
		ret

return_to_kernel:
		mov dl, [calc_drive_number]

        ;; Reset buffer 
		mov edi, buffer
		xor al, al
		mov cx, 80
		rep stosb

        jmp 2000h				    ; Jump back to kernel

;; Subroutine to print a newline
print_newline:
	push dword 000Ah
	push dword calc_csr_y
	push dword calc_csr_x
	call print_char
	add esp, 12
	
	push dword 000Dh
	push dword calc_csr_y
	push dword calc_csr_x
	call print_char
	add esp, 12

    push dword [calc_csr_y]
    push dword [calc_csr_x]
    call move_cursor
    add esp, 8
	
	ret

;; Sector padding
times 2560-($-$$) db 0 
