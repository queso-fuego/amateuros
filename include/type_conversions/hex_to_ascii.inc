;;; =========================================================================== 
;;; hex_to_ascii.inc: Convert a hex character to it's ascii equivalent
;;; ===========================================================================
;;; Parms: 
;;;  input 1: AL - hex character to convert
;;;  output 1: AL - converted ASCII character
use32
hex_to_ascii:
	mov ah, 0Eh
	add al, 30h		; convert to ascii number
	cmp al, 39h		; is value 0h-9h or A-F
	jle hexNum
	add al, 07h		; add hex 7 to get ascii 'A'-'F'
hexNum:
	ret
