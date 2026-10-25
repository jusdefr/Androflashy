; NexOS — étage 1 du chargeur (512 octets, MBR).
; Charge l'étage 2 (6 secteurs) à 0x7E00 et lui passe la main en mode réel.
BITS 16
ORG 0x7C00

STAGE2_SEG equ 0x07E0
STAGE2_SECTORS equ 6

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov si, msg_boot
    call print_string

    mov ax, STAGE2_SEG
    mov es, ax
    xor bx, bx
    mov ax, 0x0200 + STAGE2_SECTORS     ; lire 6 secteurs d'un coup
    mov cx, 0x0002                      ; piste 0, secteur 2
    xor dx, dx                          ; tête 0
    int 0x13
    jnc stage2_ok
    ; Retentie une fois après un reset du disque.
    xor ax, ax
    mov dl, [boot_drive]
    int 0x13
    mov ax, STAGE2_SEG
    mov es, ax
    xor bx, bx
    mov ax, 0x0200 + STAGE2_SECTORS
    mov cx, 0x0002
    xor dx, dx
    int 0x13
    jc disk_error
stage2_ok:
    mov si, msg_loaded
    call print_string
    mov dl, [boot_drive]
    jmp 0x07E0:0x0000               ; entrée étage 2 : CS=DS=0x07E0

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
msg_boot:   db "NexOS BIOS boot", 13, 10, 0
msg_loaded: db "Etage 2 charge...", 13, 10, 0
msg_error:  db "NexOS: erreur de lecture disque", 13, 10, 0

times 510 - ($ - $$) db 0
dw 0xAA55
