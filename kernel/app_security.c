/* NexOS - Sécurité : panneau réel (CR0/CR4/IDT/GDT/tas), sans faux badges.
 * Chaque ligne affiche la valeur mesurée au démarrage ou l'explicitation
 * « non implémenté ». */
#include "app_security.h"
#include "gfx.h"
#include "theme.h"
#include "kernel.h"
#include "string.h"

#define ROW_STEP 22

typedef struct dtr_snapshot {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) dtr_t;

typedef struct {
    uint32_t cr0, cr4;
    uint32_t efer_low;     /* MSR EFER 0xC0000080 : bit 31 = NXE */
    uint32_t idt_limit;
    uint32_t gdt_limit;
    uint32_t crc;
    bool initialized;
} security_snapshot_t;

static security_snapshot_t snap;

extern const uint8_t __image_start[];
extern const uint8_t __image_data_end[];

static void snapshot_init(void) {
    if (snap.initialized) return;
    dtr_t v = { 0, 0 };
    __asm__ volatile("movl %%cr0, %0" : "=r"(snap.cr0));
    __asm__ volatile("movl %%cr4, %0" : "=r"(snap.cr4));
    __asm__ volatile("movl $0xC0000080, %%ecx ; rdmsr" : "=a"(snap.efer_low) :: "ecx", "edx");
    __asm__ volatile("sidtl %0" : "=m"(v) :: "memory");
    snap.idt_limit = v.limit;
    v.limit = 0;
    __asm__ volatile("sgdtl %0" : "=m"(v) :: "memory");
    snap.gdt_limit = v.limit;
    /* CRC32 sur text+rodata+data (image datée du boot). */
    uint32_t crc = 0xFFFFFFFFu;
    for (const uint8_t *p = __image_start; p != __image_data_end; ++p) {
        crc ^= *p;
        for (int b = 0; b < 8; ++b)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    snap.crc = ~crc;
    snap.initialized = true;
}

static const char *hex32(uint32_t v) {
    static const char digits[] = "0123456789ABCDEF";
    static char out[12];
    out[0] = '0';
    out[1] = 'x';
    for (int i = 0; i < 8; ++i) out[2 + i] = digits[(v >> (4 * (7 - i))) & 0xFu];
    out[10] = 0;
    return out;
}


void app_security_draw(int32_t x, int32_t y, int32_t w, int32_t h) {
    (void)h;
    (void)w;
    snapshot_init();
    const theme_palette_t *t = theme();
    int32_t ly = y + 10;
    struct { const char *name; const char *value; uint32_t color; } rows[] = {
        { "Mode protégé x86 32 bits", "ACTIF", t->ok },
        { "CR0 (registre)", hex32(snap.cr0), t->text },
        { "CR4 (registre)", hex32(snap.cr4), t->text },
        { "EFER.NXE (paging/NX)", (snap.efer_low & 0x80000000u) ? "ACTIF" : "DÉSACTIVÉ (paging absent)", t->warn },
        { "IDT chargée", snap.idt_limit ? "OUI" : "NON", t->ok },
        { "GDT chargée", snap.gdt_limit ? "OUI" : "NON", t->ok },
        { "Paging / W^X", "NON IMPLÉMENTÉ (étape 2)", t->bad },
        { "Ring 3 / syscall utilisateur", "NON IMPLÉMENTÉ (étape 2)", t->bad },
        { "Pile réseau + pare-feu", "NON IMPLÉMENTÉ (étape 3)", t->bad },
        { "Chiffrement disque", "NON IMPLÉMENTÉ (pas de FS)", t->bad },
        { "CRC32 sections noyau", hex32(snap.crc), t->text },
        { "Tas noyau", "64 KiB statique, fusion des blocs libres", t->text },
    };
    for (uint32_t r = 0; r < sizeof(rows) / sizeof(rows[0]); ++r) {
        gfx_text(x + 20, ly + (int32_t)(r * ROW_STEP), rows[r].name, t->text, GFX_COLOR_TRANSPARENT);
        gfx_text(x + 330, ly + (int32_t)(r * ROW_STEP), rows[r].value, rows[r].color, GFX_COLOR_TRANSPARENT);
    }
    gfx_text(x + 20, ly + (int32_t)(sizeof(rows) / sizeof(rows[0])) * ROW_STEP + 10,
             "Prototype expérimental : pas un système sécurisé.", t->text_muted, GFX_COLOR_TRANSPARENT);
}

void app_security_key(char key) {
    (void)key; /* lecture seule : aucune interaction prévue */
}
