;;;
;;; print_char: Print a single character to video memory
;;; Parms:
;;;  input 1: char to print
;;;  input 2: row to print to	 (address, not value)
;;;  input 3: column to print to (address, not value)
;;;  output 1: return code in AX
;;;----------------------------------------------
use32
print_char:
	push ebp
	mov ebp, esp
	sub esp, 4		; error/return code

	push edi
    push esi
	push ebx
	push edx
    push ecx        

    ;; Set up EDI with row/col to print to
    mov edi, [5028h]        ; VBE mode info block + 40 bytes = framebuffer address
    xor eax, eax
    mov ax, [5010h]         ; VBE mode info block + 16 bytes = bytes per scanline

	mov ebx, [ebp+12]		; Cursor Y address
    mov bx,  [ebx]          ; Cursor Y value
    and ebx, 0000FFFFh      ; blank out upper 16 bits
    shl ebx, 4              ; Multiply by 16 - # of lines per character
    imul ebx, eax           ; multiply by bytes per line
    mov esi, ebx            ; Row to print to, in bytes

	mov ebx, [ebp+8]		; Cursor X address
    mov bx, [ebx]           ; Cursor X value
    and ebx, 0000FFFFh      ; blank out upper 16 bits
    ; Width of a text character equals:
    ; text size 8x16 = 8 pixels * 4 bytes per pixel = 32bytes
    shl ebx, 5              ; Col to print to, in bytes
    add esi, ebx            ; screen position in bytes to write text to

    add edi, esi            ; EDI = Offset into screen framebuffer

    xor eax, eax
	mov al, [ebp+16]		; Char to print (in AL)

	cmp al, 0Ah			; Line feed?
	je .LF
	cmp al, 0Dh			; Carriage return?
	je .CR

    ; Print character
    shl eax, 4          ; Multiply by 16 - length of stored char in bytes
    sub eax, 16         ; Go to start of char
    add eax, 6000h      ; Bitmap font memory address - offset by ascii value
    push dword 16       ; Height of character in lines
    .char_loop:
        mov ecx, 8      ; # of bits to check
        mov bl, [eax]   ; Get next byte of character
        inc eax         ; move to next byte
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

            pop ecx     ; # of lines left to write
            dec ecx
            jz .inc_cursor
            xor edx, edx
            mov dx, [5010h]  ; bytes per scanline
            add edi, edx     ; Go down 1 line on screen   
            sub edi, 32      ; move back 1 char width to line up
            push ecx         
     jmp .char_loop  

    .inc_cursor:
    mov ebx, [ebp+8]    ; Cursor X address

    inc word [ebx]		; Update cursor position
    cmp word [ebx], 80	; at end of line?
    jne .end_print	    ; No, go on

    xor edx, edx
    mov dx, word [ebx]	; Yes, do a CR 1st, then LF
    shl dx, 5           ; multiply by 32 - 8px char width * 4 bytes
    sub edi, edx        ; Go to start of line
    mov word [ebx], 0   ; New cursor X position = 0

	.LF:
		mov ebx, [ebp+12]						; Cursor Y address
		inc word [ebx]		; Go down 1 row
        cmp word [ebx], 66	; At bottom of screen? (66 * 16 = 1080)
		jge .scroll_down

        xor eax, eax
        mov ax, [5010h]         ; bytes per scanline
        shl eax, 4              ; multiply by 16 - char height in lines
        add edi, eax            ; go down 1 char row
        mov ebx, [ebp+8]	    ; Cursor X address
        jmp .end_print

	.CR:
        mov ebx, [ebp+8]    ; Cursor X address
        xor edx, edx
        mov dx, word [ebx]
        shl dx, 5           ; multiply by 32 - 8px char width * 4 bytes
        sub edi, edx
        mov word [ebx], 0
		jmp .end_print

	.scroll_down:
		;; Copy screen lines 1-24 into lines 0-23 (0-based),
		;;   then clear out last line, line 24
		;;   and continue printing
		push edi
		push esi
		
        mov edi, [5028h]    ; VBE mode info block + 40 bytes = framebuffer address
        mov esi, edi
        xor eax, eax
        mov ax, [5010h]     ; bytes per scanline
        shl eax, 4          ; multiply by 16 - character height in lines
        add esi, eax 		; Byte location of screen line 1

        ; 1080 lines - 16 lines character height = 1064 lines
        ; 1064 * bytes per scanline (1920 pixels * 4 bytes per pixel = 7680 bytes)
        ; 1064 * 7680 = 8171520 or 7CB000h bytes
        ; 8171520 / 4 = 2042880 or 1F2C00h dbl words
        mov ecx, 1F2C00h
        rep movsd       ; Copy char height lines offset by 1 from esi to edi

        ; EDI pointing at last character line (last 16 lines)
        ; 1920px per line * 16 lines = 30720px * 4 bytes per px = 122880 bytes
        ; 30720 or 7800h dbl words
        mov eax, 000000FFh      ; Pixel color - blue
        mov ecx, 7800h
        rep stosd		        ; Clear last line
		
		pop esi
		pop edi

		dec word [ebx]	; set Y = line 24
		mov ebx, [ebp+8]	; Cursor X address

	.end_print:
	mov dword [ebp-4], 0		; Return code 0 = success
	mov eax, [ebp-4]

    pop ecx    
	pop edx
	pop ebx
    pop esi
	pop edi

	mov esp, ebp
	pop ebp

	ret
