#include "gfx.h"
#include "kernel.h"
#include "string.h"

static gfx_t screen;
static clip_t clip;
static uint32_t max_x, max_y;

static bool point_ok(int32_t x, int32_t y) {
    if (x < clip.x || y < clip.y || x >= clip.x + clip.w || y >= clip.y + clip.h) return false;
    if ((uint32_t)x >= (uint32_t)screen.width || (uint32_t)y >= (uint32_t)screen.height) return false;
    return true;
}

void gfx_host_framebuffer(volatile uint32_t *base) { screen.base = base; }

bool gfx_init(const bootinfo_t *info) {
    if (!info || info->magic != BOOTINFO_MAGIC || info->version < 1) {
        screen.ready = false;
        return false;
    }
    if (info->bpp != 32 || info->fb_base == 0) {
        screen.ready = false;
        return false;
    }
#ifdef NEXOS_HOST_TEST
    if (info->fb_base == 0xFFFFFFFFu) { screen.ready = true; screen.width = info->width; screen.height = info->height; screen.pitch = info->pitch; max_x = info->width; max_y = info->height; gfx_reset_clip(); return true; }
#endif
    screen.base = (volatile uint32_t *)(uintptr_t)info->fb_base;
    screen.width = info->width;
    screen.height = info->height;
    screen.pitch = info->pitch;
    screen.ready = true;
    max_x = info->width;
    max_y = info->height;
    clip.x = 0;
    clip.y = 0;
    clip.w = (int32_t)screen.width;
    clip.h = (int32_t)screen.height;
    return true;
}

uint32_t gfx_width(void) { return screen.width; }
uint32_t gfx_height(void) { return screen.height; }
bool gfx_ready(void) { return screen.ready; }
uint32_t gfx_read_pixel(int32_t x, int32_t y) {
    if (!screen.ready || x < 0 || y < 0 || (uint32_t)x >= screen.width || (uint32_t)y >= screen.height)
        return GFX_COLOR_TRANSPARENT;
    return screen.base[y * (screen.pitch / 4) + x];
}

void gfx_set_clip(int32_t x, int32_t y, int32_t w, int32_t h) {
    int32_t ex = x + w, ey = y + h;
    if ((uint32_t)ex > max_x) ex = (int32_t)max_x;
    if ((uint32_t)ey > max_y) ey = (int32_t)max_y;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (ex < x) ex = x;
    if (ey < y) ey = y;
    clip.x = x;
    clip.y = y;
    clip.w = ex - x;
    clip.h = ey - y;
}

void gfx_reset_clip(void) { gfx_set_clip(0, 0, (int32_t)screen.width, (int32_t)screen.height); }

void gfx_put(int32_t x, int32_t y, uint32_t color) {
    if (!screen.ready || color == GFX_COLOR_TRANSPARENT) return;
    if (point_ok(x, y)) screen.base[y * (screen.pitch / 4) + x] = color;
}

void gfx_fill(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    if (!screen.ready || w <= 0 || h <= 0) return;
    int32_t x0 = x > clip.x ? x : clip.x;
    int32_t y0 = y > clip.y ? y : clip.y;
    int32_t x1 = x + w < clip.x + clip.w ? x + w : clip.x + clip.w;
    int32_t y1 = y + h < clip.y + clip.h ? y + h : clip.y + clip.h;
    if (x1 > (int32_t)max_x) x1 = (int32_t)max_x;
    if (y1 > (int32_t)max_y) y1 = (int32_t)max_y;
    if (x0 >= x1 || y0 >= y1) return;
    for (int32_t row = y0; row < y1; ++row) {
        volatile uint32_t *line = screen.base + (size_t)row * (screen.pitch / 4) + x0;
        for (int32_t col = x0; col < x1; ++col) *line++ = color;
    }
}

void gfx_text(int32_t x, int32_t y, const char *text, uint32_t fg, uint32_t bg) {
    if (!screen.ready || !text) return;
    int32_t cx = x;
    for (; *text; ++text) {
        uint8_t index = (uint8_t)*text;
        const uint8_t *rows = font8x16[index];
        for (uint32_t r = 0; r < 16; ++r) {
            uint8_t bits = rows[r];
            for (int32_t c = 0; c < 8; ++c) {
                uint32_t color = (bits & (0x80u >> c)) ? fg : bg;
                if (color != GFX_COLOR_TRANSPARENT)
                    gfx_put(cx + c, (int32_t)y + (int32_t)r, color);
            }
        }
        cx += 8;
    }
}

void gfx_text_scaled(int32_t x, int32_t y, const char *text, uint32_t fg, uint32_t bg, uint32_t scale) {
    if (!screen.ready || !text || scale == 0) return;
    int32_t cx = x;
    for (; *text; ++text, cx += 8 * (int32_t)scale) {
        uint8_t index = (uint8_t)*text;
        const uint8_t *rows = font8x16[index];
        for (uint32_t r = 0; r < 16; ++r) {
            uint8_t bits = rows[r];
            for (int32_t c = 0; c < 8; ++c) {
                uint32_t color = (bits & (0x80u >> c)) ? fg : bg;
                if (color != GFX_COLOR_TRANSPARENT)
                    gfx_fill(cx + c * (int32_t)scale, y + (int32_t)(r * scale), (int32_t)scale, (int32_t)scale, color);
            }
        }
    }
}

static int32_t text_len(const char *text, uint32_t scale) {
    size_t n = strlen(text);
    return (int32_t)(n * 8 * scale);
}

void gfx_text_width(const char *text, uint32_t scale, int32_t *out_w) {
    *out_w = text ? text_len(text, scale ? scale : 1) : 0;
}

void gfx_text_height(uint32_t scale, int32_t *out_h) { *out_h = (int32_t)(16 * (scale ? scale : 1)); }

void gfx_sprite(const uint32_t *pixels, int32_t x, int32_t y, int32_t w, int32_t h) {
    if (!screen.ready || !pixels || w <= 0 || h <= 0) return;
    for (int32_t row = 0; row < h; ++row)
        for (int32_t col = 0; col < w; ++col)
            gfx_put(x + col, y + row, pixels[row * w + col]);
}

void gfx_gradient_vertical(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t top, uint32_t bottom) {
    if (!screen.ready || w <= 0 || h <= 0) return;
    for (int32_t row = 0; row < h; ++row) {
        uint32_t value = (uint32_t)((row << 16) / (h > 0 ? h : 1));
        uint32_t r = ((top >> 16) & 0xFF) * (0x10000 - value) / 0x10000 + ((bottom >> 16) & 0xFF) * value / 0x10000;
        uint32_t g = ((top >> 8) & 0xFF) * (0x10000 - value) / 0x10000 + ((bottom >> 8) & 0xFF) * value / 0x10000;
        uint32_t b = (top & 0xFF) * (0x10000 - value) / 0x10000 + (bottom & 0xFF) * value / 0x10000;
        uint32_t color = 0xFF000000u | (r << 16) | (g << 8) | b;
        gfx_fill(x, y + row, w, 1, color);
    }
}

int32_t gfx_clamp_x(int32_t v) {
    if (v < 0) return 0;
    if ((uint32_t)v >= max_x) return (int32_t)max_x - 1;
    return v;
}
