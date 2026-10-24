#include "kernel.h"

static char line[96];
static size_t length;
static void puts_nl(const char *s) { console_write(s); console_putc('\n'); }
static int equals(const char *a, const char *b) {
    while (*a && *b && *a == *b) { ++a; ++b; }
    return *a == *b;
}
static void number(uint32_t value) {
    char digits[11]; unsigned n = 0;
    if (!value) { console_putc('0'); return; }
    while (value && n < sizeof(digits)) { digits[n++] = (char)('0' + value % 10u); value /= 10u; }
    while (n) console_putc(digits[--n]);
}
static void help(void) {
    puts_nl("Commandes NexOS :");
    puts_nl("  help    afficher cette aide");
    puts_nl("  about   version et architecture");
    puts_nl("  clear   effacer l'ecran");
    puts_nl("  uptime  secondes depuis le demarrage");
    puts_nl("  mem     statut du tas noyau");
    puts_nl("  tasks   nombre de taches preemptives");
    puts_nl("  reboot  redemarrer la machine");
    puts_nl("  halt    arreter le processeur");
}
static void execute(const char *cmd) {
    while (*cmd == ' ') ++cmd;
    if (!*cmd) return;
    serial_write("SHELL> "); serial_write(cmd); serial_write("\r\n");
    if (equals(cmd, "help")) help();
    else if (equals(cmd, "about")) {
        puts_nl("NexOS 0.1.0-dev | noyau x86 32 bits | C freestanding | BIOS");
        serial_write("SHELL_RESULT NexOS 0.1.0-dev x86 32 bits\r\n");
    }
    else if (equals(cmd, "clear")) console_clear();
    else if (equals(cmd, "uptime")) {
        console_write("Temps actif : "); number(timer_ticks() / 100u); puts_nl(" seconde(s)");
    } else if (equals(cmd, "mem")) {
        void *p = kmalloc(64);
        if (!p) puts_nl("Tas noyau : allocation echouee");
        else { kfree(p); puts_nl("Tas noyau : 64 KiB, allocation et liberation OK"); }
    } else if (equals(cmd, "tasks")) {
        puts_nl("Ordonnanceur PIT : preemptif, priorites, piles noyau statiques");
    } else if (equals(cmd, "reboot")) {
        puts_nl("Redemarrage...");
        cpu_disable_interrupts();
        while (inb(0x64) & 0x02) { }
        outb(0x64, 0xFE);
        for (;;) cpu_halt();
    } else if (equals(cmd, "halt")) {
        puts_nl("Arret du processeur."); cpu_disable_interrupts(); for (;;) cpu_halt();
    } else {
        console_write("Commande inconnue : "); puts_nl(cmd); puts_nl("Tapez 'help' pour obtenir la liste des commandes.");
    }
}
static void prompt(void) { console_set_color(0x0B, 0); console_write("nexos"); console_set_color(0x08, 0); console_write(" > "); console_set_color(0x0F, 0); }
void shell_run(void) {
    line[0] = 0; length = 0;
    puts_nl("NexOS Desktop runtime minimal. Tapez 'help' pour les commandes.");
    prompt();
    for (;;) {
        char ch;
        if (!keyboard_read(&ch)) { __asm__ volatile("hlt"); continue; }
        serial_putc(ch);
        if (ch == '\n') {
            console_putc('\n'); line[length] = 0; execute(line); length = 0; prompt();
        } else if (ch == '\b') {
            if (length) { --length; console_putc('\b'); }
        } else if (ch >= 32 && ch < 127 && length + 1 < sizeof(line)) {
            line[length++] = ch; console_putc(ch);
        }
    }
}
