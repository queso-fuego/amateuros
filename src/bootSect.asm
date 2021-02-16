;;;
;;; "Simple" boot loader that uses ATA PIO ports to read from disk into memory
;;;
use16
    org 7C00h              ; 'origin' of Boot code; helps make sure addresses don't change

    mov byte [drive_num], dl	; DL contains initial drive # on boot

    ;; READ 2ND STAGE BOOTLOADER INTO MEMORY FIRST
    mov bl, 02h         ; Will be reading 3 sectors 
    mov di, 7E00h       ; Memory address to read sectors into

    mov dx, 1F6h        ; Head & drive # port
    mov al, [drive_num] ; Drive # - hard disk 1
    and al, 0Fh         ; Head # (low nibble)
    or al, 0A0h         ; default high nibble to 'primary' drive (drive 1), 'secondary' drive (drive 2) would be hex B or 1011b
    out dx, al          ; Send head/drive #

    mov dx, 1F2h        ; Sector count port
    mov al, 03h         ; # of sectors to read
    out dx, al

    mov dx, 1F3h        ; Sector # port
    mov al, 2h          ; Sector to start reading at (sectors are 1-based)
    out dx, al

    mov dx, 1F4h        ; Cylinder low port
    xor al, al          ; Cylinder low #
    out dx, al

    mov dx, 1F5h        ; Cylinder high port
    xor al, al          ; Cylinder high #
    out dx, al

    mov dx, 1F7h        ; Command port (writing port 1F7h)
    mov al, 20h         ; Read with retry
    out dx, al

;; Poll status port after reading 1 sector
second_stage_loop:
    in al, dx           ; Status register (reading port 1F7h)
    test al, 8          ; Sector buffer requires servicing
    je second_stage_loop     ; Keep trying until sector buffer is ready

    mov cx, 256         ; # of words to read for 1 sector
    mov dx, 1F0h        ; Data port, reading 
    rep insw            ; Read bytes from DX port # into DI, CX # of times
    
    ;; 400ns delay - Read alternate status register
    mov dx, 3F6h
    in al, dx
    in al, dx
    in al, dx
    in al, dx

    cmp bl, 0
    je load_filetable

    dec bl
    mov dx, 1F7h
    jmp second_stage_loop

    ;; READ FILE TABLE INTO MEMORY SECOND
load_filetable:
    mov dx, 1F6h        ; Head & drive # port
    mov al, [drive_num] ; Drive # - hard disk 1
    and al, 0Fh         ; Head # (low nibble)
    or al, 0A0h         ; default high nibble to 'primary' drive (drive 1), 'secondary' drive (drive 2) would be hex B or 1011b
    out dx, al          ; Send head/drive #

    mov dx, 1F2h        ; Sector count port
    mov al, 1           ; # of sectors to read
    out dx, al

    mov dx, 1F3h        ; Sector # port
    mov al, 17h         ; Sector # to start reading at (1-based)
    out dx, al

    mov dx, 1F4h        ; Cylinder low port
    xor al, al          ; Cylinder low #
    out dx, al

    mov dx, 1F5h        ; Cylinder high port
    xor al, al          ; Cylinder high #
    out dx, al

    mov dx, 1F7h        ; Command port (writing port 1F7h)
    mov al, 20h         ; Read with retry
    out dx, al

