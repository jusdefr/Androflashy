; NexOS — étage 2 chargé à 0x07E00 par l'étage 1, entré via 0x07E0:0x0000.
; Convention : DS=CS=0x07E0 (données de l'étage), ES=0 pour les adresses fixes
; (struct bootinfo à 0x0600, ModeInfoBlock VBE à 0x0700).
BITS 16
ORG 0x7E00

STAGE2_SECTORS  equ 6
KERNEL_SECTORS  equ 300
KERNEL_SEG      equ 0x1000
SECTORS_PER_TRK equ 18
HEADS           equ 2

BOOTINFO        equ 0x0600   ; struct bootinfo (kernel/bootinfo.h)
MODEINFO        equ 0x0700   ; buffer ModeInfoBlock VBE (256 octets)

start:
    mov ax, cs
    mov ds, ax                      ; DS = 0x07E0, données de l'étage 2
    mov [boot_drive], dl

    ; ES=0 : accès aux buffers d'adresses fixes.
    xor ax, ax
    mov es, ax

    ; --- Sonde VBE : itère sur la liste des candidats jusqu'à la sentinelle 0.
    mov si, mode_list
.probe_loop:
    cmp byte [si], 0
    je .text_fallback
    mov dx, [si]                    ; DX = mode candidat
    mov di, MODEINFO
    mov ax, 0x4F01                  ; VBE : informations du mode (ES:DI)
    int 0x10
    cmp ax, 0x004F
    jne .next_mode
    test byte [es:di], 0x81         ; supporté (D0) + framebuffer linéaire (D7)
    jz .next_mode
    mov ax, 0x4F02                  ; VBE : activer le mode
    mov bx, dx
    or bx, 0x4000                   ; demander l'accès framebuffer linéaire
    int 0x10
    cmp ax, 0x004F
    jne .next_mode
    mov [mode], dx
    mov ax, [es:di+16]              ; BytesPerScanline
    mov [pitch], ax
    mov ax, [es:di+18]              ; XResolution
    mov [width], ax
    mov ax, [es:di+20]              ; YResolution
    mov [height], ax
    mov al, [es:di+25]              ; BitsPerPixel
    mov [bpp], al
    mov eax, [es:di+40]             ; PhysBasePtr
    mov [fb_base], eax
    jmp .info

.next_mode:
    add si, 2
    jmp .probe_loop

.text_fallback:
    mov word [mode], 0
    mov word [pitch], 0
    mov word [width], 0
    mov word [height], 0
    mov byte [bpp], 0
    mov dword [fb_base], 0

.info:
    mov di, BOOTINFO
    xor ax, ax
    mov cx, 8                       ; 32 octets à zéro
.zero:
    mov [es:di], ax
    mov [es:di+2], ax
    add di, 4
    loop .zero
    mov di, BOOTINFO
    mov word [es:di], 0x5853        ; 'S','X'
    mov word [es:di+2], 0x4E45      ; 'E','N'
    mov word [es:di+4], 1           ; version
    mov eax, [fb_base]
    mov [es:di+8], eax              ; pitch/bootinfo.offset 8
    mov ax, [pitch]
    mov [es:di+12], ax              ; pitch (u32, poids fort déjà zéro)
    mov ax, [width]
    mov [es:di+16], ax              ; width
    mov ax, [height]
    mov [es:di+20], ax              ; height
    mov al, [bpp]
    mov [es:di+24], al              ; bpp
    mov ax, [mode]
    mov [es:di+28], ax              ; mode

kernel_load:
    ; --- Charge le noyau : KERNEL_SECTORS secteurs depuis LBA STAGE2_SECTORS+1.
    mov ax, KERNEL_SEG
    mov es, ax
    xor bx, bx
    mov dword [lba], STAGE2_SECTORS + 1
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
    in al, 0x92
    or al, 2
    and al, 0xFE
    out 0x92, al
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
    div cx                          ; AX = piste, DX = secteur (0..17)
    mov [sector_tmp], dl
    inc byte [sector_tmp]           ; CHS commence les secteurs à 1
    xor dx, dx
    mov cx, HEADS
    div cx                          ; AX = cylindre, DX = tête
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
mode_list:  dw 0x143, 0x142, 0x141, 0 ; sentinelle 0
pitch:      dw 0
width:      dw 0
height:     dw 0
bpp:        db 0
mode:       dw 0
fb_base:    dd 0

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
