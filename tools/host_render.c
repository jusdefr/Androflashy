/* NexOS — harnais de rendu hôte (test dev) : framebuffer mémoire + BMP.NonNull
 * Ne pas embarquer dans l'image de boot ; sert à valider gfx/app fontes et layout. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "kernel.h"
#include "bootinfo.h"
#include "gfx.h"
#include "theme.h"
#include "desktop.h"
#include "../kernel/app_browser.h"
#include "../kernel/app_settings.h"
#include "../kernel/app_terminal.h"

#define VW 1024
#define VH 768
static uint32_t fb[VW * VH];

static uint32_t fake_ticks = 372100;
uint32_t timer_ticks(void) { return &fake_ticks ? ++fake_ticks : 0; }
void *kmalloc(size_t n) { return malloc(n); }
void kfree(void *p) { free(p); }
void klog(const char *l, const char *m) { (void)l; (void)m; }
void serial_init(void) { }
void serial_putc(char c) { (void)c; }
void serial_write(const char *t) { (void)t; }
void console_clear(void) { }
void console_write(const char *t) { (void)t; }
void console_putc(char c) { (void)c; }
void console_set_color(uint8_t a, uint8_t b) { (void)a; (void)b; }
void outb(uint16_t p, uint8_t v) { (void)p; (void)v; }
uint8_t inb(uint16_t p) { (void)p; return 0; }
void cpu_halt(void) { }
void cpu_enable_interrupts(void) { }
void cpu_disable_interrupts(void) { }
void idt_load(const descriptor_ptr_t *p) { (void)p; }
void gdt_flush(const descriptor_ptr_t *p) { (void)p; }
void pit_init(uint32_t hz) { (void)hz; }
void timer_advance(void) { ++fake_ticks; }
void interrupts_init(void) { }
void keyboard_init(void) { }
int keyboard_read(char *out) { (void)out; return 0; }
void kernel_panic(const char *r) { fprintf(stderr, "PANIC: %s\n", r); exit(1); }
void mouse_init(void) { }
/* pas de stub font8x16 : la vraie table est compilée depuis kernel/font8x16.c */
void desktop_request_redraw(void) { }
void desktop_close_window(int i) { (void)i; }

/* Test UX réel : fonction de la barre des tâches + contenu des fenêtres. */
static void chrome_title(int32_t x, int32_t y, int32_t w, const char *title, uint32_t accent) {
    const theme_palette_t *t = theme();
    gfx_fill(x - 1, y - 1, w + 2, 0, accent); /* cadre haut (marker) */
    gfx_fill(x, y, w, 24, t->titlebar_bg);
    gfx_text(x + 10, y + 4, title, t->titlebar_text, GFX_COLOR_TRANSPARENT);
}

/* Fenêtre complète façon desktop.c : titre, contenu découpé, poignée. */
static void render_window(int32_t x, int32_t y, int32_t w, int32_t h, const char *title,
                          void (*draw)(int32_t, int32_t, int32_t, int32_t)) {
    const theme_palette_t *t = theme();
    gfx_fill(x - 1, y - 1, w + 2, h + 2, t->window_border);
    chrome_title(x, y, w, title, t->accent);
    gfx_fill(x, y + 24, w, h - 24, t->window_bg);
    gfx_set_clip(x, y + 24, w, h - 24);
    draw(x, y + 24, w, h - 24);
    gfx_reset_clip();
    /* poignée de redimensionnement bas-droite (hachures diagonales) */
    for (int32_t i = 4; i < 14; i += 4)
        for (int32_t d = 0; d < 14 - i; ++d) {
            gfx_put(x + w - 14 + i + d, y + h - 1 - i + d, t->menu_border);
            gfx_put(x + w - 1 - d, y + h - 14 + i + d - 1, t->menu_border);
        }
}

static void write_bmp(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    const uint32_t row = VW * 3;
    const uint32_t pad = (4 - (row % 4)) % 4;
    const uint32_t data = (row + pad) * VH;
    uint8_t header[54] = { 'B', 'M' };
    memcpy(header + 2, &(const uint32_t){ 54 + data }, 4);
    header[10] = 54;
    header[14] = 40;
    memcpy(header + 18, &(const int32_t){ VW }, 4);
    memcpy(header + 22, &(const int32_t){ VH }, 4);
    header[26] = 1;
    header[28] = 24;
    fwrite(header, 1, sizeof(header), f);
    uint8_t *line = calloc(row + pad, 1);
    for (int32_t y = VH - 1; y >= 0; --y) {
        for (int32_t x = 0; x < VW; ++x) {
            uint32_t p = fb[y * VW + x];
            line[x * 3 + 0] = (uint8_t)(p & 0xFF);
            line[x * 3 + 1] = (uint8_t)((p >> 8) & 0xFF);
            line[x * 3 + 2] = (uint8_t)((p >> 16) & 0xFF);
        }
        fwrite(line, 1, row + pad, f);
    }
    free(line);
    fclose(f);
}

int main(int argc, char **argv) {
    const char *out = argc > 1 ? argv[1] : "artifacts/desktop_preview.bmp";
    bootinfo_t bi = { BOOTINFO_MAGIC, 1, 0xFFFFFFFFu, VW * 4, VW, VH, 32, 99 };
    gfx_host_framebuffer(fb);
    if (!gfx_init(&bi)) { fprintf(stderr, "gfx_init a échoué\n"); return 1; }
    theme_init();


    const theme_palette_t *t = theme();
    gfx_gradient_vertical(0, 0, VW, VH, t->wallpaper_top, t->wallpaper_bottom);
    /* Faux bureau : trois fenêtres, tailles variées, poignées visibles. */
        render_window(20, 30, 376, 224, "Terminal", app_terminal_draw);
        render_window(430, 30, 500, 360, "Paramètres", app_settings_draw);
        render_window(80, 406, 860, 306, "NexBrowser", app_browser_draw);
    /* Barre des tâches simplifiée */
    gfx_fill(0, VH - 32, VW, 32, t->taskbar_bg);
    gfx_text(16, VH - 26, "Démarrer", t->taskbar_text, GFX_COLOR_TRANSPARENT);
    int32_t ox = 130;
    const char *buttons[3] = { "Terminal", "Paramètres", "NexBrowser" };
    for (int i = 0; i < 3; ++i) {
        int32_t bw = 8 * (int32_t)strlen(buttons[i]) + 20;
        gfx_fill(ox, VH - 27, bw, 22, t->accent_soft);
        gfx_text(ox + 9, VH - 23, buttons[i], t->taskbar_text, GFX_COLOR_TRANSPARENT);
        ox += bw + 8;
    }
    gfx_text(VW - 60, VH - 26, "12:58", t->taskbar_text, GFX_COLOR_TRANSPARENT);

    write_bmp(out);
    printf("rendu hôte terminé : %s\n", out);
    return 0;
}
