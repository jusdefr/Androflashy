/* NexOS — thème du bureau : palette, fond d'écran et couleur d'accent. */
#ifndef NEXOS_THEME_H
#define NEXOS_THEME_H
#include <stdint.h>

typedef struct {
    uint32_t wallpaper_top, wallpaper_bottom;
    uint32_t taskbar_bg, taskbar_text;
    uint32_t accent, accent_soft;
    uint32_t window_bg, window_border, titlebar_bg, titlebar_text;
    uint32_t menu_bg, menu_border, menu_text, menu_hover_bg;
    uint32_t close_bg, close_text;
    uint32_t text, text_muted, input_bg, input_border, ok, warn, bad;
    const char *name_wallpaper;
    const char *name_accent;
} theme_palette_t;

void theme_init(void);
const theme_palette_t *theme(void);
void theme_next_wallpaper(void);
void theme_prev_wallpaper(void);
void theme_next_accent(void);
void theme_prev_accent(void);
uint32_t theme_wallpaper_style(void); /* 0 dégradé, 1 uni */

#endif /* NEXOS_THEME_H */
