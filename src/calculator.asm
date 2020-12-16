calc_setup:
	mov byte [calc_drive_number], dl	; Store passed in drive number

	;; Clear screen first
	call clear_screen_text_mode
	mov di, buffer	; Have DI point to start of buffer

	jmp keyloop

;; Include files
include "../include/screen/clear_screen_text_mode.inc"
include "../include/screen/move_cursor.inc"
include "../include/print/print_char_text_mode.inc"
include "../include/print/print_string_text_mode.inc"
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

;; Get line of input
keyloop:
    call get_key    ; Char in AL

	cmp al, 0Dh	; Enter key / Carriage return?
	je start_parse
	cmp al, 1Bh	; Escape key 
	je return_to_kernel
	stosb		; Store char to buffer; [ES:DI] <- AL & inc DI

	push ax		; Otherwise print char to screen
	push word calc_csr_y	
	push word calc_csr_x	
	call print_char_text_mode
	add sp, 6

	push word [calc_csr_y]	; And move cursor forward
	push word [calc_csr_x]
	call move_cursor
	add sp, 4
jmp keyloop	

start_parse:
	mov word [scan], 0
	call parse_sum

	mov word [result], ax

	call print_newline

	;; Print error msg or result
	cmp byte [error], TRUE
	jne .print_num
	mov si, error_msg

	push si					; print error msg
	push word calc_csr_y
	push word calc_csr_x
	call print_string_text_mode
	add sp, 6

	jmp .clear_buffer

	; Else print num as hex
	.print_num:
		mov dx, [result]
		push word calc_csr_y
		push word calc_csr_x
		call print_hex
		add sp, 4

	.clear_buffer:
		mov di, buffer
		xor al, al
		mov cx, 80
		rep stosb
		mov di, buffer			; Reset DI to start of buffer
		mov byte [error], FALSE	; Reset error flag
	
	call print_newline

	jmp keyloop	; Parse next line of input

parse_sum:
	push bp
	mov bp, sp
	sub sp, 4					; Local variable

	call parse_product
	cmp byte [error], TRUE
	je .ret_err

	mov word [bp-4], ax		; Return code from parse_product
	.loop:
		call skip_blanks

		push word '+'
		call match_char
		add sp, 2
		cmp ax, TRUE
		jne .check_minus

		call parse_product
		cmp byte [error], TRUE
		je .ret_err
		add word [bp-4], ax	; num += num2
		jmp .loop

		.check_minus:
			push word '-'
			call match_char
			add sp, 2
			cmp ax, TRUE
			jne .ret_num

			call parse_product
			cmp byte [error], TRUE
			je .ret_err
			sub word [bp-4], ax	; num -= num2
			jmp .loop

	.ret_num:
		mov ax, [bp-4]
		jmp .return

	.ret_err:
		mov byte [error], TRUE

	.return:
		mov sp, bp
		pop bp
		ret

parse_product:
	push bp
	mov bp, sp
	sub sp, 4				; Local variable

	call parse_term
	cmp byte [error], TRUE
	je .ret_err
	mov word [bp-4], ax		; Return code from parse_term

	.loop:
		call skip_blanks

		push word '*'
		call match_char
		add sp, 2
		cmp ax, TRUE
		jne .check_divide

		call parse_term
		cmp byte [error], TRUE
		je .ret_err
		mov bx, word [bp-4]
		imul bx, ax					
		mov word [bp-4], bx	; num *= num2
		jmp .loop

		.check_divide:
			push word '/'
			call match_char
			add sp, 2
			cmp ax, TRUE
			jne .ret_num

			call parse_term
			cmp byte [error], TRUE
			je .ret_err
			mov bx, ax					; bx = num2
			mov ax, [bp-4]				; ax = num1
			xor dx, dx					; clear upper half of dividend
			idiv bx						; num1 / num2; quotient in AX, remainder in DX
			mov word [bp-4], ax			; num1 /= num2
			jmp .loop

	.ret_num:
		mov ax, [bp-4]
		jmp .return

	.ret_err:
		mov byte [error], TRUE

	.return:
		mov sp, bp
		pop bp
		ret

