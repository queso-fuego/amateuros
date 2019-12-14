;;;
;;; Simple boot loader that uses INT13 AH2 to read from disk into memory
;;;
        org 0x7c00              ; 'origin' of Boot code; helps make sure addresses don't change

        ;; READ FILE TABLE INTO MEMORY FIRST
        ;; Set up ES:BX segment:offset to load sector(s) into 
        mov bx, 0x1000          ; load sector to memory address 0x1000
        mov es, bx              ; ES = 0x1000
        mov bx, 0x0             ; ES:BX = 0x1000:0x0000

        ;; Set up disk read
        mov dh, 0x0             ; head #
        mov dl, 0x0             ; drive #
        mov ch, 0x0             ; cylinder #
        mov cl, 0x02            ; starting sector # to read from disk

read_disk1:
        mov ah, 0x02            ; BIOS int 13/ ah=2 read disk sectors
        mov al, 0x01            ; # sectors to read 
        int 0x13                ; BIOS interrupts for disk functions

        jc read_disk1            ; retry if disk read error (carry flag set/ = 1)

        ;; READ KERNEL INTO MEMORY SECOND
        ;; Set up ES:BX segment:offset to load sector(s) into 
        mov bx, 0x2000          ; load sector to memory address 0x2000
        mov es, bx              ; ES = 0x2000
        mov bx, 0x0             ; ES:BX = 0x2000:0x0000

        ;; Set up disk read
        mov dh, 0x0             ; head #
        mov dl, 0x0             ; drive #
        mov ch, 0x0             ; cylinder #
        mov cl, 0x03            ; starting sector # to read from disk

read_disk2:
        mov ah, 0x02            ; BIOS int 13/ ah=2 read disk sectors
        mov al, 0x02            ; # sectors to read 
        int 0x13                ; BIOS interrupts for disk functions

        jc read_disk2            ; retry if disk read error (carry flag set/ = 1)


        ;; Reset segment registers for RAM
        mov ax, 0x2000
        mov ds, ax              ; data segment
        mov es, ax              ; extra segment
        mov fs, ax              ; ""
        mov gs, ax              ; ""
        mov ss, ax              ; stack segment

        jmp 0x2000:0x0          ; never return from this!

        ;; Boot sector magic
        times 510-($-$$) db 0   ; pads out 0s until we reach 510th byte

        dw 0xaa55               ; BIOS magic number; BOOT magic #
