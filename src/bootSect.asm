;;;
;;; "Simple" boot sector using ATA PIO ports to read sectors from disk into memory
;;;
use16
;FILETABLE_ADDRESS equ 1000h

    org 7C00h              ; 'origin' of Boot code; helps make sure addresses don't change

    xor ax, ax             ; Ensure data & extra segments are 0 to start, can help
    mov es, ax             ; with booting on hardware
    mov ds, ax     

    mov byte [drive_num], dl	; DL contains initial drive # on boot

    ;; Enable A20 line through BIOS
    mov ax, 2403h   ; A20 Gate support
    int 15h
    jb a20_ns       ; int 15h not supported
    cmp ah, 0
    jnz a20_ns      ; int 15h not supported

    mov ax, 2402h   ; A20 Gate status
    int 15h
    jb a20_ns       ; Couldn't get status
    cmp ah, 0
    jnz a20_ns      ; Couldn't get status
    cmp al, 1
    jz a20_on

    a20_ns:
    cli
    hlt

    a20_on:
    ;; Read boot block & Superblock from disk for 2nd stage bootloader
    ;;   sectors 2-16 / 28 bit LBA 1-15
    mov bl, 14          ; # of sectors to read - 1
    mov di, 7E00h       ; Address to read sectors into 

    mov dx, 1F6h        ; Head / drive, flags
    mov al, 0xE0        ; 0b11100000, bits: 0-3 = LBA bits 24-27, 4 = drive, 5 = set, 6 = LBA, 7 = set
    out dx, al

    mov dx, 1F2h        ; Sector count port
    mov al, 15          ; # of sectors to read
    out dx, al

    inc dx              ; 1F3h: Sector # port / LBA low
    mov al, 1           ; lowest 8 bits of LBA, reading from 2nd sector on disk
    out dx, al

    inc dx              ; 1F4h: Cylinder low / LBA mid
    xor ax, ax          ; AL = 0
    out dx, al

    inc dx              ; 1F5h: Cylinder high / LBA high
    out dx, al

    mov dx, 1F7h        ; Command port
    mov al, 20h         ; Read with retry
    out dx, al

    call load_sector_loop

    ;; Load 3rd stage bootloader at reserved inode 2
    ;; Parse superblock to find inode start disk block
    ;;  from bootsector, superblock should be at addresses 0x8C00-0x9C00
    superblock: equ 8C00h
    first_inode_block: equ superblock + 10

    ;; Load inode block from disk - each block of 4KB = 8 LBAs/Sectors of 512 bytes
    imul cx, [first_inode_block], 8

    mov bl, 7           ; # of sectors to read - 1
    mov di, 0xB000      ; Address to read sectors into (B000h - C000h)

    mov dx, 1F6h        ; Head / drive, flags
    mov al, 0xE0        ; 0b11100000, bits 5 & 7 are set bit 6 = LBA
    out dx, al

    mov dx, 1F2h        ; Sector count port
    mov al, 8           ; # of sectors to read, 8 sectors per 4KB block
    out dx, al

    inc dx              ; 1F3h: Sector # port / LBA low
    mov al, cl          ; Lowest 8 bits of LBA
    out dx, al

    inc dx              ; 1F4h: Cylinder low / LBA mid
    mov al, ch          ; Next 8 bits
    out dx, al

    inc dx              ; 1F5h: Cylinder high / LBA high
    xor ax, ax          ; Next 8 bits
    out dx, al

    mov dx, 1F7h        ; Command port
    mov al, 20h         ; Read with retry
    out dx, al

    call load_sector_loop

    ;; Parse inode 2 to get starting block & length from 1st extent
    ;;  Inode size = 64 bytes
    bootloader_inode: equ 0xB000 + 128
    size_sectors: equ bootloader_inode + 9
    extent0: equ bootloader_inode + 20
    imul cx, [extent0], 8          ; Starting block, in sectors
    imul bx, [extent0+4], 8        ; Length in blocks, in sectors

    ;; Load blocks from disk
    mov dx, 1F2h        ; Sector count port
    mov al, bl          ; # of sectors to read, 8 sectors per 4KB block
    out dx, al

    inc dx              ; 1F3h: Sector # port / LBA low
    mov al, cl          ; Lowest 8 bits of LBA
    out dx, al

    inc dx              ; 1F4h: Cylinder low / LBA mid
    shr ecx, 8
    mov al, cl          ; Next 8 bits
    out dx, al

    inc dx              ; 1F5h: Cylinder high / LBA high
    shr ecx, 8
    mov al, cl          ; Next 8 bits
    out dx, al

    inc dx              ; 1F6h: Head / drive, flags
    and ch, 0x0F        ; Get LBA bits 24-27
    mov al, ch          
    or al, 0xE0         ; 0b11100000, bits 5 & 7 are set bit 6 = LBA
    out dx, al

    inc dx              ; 1F7h: Command port
    mov al, 20h         ; Read with retry
    out dx, al

    dec bl              ; # of sectors to read - 1
    mov ax, 5000h
    mov es, ax
    xor di, di          ; ES:DI = 5000h:0000h (50000h)

    call load_sector_loop

    xor ax, ax
    mov es, ax

    mov dl, [drive_num]
    mov [1500h], dl         ; Store drive # in global address
    jmp 0000h:7E00h         ; Jump to 2nd stage boot loader

    ;; Subroutine to load disk sectors 
    ;; Inputs:
    ;;   (ES:)DI = address to load sectors to
    ;;   BL = # of sectors to read - 1
    load_sector_loop:
        in al, dx           ; Status register (reading port 1F7h)
        test al, 8          ; Sector buffer requires servicing 
        je load_sector_loop ; Keep trying until sector buffer is ready

        mov cx, 256         ; # of words to read for 1 sector
        mov dx, 1F0h        ; Data port, reading 
        rep insw            ; Read bytes from DX port # into DI, CX # of times
        
        ;; 400ns delay - Read alternate status register
        mov dx, 3F6h
        in al, dx
        in al, dx
        in al, dx
        in al, dx

        cmp bl, 0           ; Still have sectors to read?
        je .return

        dec bl              
        mov dx, 1F7h
        jmp load_sector_loop 

    .return:
    ret

;; VARIABLES
drive_num: db 0

;; Boot sector magic
times 510-($-$$) db 0   ; pads out 0s until we reach 510th byte
dw 0AA55h               ; BIOS magic number; BOOT magic #
