;;;
;;; "Simple" boot sector using ATA PIO ports to read sectors from disk into memory
;;;
use16
FILETABLE_ADDRESS equ 1000h

    org 7C00h              ; 'origin' of Boot code; helps make sure addresses don't change

    xor ax, ax             ; Ensure data & extra segments are 0 to start, can help
    mov es, ax             ; with booting on hardware
    mov ds, ax     

    mov byte [drive_num], dl	; DL contains initial drive # on boot

    ;; READ FILE TABLE INTO MEMORY FIRST
    xor bl, bl          ; Will be reading 1 sector
    mov di, 1000h       ; Memory address to read sector into (0000h:1000h)

    mov dx, 1F2h        ; Sector count port
    mov al, 1           ; # of sectors to read
    out dx, al

    mov dx, 1F3h        ; Sector # port
    mov al, 02h         ; Sector # to start reading at (1-based) 
    out dx, al

    mov dx, 1F6h        ; Head & drive # port
    mov al, [drive_num] ; Drive # - hard disk 1
    and al, 0Fh         ; Head # (low nibble)
    or al, 0A0h         ; default high nibble to 'primary' drive (drive 1), 'secondary' drive (drive 2) would be hex B or 1011b
    out dx, al          ; Send head/drive #

    mov dx, 1F4h        ; Cylinder low port
    xor al, al          ; Cylinder low #
    out dx, al

    mov dx, 1F5h        ; Cylinder high port
    out dx, al

    mov dx, 1F7h        ; Command port (writing port 1F7h)
    mov al, 20h         ; Read with retry
    out dx, al

    call load_sector_loop 

    ;; READ 2ND STAGE BOOTLOADER INTO MEMORY SECOND
    mov si, bootloader_string
    call get_filetable_entry
    
    mov di, 7E00h       ; Memory address to read sectors into
    call load_sectors

    ;; READ KERNEL INTO MEMORY THIRD
    mov si, prekernel_string
    call get_filetable_entry

    push es
    mov ax, 5000h
    mov es, ax          ; set ES to 5000h, will point to 50000h
    xor di, di          ; Memory address to read sectors into (5000h:0000h) = (50000h)
    call load_sectors
    pop es

    ;; LOAD FONT INTO MEMORY FOURTH
    mov si, font_string
    call get_filetable_entry

    mov di, 0A000h      ; Memory address to read sectors into 
    call load_sectors

    ;; Jump to 2nd stage bootloader here, does not return
    mov dl, [drive_num]
    mov [1500h], dl     ; Store drive # in global address
    jmp 0000h:7E00h

;; Subroutine to load disk sectors 
;; Inputs:
;;   DI points to address to load sectors to
;;   SI points to file table entry
;; NOTE: "Typical" disk limits are: Cylinder = 0-1023, Head = 0-15, Sector = 1-63 (1-based!!!)
load_sectors:
    ;; First get correct CHS values from given starting sector in filetable entry
    ;; Formula:
    ;; Sector = sector % 63, if 0 then sector is at 63 which is last sector on a head number
    ;; Head = sector / 63, if sector % 63 then decrement by 1
    ;; Cylinder = head / 16
    ;; Head = head % 16
    ;; ------------------------
    mov word [cylinder], 0  ; Reset CHS values
    mov byte [head], 0
    mov byte [start_sector], 0
    mov al, [si+14]         ; Get starting sector
    mov [start_sector], al

    cmp al, 63          ; Sector limit (1-63)
    jle .go_on          ; Sector is <= 63, good to go
    xor ah, ah
    mov bl, 63
    div bl              ; starting_sector / 63, AH = remainder (sector % 63), AL = quotient (sector / 63)
    cmp ah, 0           ; sector % 63 = 0?
    jne .cylinder
    mov ah, 63          ; Yes, reset sector to 63
    dec al              ; decrement head # (sector / 63)
    .cylinder:
    mov [start_sector], ah  ; AH = sector #, AL = head #
    xor ah, ah
    mov bl, 16
    div bl                  ; Head / 16, AH = remainder (head % 16), AL = quotient (head / 16)
    mov [cylinder], al      ; Cylinder = head / 16
    mov [head], ah
    ;; TODO: Check for over disk limit (cylinder > 1023)

    .go_on:
    mov al, [si+15]     ; File size in sectors
    mov bl, al          
    dec bl              ; BL = sectors-1, for disk reading logic later

    mov dx, 1F2h        ; Sector count port
    out dx, al

    mov dx, 1F3h        ; Sector # port
    mov al, [start_sector]
    out dx, al

    mov dx, 1F6h        ; Head & drive # port
    mov al, 0A0h        ; default high nibble to 'primary' drive (drive 1), 'secondary' drive (drive 2) would be hex B or 1011b
    or al, [head]       ; Set low nibble to head # (0-15)
    out dx, al          ; Send head/drive #

    mov dx, 1F4h        ; Cylinder low port
    mov ax, [cylinder]  ; Cylinder low byte is in AL, high byte is in AH
    out dx, al

    mov dx, 1F5h        ; Cylinder high port
    mov al, ah          ; Cylinder high byte
    out dx, al

    mov dx, 1F7h        ; Command port (writing port 1F7h)
    mov al, 20h         ; Read with retry
    out dx, al

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

    ;; Subroutine to get a file table entry from the file table
    ;; Inputs:
    ;;   SI points to file name to search for
    ;; Outputs:
    ;;   SI points to start of the file table entry for given file name
    get_filetable_entry:
        mov di, FILETABLE_ADDRESS  
        .loop:
            mov cl, 10
            push di
            push si
            repe cmpsb
            je .return
            pop si
            pop di
            add di, 16       ; Point to next filetable entry
        jmp .loop

        .return:
            pop si
            pop di
            mov si, di      ; Set si to the entry
            ret
            
;; VARIABLES
drive_num: db 0
bootloader_string: db '2ndstage  '
prekernel_string: db '3rdstage  '
font_string: db 'termu18n  '
cylinder: dw 0
head: db 0
start_sector: db 0

;; Boot sector magic
times 510-($-$$) db 0   ; pads out 0s until we reach 510th byte
dw 0AA55h               ; BIOS magic number; BOOT magic #