parse_term:
	push bp
	mov bp, sp
	sub sp, 4					; Local variable

	call skip_blanks

	push word '('
	call match_char
	add sp, 2
	cmp ax, TRUE
	jne .check_negative

	call parse_sum
	cmp byte [error], TRUE
	je .ret_err

	mov word [bp-4], ax			; store result to local stack variable

	call skip_blanks
	push word ')'
	call match_char
	add sp, 2
	cmp ax, FALSE
	je .ret_err
	jmp .ret_num	

	.check_negative:
		push word '-'
		call match_char
		add sp, 2
		cmp ax, TRUE
		jne .check_number

		call parse_term				; Recursion, fun!
		mov word [bp-4], ax
		xor ax, ax
		sub ax, [bp-4]				; Negate number (num = 0 - num)
		mov word [bp-4], ax			; Store new num value
		jmp .ret_num

	.check_number:
		call parse_number
		cmp byte [error], TRUE
		je .ret_err
		mov word [bp-4], ax			; store result to local stack variable
		jmp .ret_num
		
	.ret_err:
		mov byte [error], TRUE
		jmp .return

	.ret_num:
		mov ax, word [bp-4]			; Return local variable

	.return:
		mov sp, bp
		pop bp
		ret

skip_blanks:
	.loop:
		push word 20h	; ASCII space (' ')
		call match_char
		add sp, 2		; Clean up stack
		cmp ax, TRUE	; Return code from match_char
	je .loop

	ret

parse_number:
	mov word [number], 0

	push word number
	call is_digit
	add sp, 2
	cmp ax, TRUE
	jne .ret_err
	.loop:
		push word number
		call is_digit
		add sp, 2
		cmp ax, TRUE
	je .loop

	.ret_num:
		mov ax, word [number]
		ret

	.ret_err:
		mov byte [error], TRUE
		ret

is_digit:
	push bp
	mov bp, sp

	push si

	mov si, buffer
	add si, word [scan]			; bx = buffer[scan]
	mov al, [si]				; al <- [si]
	cmp al, '0'
	jl .ret_false
	cmp al, '9'
	jg .ret_false

	xor ch, ch
	mov cl, al					; store buffer[scan] value

	mov bx, [bp+4]				; Get address of passed in number parm
	mov ax, [bx]				; Get value of number
	imul ax, 10					; *num * 10

	sub cl, '0'					; buffer[scan] - '0', convert ascii to int
	add ax, cx					; (*num * 10) + (buffer[scan] - '0')

	mov bx, [bp+4]				; Reset bx to address of number
	mov [bx], ax				; set data at number to new value

	inc word [scan]				; scan++
	mov ax, TRUE
	jmp .return

	.ret_false:
		mov ax, FALSE

	.return:
		pop si
		mov sp, bp
		pop bp
		ret

match_char:
	push bp
	mov bp, sp

	push si

	mov si, buffer
	add si, word [scan]			; si = buffer[scan]
	mov al, [si]				; al <- [si]
	mov bx, [bp+4]
	cmp al, bl					; check if buffer[scan] == passed in char
	jne .ret_false
	inc word [scan]				; scan++
	mov ax, TRUE
	jmp .return

	.ret_false:
		mov ax, FALSE

	.return:
		pop si
		mov sp, bp
		pop bp
		ret

return_to_kernel:
        mov ax, 200h
        mov es, ax
        xor bx, bx             ; ES:BX <- 200h:0000h

		mov dl, [calc_drive_number]
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        jmp 200h:000h				; far jump back to kernel

;; Subroutine to print a newline
print_newline:
	mov ax, 000Ah
	push ax
	push word calc_csr_y
	push word calc_csr_x
	call print_char_text_mode
	add sp, 6
	
	push word [calc_csr_y]
	push word [calc_csr_x]
	call move_cursor
	add sp, 4

	mov ax, 000Dh
	push ax
	push word calc_csr_y
	push word calc_csr_x
	call print_char_text_mode
	add sp, 6
	
	push word [calc_csr_y]
	push word [calc_csr_x]
	call move_cursor
	add sp, 4

	ret

;; Sector padding
times 1536-($-$$) db 0 
