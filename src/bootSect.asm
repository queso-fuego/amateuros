;;;
;;; "Simple" boot loader that uses ATA PIO ports to read from disk into memory
;;;
    org 7C00h              ; 'origin' of Boot code; helps make sure addresses don't change

    mov byte [drive_num], dl	; DL contains initial drive # on boot

    ;; READ FILE TABLE INTO MEMORY FIRST
    mov dx, 1F6h        ; Head & drive # port
    mov al, [drive_num] ; Drive # - hard disk 1
    and al, 0Fh         ; Head # (low nibble)
    or al, 0A0h         ; default high nibble to 'primary' drive (drive 1), 'secondary' drive (drive 2) would be hex B or 1011b
    out dx, al          ; Send head/drive #

    mov dx, 1F2h        ; Sector count port
    mov al, 1           ; # of sectors to read
    out dx, al

    mov dx, 1F3h        ; Sector # port
    mov al, 0Ch         ; Sector to start reading at (sectors are 0-based!)
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

    ;; READ KERNEL INTO MEMORY SECOND
    mov bl, 9           ; Will be reading 10 sectors
    mov di, 2000h       ; Memory address to read sectors into (0000h:2000h)

    mov dx, 1F6h        ; Head & drive # port
    mov al, [drive_num] ; Drive # - hard disk 1
    and al, 0Fh         ; Head # (low nibble)
    or al, 0A0h         ; default high nibble to 'primary' drive (drive 1), 'secondary' drive (drive 2) would be hex B or 1011b
    out dx, al          ; Send head/drive #

    mov dx, 1F2h        ; Sector count port
    mov al, 0Ah         ; # of sectors to read
    out dx, al

    mov dx, 1F3h        ; Sector # port
    mov al, 2           ; Sector to start reading at (sectors are 0-based!!)
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
    je jump_to_kernel

    dec bl
    mov dx, 1F7h
    jmp kernel_loop

jump_to_kernel:
    ;; Set registers for kernel memory location
    mov dl, [drive_num]
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

    ;; Far jump to kernel
    jmp 0200h:0000h	        ; never return from this!

;; VARIABLES
drive_num: db 0

;; Boot sector magic
times 510-($-$$) db 0   ; pads out 0s until we reach 510th byte

dw 0AA55h               ; BIOS magic number; BOOT magic #
