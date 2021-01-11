;;; 
;;; move_cursor.inc: moves hardware cursor to new row/col
;;; assuming VGA txt mode 03 - 80x25 characters, 16 colors
;;; Parms:
;;; input 1: row to move to
;;; input 2: col to move to
;;; output 1: error/return code in AX
use32
move_cursor:
	push ebp
	mov ebp, esp	
	sub esp, 4		; Local stack variable - error/return code

	push edx			; save modified register
	push ebx

	;; Get parm off stack
	mov dx, [ebp+8]	     ; DX = column value
	mov ax, [ebp+12]     ; AX = row value

	;; Calculate cursor bytes
	;; Y pos * size of row + X pos
	imul bx, ax, 80 
	add bx, dx

	;; Move cursor using CRT Microcontroller ports!!
	;; Write the cursor low byte (BL)	
	mov ax, 0Fh		; CRT index register, cursor low byte
	mov dx, 03D4h
	out dx, ax

	mov dx, 03D5h	; CRT data register
    xor ah, ah
	mov al, bl
	out dx, ax

	;; Write the cursor high byte (BH)	
    xor ah, ah
	mov al, 0Eh		; CRT index register, cursor high byte
	mov dx, 03D4h
	out dx, ax

	mov dx, 03D5h	; CRT data register
    xor ah, ah
	mov al, bh
	out dx, ax

	mov dword [ebp-4], 0	; return code 0 = success!
	mov eax, [ebp-4]

	pop ebx
	pop edx

	mov esp, ebp
	pop ebp

	ret
