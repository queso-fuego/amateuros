;;;
;;; print_registers.asm: prints registers and memory addresses to screen
;;;
;;;	input 1: cursor Y position (address, not value)
;;; input 2: cursor X position (address, not value)
use32
print_registers:
    ;; TODO: Change to print out 32 bit registers instead of 16 bit 
	push ebp
	mov ebp, esp

	mov eax, [ebp+12]
	mov dword [csr_y_addr], eax
	mov eax, [ebp+8]
	mov dword [csr_x_addr], eax

	;; Print string
	push dword printRegHeading
	push dword [csr_y_addr]
	push dword [csr_x_addr]
	call print_string
	add esp, 12

    call .move_cursor   ; Move cursor

    mov esi, regString

	;; Print string for DX
	push dword regString
	push dword [csr_y_addr]
	push dword [csr_x_addr]
	call print_string
	add esp, 12

    call .move_cursor   ; Move cursor

	;; Print hex value
	push dword [csr_y_addr]
	push dword [csr_x_addr]
    call print_hex          ; print DX
	add esp, 8

	;; Print string for AX
    mov byte [regString+2], 'a'
	push esi
	push dword [csr_y_addr]
	push dword [csr_x_addr]
	call print_string
	add esp, 12

    call .move_cursor   ; Move cursor

	;; Print hex value
    mov dx, ax
	push dword [csr_y_addr]
	push dword [csr_x_addr]
    call print_hex          ; print AX value
	add esp, 8

	;; Print string for BX
    mov byte [regString+2], 'b'
	push esi
	push dword [csr_y_addr]
	push dword [csr_x_addr]
	call print_string
	add esp, 12

    call .move_cursor   ; Move cursor

	;; Print hex value
    mov dx, bx
	push dword [csr_y_addr]
	push dword [csr_x_addr]
    call print_hex          ; print BX value
	add esp, 8

	;; Print string
    mov byte [regString+2], 'c'
	push esi
	push dword [csr_y_addr]
	push dword [csr_x_addr]
	call print_string
	add esp, 12

    call .move_cursor   ; Move cursor

	;; Print hex value
    mov dx, cx
	push dword [csr_y_addr]
	push dword [csr_x_addr]
    call print_hex          ; print CX value
	add esp, 8

	;; Print string
    mov word [regString+2], 'si'
	push esi
	push dword [csr_y_addr]
	push dword [csr_x_addr]
	call print_string
	add esp, 12

    call .move_cursor   ; Move cursor

	;; Print hex value
    ;; TODO: Save all registers (& flags?) at start of file,
    ;;  then move them into dx to print their original values before
    ;;  this file was called
    mov dx, si
	push dword [csr_y_addr]
	push dword [csr_x_addr]
    call print_hex          ; print SI value
	add esp, 8

	;; Print string
    mov byte [regString+2], 'd'
	push esi
	push dword [csr_y_addr]
	push dword [csr_x_addr]
	call print_string
	add esp, 12

    call .move_cursor   ; Move cursor

	;; Print hex value
    mov dx, di
	push dword [csr_y_addr]
	push dword [csr_x_addr]
    call print_hex          ; print DI value
	add esp, 8

	;; Print string
    mov word [regString+2], 'cs'
	push esi
	push dword [csr_y_addr]
	push dword [csr_x_addr]
	call print_string
	add esp, 12

    call .move_cursor   ; Move cursor

	;; Print hex value
    mov dx, cs
	push dword [csr_y_addr]
	push dword [csr_x_addr]
    call print_hex          ; print CS value
	add esp, 8

	;; Print string
    mov byte [regString+2], 'd'
	push esi
	push dword [csr_y_addr]
	push dword [csr_x_addr]
	call print_string
	add esp, 12

    call .move_cursor   ; Move cursor

	;; Print hex value
    mov dx, ds
	push dword [csr_y_addr]
	push dword [csr_x_addr]
    call print_hex          ; print DS value
	add esp, 8

	;; Print string
    mov byte [regString+2], 'e'
	push esi
	push dword [csr_y_addr]
	push dword [csr_x_addr]
	call print_string
	add esp, 12

    call .move_cursor   ; Move cursor

	;; Print hex value
    mov dx, es
	push dword [csr_y_addr]
	push dword [csr_x_addr]
    call print_hex          ; print ES value
	add esp, 8

	;; Print string
	mov byte [regString+2], 's'
	push esi
	push dword [csr_y_addr]
	push dword [csr_x_addr]
	call print_string
	add esp, 12

    call .move_cursor   ; Move cursor

	;; Print hex value
    mov dx, ss
	push dword [csr_y_addr]
	push dword [csr_x_addr]
    call print_hex          ; print SS value
	add esp, 8

	;; Print char
	push dword 000Ah 
	push dword [csr_y_addr]
	push dword [csr_x_addr]
	call print_char
	add esp, 12

    call .move_cursor   ; Move cursor

	;; Print char
	push dword 000Dh 
	push dword [csr_y_addr]
	push dword [csr_x_addr]
	call print_char
	add esp, 12

    call .move_cursor   ; Move cursor

	mov esp, ebp
	pop ebp

    ret

;; Move cursor subroutine
.move_cursor:
	mov ebx, [csr_y_addr]
	push dword [ebx]			; Cursor Y value
	mov ebx, [csr_x_addr]
	push dword [ebx]			; Cursor X value
	call move_cursor
	add esp, 8
    ret

;; Variables
nl equ 0Ah,0Dh ; CR/LF

printRegHeading:    db nl,'--------  ------------',nl,\
        'Register  Mem Location',nl,\
        '--------  ------------',0

regString:  db 0Ah,0Dh,'dx        ',0 ; hold string of current register name and memory address
csr_x_addr: dd 0
csr_y_addr: dd 0
