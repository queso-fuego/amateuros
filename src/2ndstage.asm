;;;
;;; 2ndstage bootloader to hold GDT and VBE code, will jump to kernel
;;;
use16
    org 7E00h       ; 512 bytes after the bootsector in memory

    xor ax, ax
    mov es, ax      ; ES = 0, because ES:DI will be used for memory map and VBE

    ;; Get physical memory map BIOS int 15h EAX E820h
    ;; ES:DI points to buffer of 24 byte entries
    ;; BP will = number of total entries
    memmap_entries equ 0x8500       ; Store number of memory map entries here
    get_memory_map:
        mov di, 0x8504              ; Memory map entries start here
        xor ebx, ebx                ; EBX = 0 to start, will contain continuation values
        xor bp, bp                  ; BP will store the number of entries
        mov edx, 'PAMS'             ; EDX = 'SMAP' but little endian
        mov eax, 0E820h             ; bios interrupt function number
        mov [ES:DI+20], dword 1     ; Force a valid ACPI 3.x entry
        mov ecx, 24                 ; Each entry can be up to 24 bytes
        int 15h                     ; Call the interrupt
        jc .error                   ; If carry is set, function not supported or errored

        cmp eax, 'PAMS'             ; EAX should equal 'SMAP' on successful call
        jne .error
        test ebx, ebx               ; Does EBX = 0? if so only 1 entry or no entries :(
        jz .error
        jmp .start                  ; EBX != 0, have a valid entry

    .next_entry:
        mov edx, 'PAMS'             ; Some BIOS may clobber edx, reset it here
        mov ecx, 24                 ; Reset ECX
        mov eax, 0E820h             ; Reset EAX
        int 15h

    .start:
        jcxz .skip_entry            ; Memory map entry is 0 bytes in length, skip
        mov ecx, [ES:DI+8]          ; Low 32 bits of length
        or ecx, [ES:DI+12]          ; Or with high 32 bits of length, or will also set the ZF
        jz .skip_entry              ; Length of memory region returned = 0, skip
        
    .good_entry:
        inc bp                      ; Increment number of entries
        add di, 24

    .skip_entry:
        test ebx, ebx               ; If EBX != 0, still have entries to read 
        jz .done
        jmp .next_entry

    .error:
        stc
        jmp input_gfx_values
    .done:
        mov [memmap_entries], bp    ; Store # of memory map entries when done
        clc

    ;; User input x/y resolution and bpp values
    input_gfx_values:
        xor ecx, ecx
        cmp word [width], 0  ;; Already has values set, skip
        jne set_up_vbe

        mov si, choose_gfx_string
        mov cx, choose_gfx_string.len
        call print_string

        mov si, width_string
        mov cx, width_string.len
        call print_string

        call input_number

        mov [width], bx

        mov si, height_string
        mov cx, height_string.len
        call print_string

        call input_number

        mov [height], bx

        mov si, bpp_string
        mov cx, bpp_string.len
        call print_string

        call input_number

        mov [bpp], bl   

    ;; Set up vbe info structure
    set_up_vbe:
    xor ax, ax
    mov es, ax  ; Reset ES to 0

    mov ax, 4F00h
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

        ;; Compare values with desired values
        mov ax, [width]
        cmp ax, [mode_info_block.x_resolution]
        jne .next_mode

        mov ax, [height]					
        cmp ax, [mode_info_block.y_resolution]
        jne .next_mode

        mov al, [bpp]
        cmp al, [mode_info_block.bits_per_pixel]
        jne .next_mode

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
        mov si, mode_not_found_string
        mov cx, mode_not_found_string.len
        call print_string
        .try_again:
        xor ax, ax
        int 16h
        cmp al, 'y'
        jne .check_no
        mov word [width], 0
        mov word [height], 0
        mov word [bpp], 0

        mov ax, 0E0Ah       ; Print newline
        int 10h
        mov al, 0Dh
        int 10h
        jmp input_gfx_values

        .check_no:
        cmp al, 'n'
        jne .try_again
        mov word [width], 1920      ; Take default values
        mov word [height], 1080
        mov byte [bpp], 32 
        jmp set_up_vbe 

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
        mov cx, 6                     ; Length of string

    ;; print_string: Subroutine to print a string
    ;;  Inputs:
    ;;  SI = address of string
    ;;  CX = length of string
    print_string:
        mov ah, 0Eh     ; BIOS teletype output
        .loop:
            lodsb
            int 10h
        loop .loop
        ret

    ;; input_number: Subroutine to input a number
    ;; Outputs:
    ;;   BX = number
    input_number:
        xor bx, bx
        mov cx, 10
        .next_digit:
            mov ah, 0
            int 16h         ; BIOS get keystroke, AH = scancode, AL = ascii char
            mov ah, 0Eh
            int 10h         ; BIOS teletype output

            cmp al, 08h
            jne .check_enter
            mov ax, bx
            xor dx, dx
            div cx          ; DX:AX / CX, AX = quotient, DX = remainder
            mov bx, ax
            jmp .next_digit

            .check_enter:
            cmp al, 0Dh     ; Enter key
            je .done
            sub al, '0'     ; Convert ascii number to integer
            mov ah, 0       ; Isolate AL value
            imul bx, bx, 10 ; BX *= 10
            add bx, ax      ; BX += AL
        jmp .next_digit

        .done:
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

;; String constants and length values
choose_gfx_string: db 'Input graphics mode values'
.len = ($-choose_gfx_string)    ;; Fasm needs '=' to set numeric constants
width_string: db 0Ah,0Dh,'X_Resolution: '
.len = ($-width_string)    ;; Fasm needs '=' to set numeric constants
height_string: db 0Ah,0Dh,'Y_Resolution: '
.len = ($-height_string)    ;; Fasm needs '=' to set numeric constants
bpp_string: db 0Ah,0Dh,'bpp: '
.len = ($-bpp_string)    ;; Fasm needs '=' to set numeric constants
mode_not_found_string: db 0Ah,0Dh,'Video mode not found, try again (Y) or take default 1920x1080 32bpp (N): '
.len = ($-mode_not_found_string)

;; VBE Variables
width: dw 0
height: dw 0
bpp: db 0
offset: dw 0
t_segment: dw 0	; "segment" is keyword in fasm
mode: dw 0

;; End normal 2ndstage bootloader sector padding
times 1024-($-$$) db 0

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
    times 2048-($-$$) db 0