.loop:
    in al, dx           ; Status register (reading port 1F7h)
    test al, 8          ; Sector buffer requires servicing
    je .loop            ; Keep trying until sector buffer is ready

    xor ax, ax
    mov es, ax
    mov di, 1000h       ; Memory address to read sector into (0000h:1000h)
    mov cx, 256         ; # of words to read for 1 sector
    mov dx, 1F0h        ; Data port, reading 
    rep insw            ; Read bytes from DX port # into DI, CX # of times

    ;; 400ns delay - Read alternate status register
    mov dx, 3F6h
    in al, dx
    in al, dx
    in al, dx
    in al, dx

    ;; READ KERNEL INTO MEMORY THIRD
    mov bl, 0Dh         ; Will be reading 14 sectors 
    mov di, 2000h       ; Memory address to read sectors into (0000h:2000h)

    mov dx, 1F6h        ; Head & drive # port
    mov al, [drive_num] ; Drive # - hard disk 1
    and al, 0Fh         ; Head # (low nibble)
    or al, 0A0h         ; default high nibble to 'primary' drive (drive 1), 'secondary' drive (drive 2) would be hex B or 1011b
    out dx, al          ; Send head/drive #

    mov dx, 1F2h        ; Sector count port
    mov al, 0Eh         ; # of sectors to read
    out dx, al

    mov dx, 1F3h        ; Sector # port
    mov al, 9h          ; Sector to start reading at (sectors are 1-based)
    out dx, al

    mov dx, 1F4h        ; Cylinder low port
    xor al, al          ; Cylinder low #
    out dx, al

    mov dx, 1F5h        ; Cylinder high port
    xor al, al          ; Cylinder high #
    out dx, al

    mov dx, 1F7h        ; Command port (writing port 1F7h)
    mov al, 20h         ; Read with retry
    out dx, al

;; Poll status port after reading 1 sector
kernel_loop:
    in al, dx           ; Status register (reading port 1F7h)
    test al, 8          ; Sector buffer requires servicing
    je kernel_loop     ; Keep trying until sector buffer is ready

    mov cx, 256         ; # of words to read for 1 sector
    mov dx, 1F0h        ; Data port, reading 
    rep insw            ; Read bytes from DX port # into DI, CX # of times
    
    ;; 400ns delay - Read alternate status register
    mov dx, 3F6h
    in al, dx
    in al, dx
    in al, dx
    in al, dx

    cmp bl, 0
    je load_font

    dec bl
    mov dx, 1F7h
    jmp kernel_loop

    ;; LOAD FONT INTO MEMORY FOURTH
load_font:
    mov bl, 03h         ; Will be reading 4 sectors 
    mov di, 6000h       ; Memory address to read sectors into 

    mov dx, 1F6h        ; Head & drive # port
    mov al, [drive_num] ; Drive # - hard disk 1
    and al, 0Fh         ; Head # (low nibble)
    or al, 0A0h         ; default high nibble to 'primary' drive (drive 1), 'secondary' drive (drive 2) would be hex B or 1011b
    out dx, al          ; Send head/drive #

    mov dx, 1F2h        ; Sector count port
    mov al, 04h         ; # of sectors to read
    out dx, al

    mov dx, 1F3h        ; Sector # port
    mov al, 5h          ; Sector to start reading at (sectors are 1-based)
    out dx, al

    mov dx, 1F4h        ; Cylinder low port
    xor al, al          ; Cylinder low #
    out dx, al

    mov dx, 1F5h        ; Cylinder high port
    xor al, al          ; Cylinder high #
    out dx, al

    mov dx, 1F7h        ; Command port (writing port 1F7h)
    mov al, 20h         ; Read with retry
    out dx, al

;; Poll status port after reading 1 sector
font_loop:
    in al, dx           ; Status register (reading port 1F7h)
    test al, 8          ; Sector buffer requires servicing
    je font_loop        ; Keep trying until sector buffer is ready

    mov cx, 256         ; # of words to read for 1 sector
    mov dx, 1F0h        ; Data port, reading 
    rep insw            ; Read bytes from DX port # into DI, CX # of times
    
    ;; 400ns delay - Read alternate status register
    mov dx, 3F6h
    in al, dx
    in al, dx
    in al, dx
    in al, dx

    cmp bl, 0
    je load_2ndstage

    dec bl
    mov dx, 1F7h
    jmp font_loop

    ; Jump to 2nd stage bootloader here
    load_2ndstage:
        mov dl, [drive_num]
        jmp 0000h:7E00h

;; VARIABLES
drive_num: db 0

;; Boot sector magic
times 510-($-$$) db 0   ; pads out 0s until we reach 510th byte
dw 0AA55h               ; BIOS magic number; BOOT magic #
