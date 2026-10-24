; NexOS — chargeur d'amorçage BIOS pour disquette 1,44 Mo.
; Charge 100 secteurs contigus depuis le secteur 2 vers 0x10000, puis passe
; en mode protégé 32 bits et saute au noyau.
BITS 16
ORG 0x7C00

KERNEL_SEG      equ 0x1000
KERNEL_SECTORS  equ 100
SECTORS_PER_TRK equ 18
HEADS           equ 2

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [boot_drive], dl

    mov si, msg_boot
    call print_string

    ; Active A20 via le contrôleur de système rapide (port 0x92).
    in al, 0x92
    or al, 2
    and al, 0xFE
    out 0x92, al

    mov ax, KERNEL_SEG
    mov es, ax
    xor bx, bx
    mov dword [lba], 1
    mov cx, KERNEL_SECTORS
.load_loop:
    push cx
    call read_sector
    jc disk_error
    add bx, 512
    inc dword [lba]
    pop cx
    loop .load_loop

    mov si, msg_loaded
    call print_string
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:protected_mode

; Lit un secteur CHS. ES:BX est préservé comme destination à chaque appel.
read_sector:
    push ax
    push cx
    push dx
    push si
    push di
    mov si, 3
.retry:
    mov ax, [lba]
    xor dx, dx
    mov cx, SECTORS_PER_TRK
    div cx                      ; AX = piste, DX = secteur (0..17)
    mov [sector_tmp], dl
    inc byte [sector_tmp]       ; CHS commence les secteurs à 1
    xor dx, dx
    mov cx, HEADS
    div cx                      ; AX = cylindre, DX = tête
    mov ch, al
    mov dh, dl
    mov cl, [sector_tmp]
    mov dl, [boot_drive]
    mov ax, 0x0201
    int 0x13
    jnc .done
    xor ax, ax
    mov dl, [boot_drive]
    int 0x13
    dec si
    jnz .retry
    stc
.done:
    pop di
    pop si
    pop dx
    pop cx
    pop ax
    ret

print_string:
    push ax
    push bx
    mov ah, 0x0E
    xor bx, bx
.next:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .next
.done:
    pop bx
    pop ax
    ret

disk_error:
    mov si, msg_error
    call print_string
.halt:
    cli
    hlt
    jmp .halt

boot_drive: db 0
lba:        dd 1
sector_tmp: db 0
msg_boot:   db "NexOS BIOS boot", 13, 10, 0
msg_loaded: db "Kernel charge, entree en mode protege...", 13, 10, 0
msg_error:  db "NexOS: erreur de lecture disque", 13, 10, 0

align 8
gdt_start:
    dq 0
    dq 0x00CF9A000000FFFF       ; code 32 bits, ring 0
    dq 0x00CF92000000FFFF       ; données 32 bits, ring 0
gdt_end:
gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

BITS 32
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    jmp 0x08:0x10000

times 510 - ($ - $$) db 0
dw 0xAA55
