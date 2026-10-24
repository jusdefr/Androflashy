#include "../kernel/kernel.h"
static volatile uint32_t ticks;
void pit_init(uint32_t hz) {
    if (hz == 0 || hz > 1000) hz = 100;
    uint32_t divisor = 1193182u / hz;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
    ticks = 0;
    klog("INFO", "PIT configure : tick preemptif 100 Hz");
}
uint32_t timer_ticks(void) { return ticks; }
void timer_advance(void) { ++ticks; }
