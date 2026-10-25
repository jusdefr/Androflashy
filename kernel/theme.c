#include "theme.h"
#include "kernel.h"

/* Trois fonds d'écran et quatre accents ; les palettes restent sombres et lisibles. */
static const uint32_t wall_tops[3]    = { 0xFF16263F, 0xFF0B1020, 0xFF101A2A };
static const uint32_t wall_bots[3]    = { 0xFF080E1A, 0xFF0B1020, 0xFF0A1F1C };
static const char *wall_names[3]      = { "Aube technique", "Nuit profonde", "Sombre cyber" };
static const uint32_t accents[4]      = { 0xFF3B82F6, 0xFF22C55E, 0xFFA855F7, 0xFFF97316 };
static const char *accent_names[4]    = { "Bleu", "Vert", "Violet", "Orange" };

static theme_palette_t pal;
static uint32_t wallpaper = 0;
static uint32_t accent = 0;

void theme_init(void) {
    pal.wallpaper_top = wall_tops[wallpaper];
    pal.wallpaper_bottom = wall_bots[wallpaper];
    pal.taskbar_bg = 0xE3101118;
    pal.taskbar_text = 0xFFE6EBF4;
    pal.accent = accents[accent];
    pal.accent_soft = 0xFF1E2A44;
    pal.window_bg = 0xFFF7F8FC;
    pal.window_border = 0xFF3A4257;
    pal.titlebar_bg = 0xFF212837;
    pal.titlebar_text = 0xFFF2F5FB;
    pal.menu_bg = 0xFF171C28;
    pal.menu_border = 0xFF2E3547;
    pal.menu_text = 0xFFDCE2EF;
    pal.menu_hover_bg = 0xFF2A3348;
    pal.close_bg = 0xFFD64545;
    pal.close_text = 0xFFFFF6F6;
    pal.text = 0xFF1F2533;
    pal.text_muted = 0xFF6B7385;
    pal.input_bg = 0xFFFFFFFF;
    pal.input_border = 0xFFC6CBD8;
    pal.ok = 0xFF1D9E57;
    pal.warn = 0xFFC28A00;
    pal.bad = 0xFFC0392B;
    pal.name_wallpaper = wall_names[wallpaper];
    pal.name_accent = accent_names[accent];
}

const theme_palette_t *theme(void) { return &pal; }

static void refresh(void) {
    pal.wallpaper_top = wall_tops[wallpaper];
    pal.wallpaper_bottom = wall_bots[wallpaper];
    pal.accent = accents[accent];
    pal.name_wallpaper = wall_names[wallpaper];
    pal.name_accent = accent_names[accent];
}

void theme_next_wallpaper(void) { wallpaper = (wallpaper + 1u) % 3u; refresh(); }
void theme_prev_wallpaper(void) { wallpaper = (wallpaper + 2u) % 3u; refresh(); }
void theme_next_accent(void) { accent = (accent + 1u) % 4u; refresh(); }
void theme_prev_accent(void) { accent = (accent + 3u) % 4u; refresh(); }
uint32_t theme_wallpaper_style(void) { return wallpaper == 1u ? 1u : 0u; }
