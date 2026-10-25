/* NexOS - NexBrowser : navigateur natif minimal, hors ligne par construction.
 * Rend uniquement les pages embarquées du noyau ; toute URL non reconnue
 * affiche explicitement l'absence de pile réseau. Aucun moteur externe, aucune
 * compatibilité revendiquée avec un navigateur existant. */
#include "app_browser.h"
#include "gfx.h"
#include "theme.h"
#include "kernel.h"
#include "string.h"

typedef enum { PAGE_HOME = 0, PAGE_ABOUT, PAGE_SECURITY, PAGE_OFFLINE } page_kind_t;

#define URL_MAX 72
#define LINE_STEP 18

typedef struct {
    page_kind_t page;
    char url[URL_MAX];
    bool editing;
    char offline_host[URL_MAX];
} browser_state_t;

static browser_state_t st;
static bool initialized;

static void copy_out(char *dst, const char *src) {
    while ((*dst++ = *src++) != 0) { }
}

static void app_browser_init(void) {
    st.page = PAGE_HOME;
    st.editing = false;
    copy_out(st.url, "nex://home");
    st.offline_host[0] = 0;
    initialized = true;
}

static const char *const home_lines[] = {
    "Bienvenue dans NexBrowser",
    "",
    "Le navigateur natif de NexOS est ouvert par défaut depuis le",
    "menu Démarrer. Les pages ci-dessous sont embarquées dans le",
    "noyau et générées localement :",
    "",
    "   - Accueil : cette page",
    "   - À propos : mécanismes et limites réelles",
    "   - Sécurité : état courant du noyau",
    "",
    "Il n'y a pas encore de pile réseau ni de TLS dans NexOS : les",
    "adresses restent hors ligne, ce que l'interface affiche clairement.",
};

static const char *const about_lines[] = {
    "NexBrowser 1.0 - rendu natif du noyau, sans moteur externe.",
    "",
    "   . Pages embarquées : Accueil, À propos, Sécurité",
    "   . Réseau : indisponible tant que le pilote réseau manque",
    "   . Aucun chiffrement applicatif à ce stade",
    "",
    "Les contenus non embarqués ne sont pas simulés : l'application",
    "affiche un état hors ligne assumé plutôt qu'une navigation factice.",
};

static const char *const security_lines[] = {
    "Sécurité du noyau - état actuel",
    "",
    "   . Mode protégé 32 bits : actif",
    "   . Pagination / W^X : pas encore (étape 2 du plan)",
    "   . Séparation ring 3 : pas encore (étape 2 du plan)",
    "   . Journal COM1 : opérationnel",
    "",
    "Le détail complet, mesuré au démarrage, est ouvert depuis",
    "l'application Sécurité du menu Démarrer.",
};

static const char *const *page_lines(page_kind_t page, int *count) {
    switch (page) {
        case PAGE_HOME: *count = (int)(sizeof(home_lines) / sizeof(home_lines[0])); return home_lines;
        case PAGE_ABOUT: *count = (int)(sizeof(about_lines) / sizeof(about_lines[0])); return about_lines;
        case PAGE_SECURITY: *count = (int)(sizeof(security_lines) / sizeof(security_lines[0])); return security_lines;
        default: *count = 0; return 0;
    }
}

void app_browser_draw(int32_t x, int32_t y, int32_t w, int32_t h) {
    if (!initialized) app_browser_init();
    const theme_palette_t *t = theme();
    /* barre d'URL */
    int32_t uy = y + 8, uh = 26;
    gfx_fill(x + 12, uy, w - 24, uh, st.editing ? t->input_bg : t->input_bg);
    gfx_fill(x + 12, uy, w - 24, 1, t->input_border);
    gfx_fill(x + 12, uy + uh - 1, w - 24, 1, t->input_border);
    gfx_fill(x + 12, uy, 1, uh, t->input_border);
    gfx_fill(x + 12 + w - 24 - 1, uy, 1, uh, t->input_border);
    gfx_text(x + 20, uy + 5, st.url, t->text, GFX_COLOR_TRANSPARENT);
    if (st.editing) {
        int32_t tx = x + 20 + 8 * (int32_t)strlen(st.url);
        gfx_fill(tx + 2, uy + 4, 2, uh - 8, t->accent);
    }
    /* barre d'état (position calculée tôt : borne le contenu) */
    const char *status = "HORS LIGNE . réseau non implémenté";
    int32_t sy = y + h - 20;
    /* contenu : borné pour ne jamais passer sous la barre d'état */
    int32_t cx = x + 20;
    int32_t cy = uy + uh + 18;
    int32_t usable = sy - 14 - cy;
    int max_lines = usable >= LINE_STEP ? (int)(usable / LINE_STEP) : 1;
    if (st.page == PAGE_OFFLINE) {
        char line[URL_MAX + 48];
        copy_out(line, "Hors ligne : « ");
        strcat(line, st.offline_host);
        strcat(line, " » n'a pas de correspondance embarquée.");
        if (max_lines > 0) gfx_text(cx, cy, line, t->bad, GFX_COLOR_TRANSPARENT);
        if (max_lines > 1) gfx_text(cx, cy + LINE_STEP, "NexOS n'a pas encore de pile réseau ni de TLS.", t->text_muted, GFX_COLOR_TRANSPARENT);
        if (max_lines > 2) {
            copy_out(line, "Pages disponibles : ");
            strcat(line, "home . about . security");
            gfx_text(cx, cy + 2 * LINE_STEP, line, t->text_muted, GFX_COLOR_TRANSPARENT);
        }
    } else {
        int count = 0;
        const char *const *lines = page_lines(st.page, &count);
        for (int i = 0; i < count && i < max_lines; ++i)
            gfx_text(cx, cy + i * LINE_STEP, lines[i], i == 0 ? t->accent : t->text, GFX_COLOR_TRANSPARENT);
    }
    gfx_fill(x + 12, sy - 8, w - 24, 1, t->input_border);
    gfx_text(x + 16, sy + 2, status, t->text_muted, GFX_COLOR_TRANSPARENT);
}

static void navigate(const char *url) {
    char probe[URL_MAX];
    copy_out(probe, url);
    for (char *p = probe; *p; ++p) {
        if (*p >= 'A' && *p <= 'Z') *p = (char)(*p - 'A' + 'a');
    }
    st.editing = false;
    if (strstr(probe, "home")) { st.page = PAGE_HOME; copy_out(st.url, "nex://home"); return; }
    if (strstr(probe, "about")) { st.page = PAGE_ABOUT; copy_out(st.url, "nex://about"); return; }
    if (strstr(probe, "security") || strstr(probe, "securite")) { st.page = PAGE_SECURITY; copy_out(st.url, "nex://security"); return; }
    st.page = PAGE_OFFLINE;
    copy_out(st.offline_host, url);
}

void app_browser_key(char key) {
    if (!st.editing) {
        if (key >= 32 && key < 127) {
            st.editing = true;
            st.url[0] = key;
            st.url[1] = 0;
        }
        return;
    }
    switch (key) {
        case '\n': navigate(st.url); return;
        case '\b': {
            size_t n = strlen(st.url);
            if (n) st.url[n - 1] = 0;
            return;
        }
        case 27: st.editing = false; return;
        default:        if (key >= 32 && key < 127) {
            size_t n = strlen(st.url);
            if (n + 1 < URL_MAX) { st.url[n] = key; st.url[n + 1] = 0; }
        }
        return;
    }
}
