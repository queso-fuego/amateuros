;;;
;;; "Simple" boot loader that uses ATA PIO ports to read from disk into memory
;;;
use16
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
    mov al, 0Fh         ; Sector to start reading at (sectors are 0-based!)
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
    mov bl, 0Dh         ; Will be reading 14 sectors NEW
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
    je load_GDT

    dec bl
    mov dx, 1F7h
    jmp kernel_loop

    ;; Set up GDT
    GDT_start:
    ;; Offset 0h
    dq 0h           ;; 1st descriptor required to be NULL descriptor

    ;; Offset 08h
    .code:
    dw 0FFFFh       ; Segment limit 1 - 2 bytes
    dw 0h           ; Segment base 1 - 2 bytes
    db 0h           ; Segment base 2 - 1 byte
    db 10011010b    ; Access byte - bits: 7 - Present, 6-5 - privelege level (0 = kernel), 4 - descriptor type (code/data)
                    ;  3 - executable y/n, 2 - direction/conforming (grow up from base to limit), 1 - read/write, 0 - accessed (CPU sets this)
    db 11001111b    ; bits: 7 - granularity (4KiB), 6 - size (32bit protected mode), 3-0 segment limit 2 - 4 bits
    db 0h           ; Segment base 3 - 1 byte

    ;; Offset 10h
    .data:
    dw 0FFFFh       ; Segment limit 1 - 2 bytes
    dw 0h           ; Segment base 1 - 2 bytes
    db 0h           ; Segment base 2 - 1 byte
    db 10010010b    ; Access byte
    db 11001111b    ; bits: 7 - granularity (4KiB), 6 - size (32bit protected mode), 3-0 segment limit 2 - 4 bits
    db 0h           ; Segment base 3 - 1 byte

    ;; Load GDT
    GDT_Desc:
    dw ($ - GDT_start - 1)
    dd GDT_start

    load_GDT:
    mov dl, [drive_num]
    cli             ; Clear interrupts first
    lgdt [GDT_Desc] ; Load the GDT to the cpu

    mov eax, cr0
    or eax, 1       ; Set protected mode bit
    mov cr0, eax    ; Turn on protected mode

    jmp 08h:set_segments   ; Do a far jump to set CS register
    
use32                    ; We are officially in 32 bit mode now
    set_segments:
    mov ax, 10h          ; Set to data segment descriptor
    mov ds, ax
    mov es, ax                  
    mov fs, ax                 
    mov gs, ax                
    mov ss, ax
    mov esp, 090000h	    ; Set up stack pointer

    sti                 ; Enable interrupts
    jmp 08h:2000h              ; Jump to kernel

;; VARIABLES
drive_num: db 0

;; Boot sector magic
times 510-($-$$) db 0   ; pads out 0s until we reach 510th byte
dw 0AA55h               ; BIOS magic number; BOOT magic #
