;;;
;;; print_char_text_mode: Print a single character to video memory
;;; Parms:
;;;  input 1: char to print
;;;  input 2: row to print to	 (address, not value)
;;;  input 3: column to print to (address, not value)
;;;  output 1: return code in AX
;;;----------------------------------------------
use32
print_char_text_mode:
	push ebp
	mov ebp, esp
	sub esp, 4		; error/return code

	push edi
    push esi
	push ebx
	push edx

    ;; Set up DI row/col to print to
    mov edi, 0B8000h        ; Start of vga color text mode memory
	mov ebx, [ebp+12]	    ; Cursor Y address
	imul si, [ebx], 160		; Row to print to, in bytes
	mov ebx, [ebp+8]		; Cursor X address
	imul ax, [ebx], 2		; Col to print to, in bytes
	add si, ax				; DI = row*sizeof(row) + col*sizeof(col)

    add di, si

	mov al, [ebp+16]		; Char to print (in AL)

	mov ah,	17h				; Colors to print

	cmp al, 0Ah			; Line feed?
	je .LF
	cmp al, 0Dh			; Carriage return?
	je .CR
	stosw				; Print character to screen by storing AL in [ES:DI]

	inc word [ebx]		; Update cursor position
	cmp word [ebx], 80	; at end of line?
	jne .end_print						; No, go on

	mov dx, word [ebx]	; Yes, do a CR 1st, then LF
	shl dx, 1
	sub di, dx	
	mov word [ebx], 0
	
	.LF:
		mov ebx, [ebp+12]						; Cursor Y address
		inc word [ebx]		; Go down 1 row
		cmp word [ebx], 25	; At bottom of screen?
		jge .scroll_down

		add di, 160							; Go down "1 row" in video memory
		mov ebx, [ebp+8]						; Cursor X address
		jmp .end_print

	.CR:
		mov dx, word [ebx]
		shl dx, 1
		sub di, dx					; Move to "beginning of line" in video memory
		mov word [ebx], 0			; Move to beginning of line
		jmp .end_print

	.scroll_down:
		;; Copy screen lines 1-24 into lines 0-23 (0-based),
		;;   then clear out last line, line 24
		;;   and continue printing
		push edi
		push esi
		
        mov edi, 0B8000h
        mov esi, edi
		add esi, 160		; Byte location of screen line 1
		mov cx, 80*24
		rep movsw		; Copy lines 1-24 into lines 0-23
		
		mov ax, 1720h	; blue bg, light gray fg, space
		mov cx, 80		; # of chars in line
		rep stosw		; Clear line 24
		
		pop esi
		pop edi

		dec word [ebx]	; set Y = line 24
		mov ebx, [ebp+8]	; Cursor X address

	.end_print:
	mov dword [ebp-4], 0		; Return code 0 = success
	mov eax, [ebp-4]

	pop edx
	pop ebx
    pop esi
	pop edi

	mov esp, ebp
	pop ebp

	ret
