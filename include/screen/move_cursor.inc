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

	push edx			; save modified registers
	push ebx
    push edi
    push ecx

	;; Get parm off stack
    xor edx, edx
	mov dx, [ebp+8]	     ; DX = column value
    shl edx, 5           ; multiply by 32 - 8px char width * 4 bytes

    xor eax, eax
    xor ebx, ebx

	mov ax, [ebp+12]     ; AX = row value
    shl eax, 4           ; multiply by 16 - # of lines per char
    mov bx, [5010h]      ; VBE mode info block bytes per scanline
    imul eax, ebx        ; byte value for row
    add eax, edx         ; add byte value for column = byte value for cursor

    mov edi, [5028h]     ; VBE mode info block framebuffer address
    add edi, eax         ; offset cursor from framebuffer

    ;; Draw cursor
    mov eax, 127*16     ; cursor character stored at ascii 127
    dec eax             ; cursor only stored in last line
    add eax, 6000h      ; Bitmap font memory address - offset by 'ascii' char value

    imul ebx, 15         ; Go to last row of current character
    add edi, ebx

    mov ecx, 8      ; # of bits to check
    mov bl, [eax]   ; Get next byte of character
    .bit_loop:
        mov edx, ecx
        dec edx
        bt bx, dx
        jc .write_text_color
        mov [edi], dword 000000FFh    ; ARGB text bg color value - blue
        jmp .next_bit

        .write_text_color:
        mov [edi], dword 00EEEEEEh    ; ARGB text fg color value - white

        .next_bit:
        add edi, 4              ; Next pixel position
        dec ecx
    jnz .bit_loop

	mov dword [ebp-4], 0	; return code 0 = success!
	mov eax, [ebp-4]

    pop ecx
    pop edi
	pop ebx
	pop edx

	mov esp, ebp
	pop ebp

	ret
