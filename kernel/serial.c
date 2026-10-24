#include "kernel.h"

void serial_init(void) {
    outb(0x3F8 + 1, 0x00); /* couper IRQ UART */
    outb(0x3F8 + 3, 0x80); /* accès diviseur */
    outb(0x3F8 + 0, 0x01); /* 115200 bauds */
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03); /* 8N1 */
    outb(0x3F8 + 2, 0xC7); /* FIFO */
    outb(0x3F8 + 4, 0x0B);
}
void serial_putc(char ch) {
    for (uint32_t spin = 0; spin < 1000000; ++spin)
        if (inb(0x3F8 + 5) & 0x20) { outb(0x3F8, (uint8_t)ch); return; }
}
void serial_write(const char *s) { if (s) while (*s) serial_putc(*s++); }
