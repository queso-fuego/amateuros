;;;
;;; 2ndstage bootloader to hold GDT and VBE code, will jump to kernel
;;;
use16
    org 7E00h       ; 512 bytes after the bootsector in memory

    ;; Set up vbe info structure
    xor ax, ax
    mov es, ax
    mov ah, 4Fh
    mov di, vbe_info_block
    int 10h

    cmp ax, 4Fh
    jne error

    mov ax, word [vbe_info_block.video_mode_pointer]
    mov [offset], ax
    mov ax, word [vbe_info_block.video_mode_pointer+2]
    mov [t_segment], ax
        
    mov fs, ax
    mov si, [offset]

    ;; Get next VBE video mode
    .find_mode:
        mov dx, [fs:si]
        inc si
        inc si
        mov [offset], si
        mov [mode], dx

        cmp dx, word 0FFFFh	        ; at end of video mode list?
        je end_of_modes

        mov ax, 4F01h		        ; get vbe mode info
        mov cx, [mode]
        mov di, mode_info_block		; Mode info block mem address
        int 10h

        cmp ax, 4Fh
        jne error

        ;; Print out mode values...
        ;mov dx, [mode_info_block.x_resolution]	
        ;call print_hex	; Print width
        ;mov ax, 0E20h	; Print a space
        ;int 10h

        ;mov dx, [mode_info_block.y_resolution]
        ;call print_hex	; Print height
        ;mov ax, 0E20h   ; Print a space
        ;int 10h

        ;xor dh, dh
        ;mov dl, [mode_info_block.bits_per_pixel]
        ;call print_hex	; Print bpp
        ;mov ax, 0E0Ah	; Print a newline
        ;int 10h
        ;mov al, 0Dh
        ;int 10h

        ;; Compare values with desired values
        mov ax, [width]
        cmp ax, [mode_info_block.x_resolution]
        jne .next_mode

        mov ax, [height]					
        cmp ax, [mode_info_block.y_resolution]
        jne .next_mode

        mov ax, [bpp]
        cmp al, [mode_info_block.bits_per_pixel]
        jne .next_mode

        ;; Uncomment these to verify correct width/height/bpp values
;;    	cli
;;    	hlt

        mov ax, 4F02h	; Set VBE mode
        mov bx, [mode]
        or bx, 4000h	; Enable linear frame buffer, bit 14
        xor di, di
        int 10h

        cmp ax, 4Fh
        jne error

        jmp load_GDT    ; Move on to set up GDT & 32bit protected mode

    .next_mode:
        mov ax, [t_segment]
        mov fs, ax
        mov si, [offset]
        jmp .find_mode

    error:
        mov ax, 0E46h	; Print 'F'
        int 10h
        cli
        hlt

    end_of_modes:
        mov ax, 0E4Eh	; Print 'N'
        int 10h
        cli
        hlt

    ;; print_hex: Suboutine to print a hex string
    print_hex:
        mov cx, 4	; offset in string, counter (4 hex characters)
        .hex_loop:
            mov ax, dx	              ; Hex word passed in DX
            and al, 0Fh               ; Use nibble in AL
            mov bx, hex_to_ascii
            xlatb                     ; AL = [DS:BX + AL]

            mov bx, cx                ; Need bx to index data
            mov [hexString+bx+1], al  ; Store hex char in string
            ror dx, 4                 ; Get next nibble
        loop .hex_loop 

        mov si, hexString             ; Print out hex string
        mov ah, 0Eh
        mov cx, 6                     ; Length of string
        .loop:
            lodsb
            int 10h
        loop .loop
        ret

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

    ;; Set up VBE mode info block in memory to be easier to work with
    mov esi, mode_info_block
    mov edi, 9000h
    mov ecx, 64                 ; Mode info block is 256 bytes / 4 = # of dbl words
    rep movsd

    jmp 08h:2000h              ; Jump to kernel

;; DATA AREA
hexString: db '0x0000'
hex_to_ascii: db '0123456789ABCDEF'

;; VBE Variables
width: dw 1920
height: dw 1080
bpp: db 32
offset: dw 0
t_segment: dw 0	; "segment" is keyword in fasm
mode: dw 0

;; End normal 2ndstage bootloader sector padding
times 512-($-$$) db 0

vbe_info_block:		; 'Sector' 2
	.vbe_signature: db 'VBE2'
	.vbe_version: dw 0          ; Should be 0300h? BCD value
	.oem_string_pointer: dd 0 
	.capabilities: dd 0
	.video_mode_pointer: dd 0
	.total_memory: dw 0
	.oem_software_rev: dw 0
	.oem_vendor_name_pointer: dd 0
	.oem_product_name_pointer: dd 0
	.oem_product_revision_pointer: dd 0
	.reserved: times 222 db 0
	.oem_data: times 256 db 0

mode_info_block:	; 'Sector' 3
    ;; Mandatory info for all VBE revisions
	.mode_attributes: dw 0
	.window_a_attributes: db 0
	.window_b_attributes: db 0
	.window_granularity: dw 0
	.window_size: dw 0
	.window_a_segment: dw 0
	.window_b_segment: dw 0
	.window_function_pointer: dd 0
	.bytes_per_scanline: dw 0

    ;; Mandatory info for VBE 1.2 and above
	.x_resolution: dw 0
	.y_resolution: dw 0
	.x_charsize: db 0
	.y_charsize: db 0
	.number_of_planes: db 0
	.bits_per_pixel: db 0
	.number_of_banks: db 0
	.memory_model: db 0
	.bank_size: db 0
	.number_of_image_pages: db 0
	.reserved1: db 1

    ;; Direct color fields (required for direct/6 and YUV/7 memory models)
	.red_mask_size: db 0
	.red_field_position: db 0
	.green_mask_size: db 0
	.green_field_position: db 0
	.blue_mask_size: db 0
	.blue_field_position: db 0
	.reserved_mask_size: db 0
	.reserved_field_position: db 0
	.direct_color_mode_info: db 0

    ;; Mandatory info for VBE 2.0 and above
	.physical_base_pointer: dd 0     ; Physical address for flat memory frame buffer
	.reserved2: dd 0
	.reserved3: dw 0

    ;; Mandatory info for VBE 3.0 and above
	.linear_bytes_per_scan_line: dw 0
    .bank_number_of_image_pages: db 0
    .linear_number_of_image_pages: db 0
    .linear_red_mask_size: db 0
    .linear_red_field_position: db 0
    .linear_green_mask_size: db 0
    .linear_green_field_position: db 0
    .linear_blue_mask_size: db 0
    .linear_blue_field_position: db 0
    .linear_reserved_mask_size: db 0
    .linear_reserved_field_position: db 0
    .max_pixel_clock: dd 0

    .reserved4: times 190 db 0      ; Remainder of mode info block

    ;; Sector padding
    times 1536-($-$$) db 0
