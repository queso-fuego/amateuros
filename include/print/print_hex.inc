;;;
;;; Prints hexidecimal values using register DX and print_string.asm
;;;
;;; input 1: cursor Y position	(address, not value)
;;; input 2: cursor X position	(address, not value)
;;;
;;; Ascii '0'-'9' = hex 30h-39h
;;; Ascii 'A'-'F' = hex 41h-46h
;;; Ascii 'a'-'f' = hex 61h-66h
;;;
use32
print_hex:
        push ebp
		mov ebp, esp

		push esi

        xor ecx, ecx      ; initialize loop counter 

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
        mov ebx, hexString + 5   ; base address of hexString + length of string
        sub ebx, ecx      ; subtract loop counter
        mov [ebx], al
        ror dx, 4       ; rotate right by 4 bits,
                        ; 12ABh -> 0B12Ah -> 0AB12h -> 2AB1h -> 12ABh
        add cx, 1       ; increment counter   
        jmp hex_loop    ; loop for next hex digit in DX
        
end_hexloop:
		;; Print hex string
        mov esi, hexString
		push esi
		mov ebx, [ebp+12]
		push ebx				; Cursor Y address
		mov ebx, [ebp+8]
		push ebx				; Cursor X address
        call print_string
		add esp, 12

		pop esi

		mov esp, ebp
		pop ebp

        ret             ; return to caller

;; VARIABLES
hexString:      db '0x0000', 0
