#include "kernel.h"

extern void isr0(void); extern void isr1(void); extern void isr2(void); extern void isr3(void);
extern void isr4(void); extern void isr5(void); extern void isr6(void); extern void isr7(void);
extern void isr8(void); extern void isr9(void); extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void); extern void isr15(void);
extern void isr16(void); extern void isr17(void); extern void isr18(void); extern void isr19(void);
extern void isr20(void); extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void); extern void isr27(void);
extern void isr28(void); extern void isr29(void); extern void isr30(void); extern void isr31(void);
extern void isr32(void); extern void isr33(void); extern void isr34(void); extern void isr35(void);
extern void isr36(void); extern void isr37(void); extern void isr38(void); extern void isr39(void);
extern void isr40(void); extern void isr41(void); extern void isr42(void); extern void isr43(void);
extern void isr44(void); extern void isr45(void); extern void isr46(void); extern void isr47(void);
extern void isr128(void);

static idt_entry_t idt[IDT_ENTRIES];
static descriptor_ptr_t idtr;
static void (*const stubs[48])(void) = {
    isr0,isr1,isr2,isr3,isr4,isr5,isr6,isr7,isr8,isr9,isr10,isr11,
    isr12,isr13,isr14,isr15,isr16,isr17,isr18,isr19,isr20,isr21,isr22,isr23,
    isr24,isr25,isr26,isr27,isr28,isr29,isr30,isr31,isr32,isr33,isr34,isr35,
    isr36,isr37,isr38,isr39,isr40,isr41,isr42,isr43,isr44,isr45,isr46,isr47
};
void outb(uint16_t port, uint8_t value) { __asm__ volatile("outb %0,%1" :: "a"(value), "Nd"(port)); }
uint8_t inb(uint16_t port) { uint8_t v; __asm__ volatile("inb %1,%0" : "=a"(v) : "Nd"(port)); return v; }
static void idt_set(uint8_t n, uintptr_t addr, uint8_t flags) {
    idt[n].offset_low = (uint16_t)addr; idt[n].selector = 0x08;
    idt[n].zero = 0; idt[n].flags = flags; idt[n].offset_high = (uint16_t)(addr >> 16);
}

void interrupts_init(void) {
    idtr.limit = (uint16_t)(sizeof(idt) - 1u);
    idtr.base = (uint32_t)(uintptr_t)idt;
    for (unsigned i = 0; i < 48; ++i) idt_set((uint8_t)i, (uintptr_t)stubs[i], 0x8E);
    idt_set(128, (uintptr_t)isr128, 0x8E); /* noyau ring 0 uniquement */
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0xFC); outb(0xA1, 0xFF);
    idt_load(&idtr);
    klog("INFO", "IDT chargee ; exceptions, IRQ et syscall interne actifs");
}

void isr_handler(registers_t *r) {
    if (r->int_no < 32) {
        if (r->int_no == 3) return;
        static const char *const names[32] = {
            "division par zero","debug","NMI","point d'arret","overflow","limite depassee",
            "opcode invalide","peripherique indisponible","double faute","coprocesseur segment overrun",
            "TSS invalide","segment absent","pile invalide","protection generale","page absente",
            "reserve","erreur x87","alignement","erreur machine","SIMD flottant","virtualisation",
            "controle protection","reserve22","reserve23","reserve24","reserve25","reserve26",
            "reserve27","hyperviseur","VMM communication","securite","reserve31"};
        kernel_panic(names[r->int_no]);
    }
    if (r->int_no >= IRQ_BASE && r->int_no < IRQ_BASE + 16) {
        uint8_t irq = (uint8_t)(r->int_no - IRQ_BASE);
        if (irq == 0) timer_advance();
        else if (irq == 1) keyboard_irq();
        if (irq >= 8) outb(0xA0, 0x20);
        outb(0x20, 0x20);
    }
}

registers_t *syscall_handler(registers_t *r) {
    switch (r->eax) {
        case 0: r->eax = 0; break;
        case 1: r->eax = timer_ticks(); break;
        default: r->eax = (uint32_t)-38; break;
    }
    return r;
}
