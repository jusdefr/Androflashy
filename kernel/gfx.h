/* NexOS — rendu sur framebuffer linéaire (arme VBE 4 octets par pixel). */
#ifndef NEXOS_GFX_H
#define NEXOS_GFX_H
#include <stdint.h>
#include <stdbool.h>
#include "bootinfo.h"

#define GFX_COLOR_TRANSPARENT 0x00000000u

typedef struct {
    volatile uint32_t *base;
    uint32_t width, height;
    uint32_t pitch;       /* octets par ligne */
    bool ready;           /* false = mode texte, rendu GUI indisponible */
} gfx_t;

typedef struct { int32_t x, y, w, h; } clip_t;

bool gfx_init(const bootinfo_t *info);
void gfx_host_framebuffer(volatile uint32_t *base); /* harnais hôte uniquement */
bool gfx_ready(void);
uint32_t gfx_read_pixel(int32_t x, int32_t y);
uint32_t gfx_width(void);
uint32_t gfx_height(void);
void gfx_set_clip(int32_t x, int32_t y, int32_t w, int32_t h);
void gfx_reset_clip(void);
void gfx_fill(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void gfx_put(int32_t x, int32_t y, uint32_t color);
void gfx_text(int32_t x, int32_t y, const char *text, uint32_t fg, uint32_t bg);
void gfx_text_len(const char *text, int32_t *w);
void gfx_text_scaled(int32_t x, int32_t y, const char *text, uint32_t fg, uint32_t bg, uint32_t scale);
void gfx_text_scaled_w(int32_t x, int32_t y, const char *text, uint32_t fg, uint32_t bg, uint32_t scale);
void gfx_text_width(const char *text, uint32_t scale, int32_t *out_w);
void gfx_text_height(uint32_t scale, int32_t *out_h);
void gfx_sprite(const uint32_t *pixels, int32_t x, int32_t y, int32_t w, int32_t h);
void gfx_gradient_vertical(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t top, uint32_t bottom);
int32_t gfx_clamp_x(int32_t v);

#endif /* NEXOS_GFX_H */
