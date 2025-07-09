;boot.asm
[ORG 0x7C00]
    xor ax, ax          ; make it zero
    mov ds, ax          ; data segment starts at zero

    mov si, msg         ; load pointer to msg

ch_loop:
    lodsb               ; load char from string into al
    or al, al           ; zero means end of string
    jz done             ; end of string reached, bail out
    mov ah, 0x0E        ; color in hi byte of word
    int 0x10            ; use bios for print
    jmp ch_loop         ; go to next char
done:

    ; reset floppy controller
    mov ah, 0
    int 0x13

    ; load the kernel from floppy
    mov ah, 0x02        ; read data from floppy
    mov al, 0x20        ; number of sectors to read
    mov bx, 0x0000      ; destination buffer address: 0x0000:0x8000
    mov es, bx
    mov bx, 0x8000
    mov dl, 0x00        ; drive number: floppy A
    mov ch, 0x00        ; track number: 0
    mov cl, 0x02        ; sector number: 2
    mov dh, 0x00        ; head number: 0
    int 0x13            ; call the BIOS to read the sector

    ; Jump to the kernel's entry point
    jmp 0x0000:0x8000

halt:
    cli                 ; disable interrupts
    hlt                 ; stop execution

msg db 'loading kernel ...', 13, 10, 0

    times 510-($-$$) db 0
    db 0x55             ; write boot sector signature at
    db 0xAA             ; and of sector