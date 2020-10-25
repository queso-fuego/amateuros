;;;
;;; Simple boot loader that uses INT13 AH2 to read from disk into memory
;;;
        org 7C00h              ; 'origin' of Boot code; helps make sure addresses don't change

        ;; READ FILE TABLE INTO MEMORY FIRST
        ;; Set up ES:BX segment:offset to load sector(s) into 
        mov bx, 100h          ; load sector to memory address 1000h
        mov es, bx            ; ES = 1000h
        xor bx, bx            ; ES:BX = 100h:0000h = 1000h

        ;; Set up disk read
		mov dx, 0080h
		mov cx, 000Bh			; ch = cylinder #, cl = starting sector to read

load_file_table:
		mov ax, 0201h			; ah = 02h/int 13 read disk sectors; al = # of sectors to read
        int 13h                 ; BIOS interrupts for disk functions

        jc load_file_table      ; retry if disk read error (carry flag set/ = 1)

        ;; READ KERNEL INTO MEMORY SECOND
        ;; Set up ES:BX segment:offset to load sector(s) into 
        mov bx, 200h          ; load sector to memory address 2000h
        mov es, bx            ; ES = 2000h
        xor bx, bx            ; ES:BX = 200h:0000h = 2000h

        ;; Set up disk read
		mov dx, 0080h
		mov cx, 0002h			; ch = cylinder #, cl = starting sector to read

load_kernel:
		mov ax, 0209h			; ah = 02h/int 13 read disk sectors; al = # of sectors to read
        int 13h                ; BIOS interrupts for disk functions

        jc load_kernel         ; retry if disk read error (carry flag set/ = 1)

        ;; Reset segment registers for RAM
        mov ax, 200h
        mov ds, ax              ; data segment
        mov es, ax              ; extra segment
        mov fs, ax              ; ""
        mov gs, ax              ; ""

		;; Set up stack
		mov sp, 0FFFFh			; stack pointer
		mov ax, 900h
        mov ss, ax              ; stack segment

		;; Set up initial video mode & palette before going to kernel
		mov ax, 0003h			; int 10h ah00 = set video mode; 80x25 16 colors text mode
		int 10h
		
		mov ah, 0Bh				; int 10h ah 0Bh = set palette
		mov bx, 0001h			; bl = bg color for text mode; blue
		int 10h

        jmp 200h:0000h	            ; never return from this!

        ;; Boot sector magic
        times 510-($-$$) db 0   ; pads out 0s until we reach 510th byte

        dw 0AA55h               ; BIOS magic number; BOOT magic #
