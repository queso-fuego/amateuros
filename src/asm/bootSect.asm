;;;
;;; Simple boot loader that uses INT13 AH2 to read from disk into memory
;;;
        org 7C00h              ; 'origin' of Boot code; helps make sure addresses don't change

        ;; READ FILE TABLE INTO MEMORY FIRST
        ;; Set up ES:BX segment:offset to load sector(s) into 
        mov bx, 1000h          ; load sector to memory address 1000h
        mov es, bx             ; ES = 1000h
        mov bx, 00h            ; ES:BX = 1000h:0000h

        ;; Set up disk read
        mov dh, 00h            ; head #
        mov dl, 00h            ; drive #
        mov ch, 00h            ; cylinder #
        mov cl, 06h            ; starting sector # to read from disk

load_file_table:
        mov ah, 02h            ; BIOS int 13/ ah=2 read disk sectors
        mov al, 01h            ; # sectors to read 
        int 13h                ; BIOS interrupts for disk functions

        jc load_file_table     ; retry if disk read error (carry flag set/ = 1)

        ;; READ KERNEL INTO MEMORY SECOND
        ;; Set up ES:BX segment:offset to load sector(s) into 
        mov bx, 2000h          ; load sector to memory address 2000h
        mov es, bx             ; ES = 2000h
        mov bx, 00h            ; ES:BX = 2000h:0000h

        ;; Set up disk read
        mov dh, 00h            ; head #
        mov dl, 00h            ; drive #
        mov ch, 00h            ; cylinder #
        mov cl, 02h            ; starting sector # to read from disk

load_kernel:
        mov ah, 02h            ; BIOS int 13/ ah=2 read disk sectors
        mov al, 04h            ; # sectors to read 
        int 13h                ; BIOS interrupts for disk functions

        jc load_kernel         ; retry if disk read error (carry flag set/ = 1)


        ;; Reset segment registers for RAM
        mov ax, 2000h
        mov ds, ax              ; data segment
        mov es, ax              ; extra segment
        mov fs, ax              ; ""
        mov gs, ax              ; ""

		;; Set up stack
		mov sp, 0FFFFh			; stack pointer
		mov ax, 9000h
        mov ss, ax              ; stack segment

		;; Set up initial video mode & palette before going to kernel
		mov ax, 0003h			; int 10h ah00 = set video mode; 80x25 16 colors text mode
		int 10h
		
		mov ah, 0Bh				; int 10h ah 0Bh = set palette
		mov bx, 0001h			; bl = bg color for text mode; blue
		int 10h

        jmp 2000h:0000h         ; never return from this!

        ;; Boot sector magic
        times 510-($-$$) db 0   ; pads out 0s until we reach 510th byte

        dw 0AA55h               ; BIOS magic number; BOOT magic #
