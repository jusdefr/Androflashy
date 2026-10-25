#include "kernel.h"
#include "bootinfo.h"
#include "desktop.h"
#include "gfx.h"
#include "theme.h"
#include "../drivers/mouse.h"

static bootinfo_t *const bi = (bootinfo_t *)BOOTINFO_ADDR;

static void serial_boot_status(const char *text) {
    serial_write("[NexOS/INFO] ");
    serial_write(text);
    serial_write("\r\n");
}

void kmain(void) {
    serial_init();
    console_set_color(0x0F, 0x01);
    console_clear();
    console_write("  _   _           ___  ____\n");
    console_write(" | \\ | | _____  _/ _ \\/ ___|\n");
    console_write(" |  \\| |/ _ \\ / / | \\___ \\\n");
    console_write(" | |\\  |  __/>  <| |_| |___) |\n");
    console_write(" |_| \\_|\\___/_/\\_\\___/|____/\n\n");
    console_set_color(0x0B, 0);
    console_write("  NexOS 0.1.0-dev | noyau x86 32 bits\n");
    console_set_color(0x07, 0);
    console_write("  OS pedagogique amorce en mode protege BIOS\n\n");
    serial_write("NexOS boot\r\n");
    serial_boot_status("initialisation du noyau");
    memory_init();
    serial_boot_status("allocateur memoire initialise");
    interrupts_init();
    keyboard_init();
    pit_init(100);
    scheduler_init();
    serial_boot_status("tache shell prete");

    if (gfx_init(bi)) {
        /* Chemin graphique : framebuffer VBE, bureau fenêtré, souris PS/2. */
        console_write("  framebuffer VBE 32 bpp actif\n");
        theme_init();
        mouse_init();
        cpu_enable_interrupts();
        klog("INFO", "bureau graphique lance (VBE)");
        desktop_run();
        kernel_panic("desktop_run ne devrait pas revenir");
    }
    klog("INFO", "mode texte : clavier PS/2, shell diagnostique");
    cpu_enable_interrupts();
    shell_run();
    kernel_panic("shell_run ne devrait pas revenir");
}
