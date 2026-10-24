#include "kernel.h"

static volatile uint16_t *const vga = (volatile uint16_t *)0xB8000;
static uint8_t color = 0x0F;
static size_t row, col;
static uint16_t make_cell(char ch) { return (uint16_t)(uint8_t)ch | ((uint16_t)color << 8); }
void console_set_color(uint8_t fg, uint8_t bg) { color = (uint8_t)((bg << 4) | (fg & 0x0F)); }
void console_clear(void) {
    for (size_t i = 0; i < VGA_COLS * VGA_ROWS; ++i) vga[i] = make_cell(' ');
    row = col = 0;
}
static void scroll(void) {
    if (row < VGA_ROWS) return;
    for (size_t y = 1; y < VGA_ROWS; ++y)
        for (size_t x = 0; x < VGA_COLS; ++x) vga[(y - 1) * VGA_COLS + x] = vga[y * VGA_COLS + x];
    for (size_t x = 0; x < VGA_COLS; ++x) vga[(VGA_ROWS - 1) * VGA_COLS + x] = make_cell(' ');
    row = VGA_ROWS - 1;
}
void console_putc(char ch) {
    if (ch == '\n') { col = 0; ++row; }
    else if (ch == '\r') col = 0;
    else if (ch == '\b') { if (col) { --col; vga[row * VGA_COLS + col] = make_cell(' '); } }
    else {
        vga[row * VGA_COLS + col] = make_cell(ch);
        if (++col == VGA_COLS) { col = 0; ++row; }
    }
    scroll();
}
void console_write(const char *s) { if (s) while (*s) console_putc(*s++); }
void klog(const char *level, const char *message) {
    uint8_t previous = color;
    console_set_color(0x0B, 0);
    console_write("[NexOS/"); console_write(level); console_write("] ");
    console_set_color(0x0F, 0); console_write(message); console_putc('\n');
    serial_write("[NexOS/"); serial_write(level); serial_write("] ");
    serial_write(message); serial_write("\r\n");
    color = previous;
}
void kernel_panic(const char *reason) {
    cpu_disable_interrupts();
    console_set_color(0x0F, 0x04); console_clear();
    console_write(" *** NEXOS KERNEL PANIC ***\n\nErreur fatale du noyau.\n\nRaison : ");
    console_write(reason ? reason : "inconnue");
    console_write("\n\nLe systeme est arrete pour proteger les donnees.\n");
    for (;;) cpu_halt();
}
