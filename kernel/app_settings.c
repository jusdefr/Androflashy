/* NexOS - Paramètres : apparence (fond d'écran, accent), navigateur par
 * défaut (NexBrowser, unique moteur natif) et à propos avec valeurs réelles. */
#include "app_settings.h"
#include "gfx.h"
#include "theme.h"
#include "kernel.h"
#include "string.h"

typedef enum { TAB_APPEARANCE = 0, TAB_BROWSER, TAB_ABOUT } tab_kind_t;

#define ROW_STEP 30
#define TAB_STEP 36

typedef struct {
    tab_kind_t tab;
    uint32_t row;
} settings_state_t;

static settings_state_t st;

static const uint32_t row_counts[3] = { 2u, 1u, 4u };

static char num_buf[12];
static const char *to_num(uint32_t v) {
    if (!v) return "0";
    char digits[12];
    unsigned n = 0;
    while (v && n < sizeof(digits)) {
        digits[n++] = (char)('0' + v % 10u);
        v /= 10u;
    }
    unsigned i = 0;
    for (--n; n + 1 > 0; --n) num_buf[i++] = digits[n];
    num_buf[i] = 0;
    return num_buf;
}

static const char *mem_line_compose(const char *label, const char *value, char *out, uint32_t n) {
    (void)n;
    strcpy(out, label);
    strcat(out, value);
    return out;
}

void app_settings_draw(int32_t x, int32_t y, int32_t w, int32_t h) {
    const theme_palette_t *t = theme();
    /* Onglets */
    static const char *tabs[3] = { "Apparence", "Navigateur", "À propos" };
    for (uint32_t r = 0; r < 3; ++r) {
        uint32_t bg = (st.tab == (tab_kind_t)r) ? t->accent : t->input_bg;
        uint32_t fg = (st.tab == (tab_kind_t)r) ? 0xFFFFFFFFu : t->text;
        gfx_fill(x + 14 + (int32_t)r * 120, y + 10, 108, 26, bg);
        gfx_text(x + 34 + (int32_t)r * 120, y + 15, tabs[r], fg, GFX_COLOR_TRANSPARENT);
    }
    int32_t ly = y + 64;
    if (st.tab == TAB_APPEARANCE) {
        struct { const char *label; const char *value; } rows[2] = {
            { "Fond d'écran", t->name_wallpaper },
            { "Accent", t->name_accent },
        };
        for (uint32_t r = 0; r < 2; ++r) {
            int32_t ry = ly + (int32_t)(r * ROW_STEP);
            uint32_t label_color = t->text, value_color = t->text_muted;
            if (st.row == r) {
                gfx_fill(x + 12, ry - 6, w - 24, ROW_STEP - 4, t->accent_soft);
                label_color = 0xFFF2F5FB;
                value_color = 0xFFC9D2E4;
            }
            gfx_text(x + 24, ry, rows[r].label, label_color, GFX_COLOR_TRANSPARENT);
            gfx_text(x + 250, ry, rows[r].value, value_color, GFX_COLOR_TRANSPARENT);
        }
        gfx_text(x + 24, ly + 10 + 2 * ROW_STEP, "Flèches gauche/droite : suivant/précédent", t->text_muted, GFX_COLOR_TRANSPARENT);
    } else if (st.tab == TAB_BROWSER) {
        if (st.row == 0) gfx_fill(x + 12, ly - 6, w - 24, ROW_STEP - 4, t->accent_soft);
        gfx_text(x + 24, ly, "Navigateur par défaut", t->text, GFX_COLOR_TRANSPARENT);
        gfx_text(x + 250, ly, "NexBrowser (natif, hors ligne)", t->text_muted, GFX_COLOR_TRANSPARENT);
        gfx_text(x + 24, ly + ROW_STEP, "Aucun autre moteur n'est intégré à NexOS.", t->text_muted, GFX_COLOR_TRANSPARENT);
        gfx_text(x + 24, ly + 2 * ROW_STEP, "Une pile réseau + TLS arriverait en étape 3.", t->text_muted, GFX_COLOR_TRANSPARENT);
    } else {
        char line[64];
        strcpy(line, "NexOS 0.1.0-dev . bureau graphique x86 32 bits (BIOS+VBE)");
        gfx_text(x + 24, ly, line, t->text, GFX_COLOR_TRANSPARENT);
        strcpy(line, "Fenêtre : ");
        strcat(line, to_num((uint32_t)gfx_width()));
        strcat(line, " × ");
        strcat(line, to_num((uint32_t)gfx_height()));
        strcat(line, " . 32 bpp linéaire");
        gfx_text(x + 24, ly + ROW_STEP, line, t->text, GFX_COLOR_TRANSPARENT);
        strcpy(line, "Uptime : ");
        uint32_t seconds = timer_ticks() / 100u;
        strcat(line, to_num(seconds));
        strcat(line, " s");
        gfx_text(x + 24, ly + 2 * ROW_STEP, line, t->text, GFX_COLOR_TRANSPARENT);
        strcpy(line, "Tas noyau : 64 KiB statique, fusion des blocs libérés");
        gfx_text(x + 24, ly + 3 * ROW_STEP, line, t->text, GFX_COLOR_TRANSPARENT);
    }
    (void)mem_line_compose;
    (void)row_counts;
    (void)h;
}

static void apply_appearance(void) { /* rien à faire : le rendu relit theme() */ }
static void apply_appearance_full(void) { apply_appearance(); desktop_request_redraw(); }

void app_settings_key(char key) {
    uint8_t code = (uint8_t)key;
    switch (code) {
        case DESK_KEY_LEFT:
            if (st.tab == TAB_APPEARANCE) {
                if (st.row == 0) theme_prev_wallpaper(); else theme_prev_accent();
            }
            break;
        case DESK_KEY_RIGHT:
            if (st.tab == TAB_APPEARANCE) {
                if (st.row == 0) theme_next_wallpaper(); else theme_next_accent();
            }
            break;
        case DESK_KEY_DOWN:
            if (st.row + 1u < row_counts[st.tab]) ++st.row;
            break;
        case DESK_KEY_UP:
            if (st.row) --st.row;
            break;
        default:
            if (key >= '1' && key <= '3') { st.tab = (tab_kind_t)(key - '1'); st.row = 0; }
            break;
    }
    if (st.tab == TAB_APPEARANCE) apply_appearance_full();
    desktop_request_redraw();
}
