#ifndef NEXOS_KERNEL_H
#define NEXOS_KERNEL_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VGA_COLS 80u
#define VGA_ROWS 25u
#define IDT_ENTRIES 256u
#define IRQ_BASE 32u
#define KEYBUF_SIZE 128u

typedef struct __attribute__((packed)) { uint16_t limit; uint32_t base; } descriptor_ptr_t;
typedef struct __attribute__((packed)) {
    uint16_t offset_low, selector;
    uint8_t zero, flags;
    uint16_t offset_high;
} idt_entry_t;
/* Cadre pile : ordre exact correspondant à entry.asm et pusha. */
typedef struct {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code, eip, cs, eflags;
} registers_t;

void kmain(void);
void console_clear(void);
void console_write(const char *text);
void console_putc(char ch);
void console_set_color(uint8_t fg, uint8_t bg);
void serial_init(void);
void serial_putc(char ch);
void serial_write(const char *text);
void klog(const char *level, const char *message);
void kernel_panic(const char *reason);
void interrupts_init(void);
void isr_handler(registers_t *regs);
registers_t *syscall_handler(registers_t *regs);
void scheduler_init(void);

void pit_init(uint32_t hz);
void timer_advance(void);
uint32_t timer_ticks(void);
void memory_init(void);
void *kmalloc(size_t size);
void kfree(void *ptr);
void keyboard_init(void);
int keyboard_read(char *out);
void keyboard_irq(void);
void shell_run(void);
void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);
void idt_load(const descriptor_ptr_t *ptr);
void gdt_flush(const descriptor_ptr_t *ptr);
void cpu_halt(void);
void cpu_enable_interrupts(void);
void cpu_disable_interrupts(void);
extern const uint8_t font8x16[256][16];
#endif
