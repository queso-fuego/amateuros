;;;
;;; Prints hexidecimal values using register DX and print_string.asm
;;;
;;; input 1: cursor Y position
;;; input 2: cursor X position
;;; output 1: new cursor Y position in BX
;;; output 2: new cursor X position in CX
;;;
;;; Ascii '0'-'9' = hex 30h-39h
;;; Ascii 'A'-'F' = hex 41h-46h
;;; Ascii 'a'-'f' = hex 61h-66h
;;;
print_hex:
        push bp
		mov bp, sp

		push si

		mov ax, [bp+6]
		mov word [print_hex_cursor_y], ax
		mov ax, [bp+4]	
		mov word [print_hex_cursor_x], ax

        xor cx, cx      ; initialize loop counter 

hex_loop:
        cmp cx, 4       ; are we at end of loop?
        je end_hexloop

        ;; Convert DX hex values to ascii
        mov ax, dx
        and ax, 000Fh   ; turn 1st 3 hex to 0, keep final digit to convert
        add al, 30h    ; get ascii number value by default
        cmp al, 39h    ; is hex value 0-9 (<= 39h) or A-F (> 39h)?
        jle move_intoBX
        add al, 07h    ; to get ascii 'A'-'F'

;; Move ascii char into bx string
move_intoBX:
        mov bx, hexString + 5   ; base address of hexString + length of string
        sub bx, cx      ; subtract loop counter
        mov [bx], al
        ror dx, 4       ; rotate right by 4 bits,
                        ; 12ABh -> 0B12Ah -> 0AB12h -> 2AB1h -> 12ABh
        add cx, 1       ; increment counter   
        jmp hex_loop    ; loop for next hex digit in DX
        
end_hexloop:
		;; Print hex string
        mov si, hexString
		push si
		push word [print_hex_cursor_y]
		push word [print_hex_cursor_x]
        call print_string_text_mode
		add sp, 6

		; Move cursor
		mov word [print_hex_cursor_y], bx
		mov word [print_hex_cursor_x], cx
		push bx
		push cx
		call move_cursor
		add sp, 4

		;; Reminder: New cursor X/Y is in BX/CX!

		pop si

		mov sp, bp
		pop bp

        ret             ; return to caller

;; VARIABLES
hexString:      db '0x0000', 0
print_hex_cursor_y: dw 0
print_hex_cursor_x: dw 0
