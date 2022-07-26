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

    ;; Read in the boot block and the superblock to memory for 2nd stage bootloader
    mov al, 15          ; 8 - 1 for boot block - bootsector & 8 for super block
    mov bl, al
    dec bl              ; BX = # of sectors to read - 1
    mov di, 7E00h       ; Read sectors to this address, 7C00h-8C00h = boot block, 8C00h-9C00h = superblock

    mov dx, 1F6h        ; Head / drive port, flags
    mov al, 0xE0        ; 0b0111 0000, bits: 0-3 = LBA bits 24-27, 4 = drive, 5 = always set, 6 = set for LBA, 7 = always set
    out dx, al

    mov dx, 1F2h        ; Sector count port
    mov al, 15          ; # of sectors to read
    out dx, al

    inc dx              ; 1F3h, sector # port / LBA low
    mov al, 1           ; LBA 1 = 2nd disk sector (0-based), bits 0-7 of LBA
    out dx, al

    inc dx              ; 1F4h, Cylinder low / LBA mid
    xor ax, ax          ; bits 8-15 of LBA
    out dx, al

    inc dx              ; 1F5h, Cylinder high / LBA high
    ; xor ax, ax        ; bits 16-23 of LBA
    out dx, al

    inc dx
    inc dx              ; 1F7h, Command port
    mov al, 20h         ; Read with retry
    out dx, al

    call load_sector_loop

    ;; Parse superblock for first inode block, offset to reserved bootloader
    ;;  inode for 3rd stage bootloader (inode 2), and load it to memory
    SUPERBLOCK: equ 8C00h
    FIRST_INODE_BLOCK: equ SUPERBLOCK + 12

    ;; Read first inode block to memory
    ;; Each disk block is 4KB or 8 sectors of 512 bytes
    imul cx, [FIRST_INODE_BLOCK], 8 ; CX = disk sector/LBA of first inode block

    mov bl, 7       ; 8 sectors for a block - 1
    mov di, 0xB000  ; Address to read into, first inode block = 0xB000-0xC000

    mov dx, 1F6h    ; Head/drive port, flags
    mov al, 0xE0
    out dx, al

    mov dx, 1F2h        ; Sector count port
    mov al, 8           ; # of sectors to read
    out dx, al

    inc dx              ; 1F3h, sector # port / LBA low
    mov al, cl          ; LBA 1 = 2nd disk sector (0-based), bits 0-7 of LBA
    out dx, al

    inc dx              ; 1F4h, Cylinder low / LBA mid
    mov al, ch          ; bits 8-15 of LBA
    out dx, al

    inc dx              ; 1F5h, Cylinder high / LBA high
    xor ax, ax          ; bits 16-23 of LBA
    out dx, al

    inc dx
    inc dx              ; 1F7h, Command port
    mov al, 20h         ; Read with retry
    out dx, al

    call load_sector_loop

    ;; Read boot loader inode to memory
    ;;  inode 2, each inode is 64 bytes in size
    BOOTLOADER_INODE: equ 0xB000 + 128
    EXTENT0: equ BOOTLOADER_INODE + 21      ; Offset of extent[0] in inode_t
    imul ecx, [EXTENT0], 8                  ; first_block translated to disk sector/LBA
    imul ebx, [EXTENT0+4], 8                ; length_blocks translated to disk sectors/LBAs

    mov dx, 1F2h        ; Sector count port
    mov al, bl          ; # of sectors to read
    out dx, al

    inc dx              ; 1F3h, sector # port / LBA low
    mov al, cl          ; LBA 1 = 2nd disk sector (0-based), bits 0-7 of LBA
    out dx, al

    inc dx              ; 1F4h, Cylinder low / LBA mid
    mov al, ch          ; bits 8-15 of LBA
    out dx, al

    inc dx              ; 1F5h, Cylinder high / LBA high
    shr ecx, 16
    mov al, cl          ; bits 16-23 of LBA
    out dx, al

    inc dx              ; 1F6h, Head/drive port, flags
    and ch, 0x0F        ; Get top 4 bits only
    mov al, ch          ; bits 24-27 of LBA
    or al, 0xE0         ; Set flags
    out dx, al

    inc dx              ; 1F7h, Command port
    mov al, 20h         ; Read with retry
    out dx, al

    dec bl              ; BL = # of sectors to read - 1
    mov ax, 5000h
    mov es, ax          ; ES = 0x5000
    xor di, di          ; ES:DI = 0x5000:0x0000 = 0x50000

    call load_sector_loop

    xor ax, ax
    mov es, ax          ; ES = 0x0000

    ;; Jump to 2nd stage bootloader here, does not return
    mov dl, [drive_num]
    mov [1500h], dl     ; Store drive # in global address
    jmp 0000h:7E00h

    ;; Poll status port after reading 1 sector
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

        cmp bl, 0           ; Sector # to still read
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
