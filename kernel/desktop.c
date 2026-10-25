#include "desktop.h"
#include "gfx.h"
#include "theme.h"
#include "app_browser.h"
#include "app_settings.h"
#include "app_security.h"
#include "app_terminal.h"
#include "kernel.h"
#include "../drivers/mouse.h"
#include "string.h"

/* NexOS — bureau graphique : gestionnaire de fenêtres, barre, menu, curseur. */

#define TASKBAR_HEIGHT 32
#define MENU_WIDTH 236u
#define MENU_ROW 30u
#define MENU_APP_ITEMS 4u   /* applications */
#define MENU_ITEMS 6u       /* + Redémarrer, Arrêter */
#define MAX_WINDOWS 8
#define CURSOR_W 12
#define CURSOR_H 18
#define RESIZE_MARGIN 4
#define GRIP_SIZE 14
#define MIN_WINDOW_W 320
#define MIN_WINDOW_H 240
#define DBLCLICK_TICKS 40u /* ~0,4 s à 100 Hz */

typedef struct {
    bool used, focused, maximized;
    int32_t x, y, w, h;
    int32_t rx, ry, rw, rh; /* rectangle de restauration */
    app_id_t app;
} window_t;

static window_t windows[MAX_WINDOWS];
static int zorder[MAX_WINDOWS];      /* du bas vers le haut : indices de windows[] */
static int reachable;                /* fenêtres employées */
static bool menu_open, redraw_pending;
static int menu_selected;
static int32_t cursor_x, cursor_y;
static uint32_t mouse_buttons;
static bool dragging;
static int32_t drag_dx, drag_dy;
static int drag_window;
static unsigned drag_edges; /* 1 = droite, 2 = bas, 3 = coin */
static int32_t last_click_pos[2];
static uint32_t last_click_ticks;
static int last_click_window;
static uint32_t under_cursor[CURSOR_W * CURSOR_H];
static char clock_text[8] = "--:--";
static const app_t *apps[APP_COUNT];

static int reached_count(void);
void desktop_close_window(int index);

static const char *app_titles[APP_COUNT] = {
    "NexBrowser", "Paramètres", "Sécurité", "Terminal"
};

/* Curseur flèche 12x18 : 0x202020 bordure, 0xF7F7F7 corps, 0xAFB2BB ombre, alpha 0 masqué. */
static const uint32_t cursor_sprite[CURSOR_W * CURSOR_H] = {
    0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x202020,0x202020,0x202020,0x202020,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x202020,0x202020,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0x202020,0x00000000,0x00000000,
    0x00000000,0x00000000,0x202020,0x202020,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0x202020,0x00000000,
    0x00000000,0x00000000,0x202020,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0x202020,
    0x00000000,0x00000000,0x202020,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0x202020,
    0x00000000,0x00000000,0x202020,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0x202020,
    0x00000000,0x00000000,0x202020,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,
    0x00000000,0x00000000,0x202020,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xAFB2BB,0xAFB2BB,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,
    0x00000000,0x00000000,0x202020,0xF7F7F7,0xF7F7F7,0xAFB2BB,0xF7F7F7,0x202020,0x202020,0x202020,0x00000000,0x00000000,
    0x00000000,0x00000000,0x202020,0xF7F7F7,0xF7F7F7,0xAFB2BB,0x202020,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x202020,0xF7F7F7,0xAFB2BB,0x202020,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x202020,0xF7F7F7,0xAFB2BB,0x202020,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x202020,
    0x00000000,0x00000000,0x202020,0xF7F7F7,0xAFB2BB,0x202020,0x00000000,0x00000000,0x00000000,0x00000000,0x202020,0x00000000,
    0x00000000,0x00000000,0x202020,0xF7F7F7,0xAFB2BB,0x202020,0x00000000,0x00000000,0x00000000,0x202020,0x00000000,0x00000000,
    0x00000000,0x00000000,0x202020,0xF7F7F7,0xAFB2BB,0x202020,0x00000000,0x00000000,0x202020,0xF7F7F7,0x202020,0x00000000,
    0x00000000,0x00000000,0x202020,0xF7F7F7,0xAFB2BB,0x202020,0x00000000,0x202020,0xF7F7F7,0xF7F7F7,0xF7F7F7,0x202020,
    0x00000000,0x00000000,0x202020,0xF7F7F7,0xAFB2BB,0x202020,0x202020,0xF7F7F7,0xF7F7F7,0xF7F7F7,0xF7F7F7,0x202020,
    0x00000000,0x00000000,0x00000000,0x00000000,0x202020,0x00000000,0xF7F7F7,0xF7F7F7,0xF7F7F7,0x202020,0x00000000,0x00000000
};

static int str_len(const char *s) { int n = 0; if (s) while (s[n]) ++n; return n; }
static bool point_in(int32_t px, int32_t py, int32_t x, int32_t y, int32_t w, int32_t h) {
    return px >= x && px < x + w && py >= y && py < y + h;
}
static void save_under_cursor(void);
static int next_under(void);

static int top_window(void) {
    if (reachable <= 0) return -1;
    return zorder[reachable - 1];
}
static int focused_index(void) {
    for (int i = 0; i < MAX_WINDOWS; ++i)
        if (windows[i].used && windows[i].focused) return i;
    return -1;
}
static void focus_only(int index) {
    for (int i = 0; i < MAX_WINDOWS; ++i) windows[i].focused = (i == index);
}
static void raise_window(int index) {
    if (index < 0 || index >= MAX_WINDOWS || !windows[index].used) return;
    int pos = 0;
    while (pos < reachable && zorder[pos] != index) ++pos;
    for (int i = pos; i < reachable - 1; ++i) zorder[i] = zorder[i + 1]; /* retire */
    if (pos < reachable) reachable--;
    zorder[reachable++] = index;
    focus_only(index);
    redraw_pending = true;
}

static void clock_write(void) {
    uint32_t minutes = (timer_ticks() / 100u / 60u) % (24u * 60u);
    uint32_t hh = minutes / 60u, mm = minutes % 60u;
    clock_text[0] = (char)('0' + (hh / 10u) % 10u);
    clock_text[1] = (char)('0' + hh % 10u);
    clock_text[2] = ':';
    clock_text[3] = (char)('0' + (mm / 10u) % 10u);
    clock_text[4] = (char)('0' + mm % 10u);
    clock_text[5] = 0;
}

/* --- Rendu ----------------------------------------------------------------- */
static void draw_wallpaper(void) {
    const theme_palette_t *t = theme();
    if (theme_wallpaper_style() == 0)
        gfx_gradient_vertical(0, 0, (int32_t)gfx_width(), (int32_t)gfx_height(), t->wallpaper_top, t->wallpaper_bottom);
    else
        gfx_fill(0, 0, (int32_t)gfx_width(), (int32_t)gfx_height(), t->wallpaper_top);
}

static void draw_window(const window_t *w) {
    const theme_palette_t *t = theme();
    /* cadre + barre de titre */
    gfx_fill(w->x - 1, w->y - 1, w->w + 2, w->h + 2, w->focused ? t->accent : t->window_border);
    gfx_fill(w->x, w->y, w->w, 24, t->titlebar_bg);
    gfx_text(w->x + 10, w->y + 4, app_titles[w->app], t->titlebar_text, GFX_COLOR_TRANSPARENT);
    gfx_fill(w->x + w->w - 26, w->y + 4, 22, 17, t->close_bg);
    gfx_text(w->x + w->w - 19, w->y + 5, "X", t->close_text, GFX_COLOR_TRANSPARENT);
    /* contenu */
    int32_t cx = w->x, cy = w->y + 24, cw = w->w, ch = w->h - 24;
    gfx_fill(cx, cy, cw, ch, t->window_bg);
    gfx_set_clip(cx, cy, cw, ch);
    if (apps[w->app] && apps[w->app]->draw) apps[w->app]->draw(cx, cy, cw, ch);
    gfx_reset_clip();
    /* poignée de redimensionnement : hachures diagonales bas-droite */
    uint32_t grip_color = w->focused ? t->accent : t->menu_border;
    for (int32_t i = 4; i < (int32_t)GRIP_SIZE; i += 4) {
        for (int32_t d = 0; d < (int32_t)GRIP_SIZE - i; ++d) {
            gfx_put(w->x + w->w - (int32_t)GRIP_SIZE + i + d,
                    w->y + w->h - 1 - i + d, grip_color);
            gfx_put(w->x + w->w - 1 - d,
                    w->y + w->h - (int32_t)GRIP_SIZE + i + d - 1, grip_color);
        }
    }
}

static void draw_taskbar(void) {
    const theme_palette_t *t = theme();
    int32_t bar_y = (int32_t)gfx_height() - TASKBAR_HEIGHT;
    int32_t bar_w = (int32_t)gfx_width();
    gfx_fill(0, bar_y, bar_w, TASKBAR_HEIGHT, t->taskbar_bg);
    gfx_fill(0, bar_y, bar_w, 1, t->menu_border);
    int32_t bx = 8;
    const char *label = "Démarrer";
    int32_t bw = 8 * str_len(label) + 20;
    gfx_fill(bx, bar_y + 4, bw, TASKBAR_HEIGHT - 8, menu_open ? t->accent : t->menu_hover_bg);
    gfx_text(bx + 10, bar_y + 8, label, t->taskbar_text, GFX_COLOR_TRANSPARENT);
    int32_t ox = bx + bw + 10;
    for (int i = 0; i < MAX_WINDOWS; ++i) {
        if (!windows[i].used) continue;
        const char *ttl = app_titles[windows[i].app];
        int32_t ow = 8 * str_len(ttl) + 18;
        if (ox + ow > bar_w - 90) break;
        gfx_fill(ox, bar_y + 4, ow, TASKBAR_HEIGHT - 8, windows[i].focused ? t->accent_soft : t->menu_hover_bg);
        gfx_text(ox + 9, bar_y + 8, ttl, t->taskbar_text, GFX_COLOR_TRANSPARENT);
        ox += ow + 8;
    }
    clock_write();
    gfx_text(bar_w - 8 * 5 - 14, bar_y + 8, clock_text, t->taskbar_text, GFX_COLOR_TRANSPARENT);
}

static int32_t menu_y(void) {
    return (int32_t)gfx_height() - TASKBAR_HEIGHT - (int32_t)(MENU_ITEMS * MENU_ROW) - 10;
}
static void draw_menu(void) {
    if (!menu_open) return;
    const theme_palette_t *t = theme();
    int32_t mx = 8, my = menu_y();
    int32_t mh = (int32_t)(MENU_ITEMS * MENU_ROW) + 8;
    gfx_fill(mx, my, (int32_t)MENU_WIDTH, mh, t->menu_bg);
    for (uint32_t r = 0; r < MENU_ITEMS; ++r) {
        int32_t ry = my + 5 + (int32_t)r * (int32_t)MENU_ROW;
        uint32_t bg = ((int32_t)r == menu_selected) ? t->menu_hover_bg : t->menu_bg;
        gfx_fill(mx + 3, ry, (int32_t)MENU_WIDTH - 6, (int32_t)MENU_ROW - 3, bg);
        const char *label = (r < MENU_APP_ITEMS) ? app_titles[r] : ((r == MENU_APP_ITEMS) ? "Redémarrer" : "Arrêter");
        gfx_text(mx + 12, ry + 6, label, t->menu_text, GFX_COLOR_TRANSPARENT);
    }
    gfx_fill(mx, my, (int32_t)MENU_WIDTH, 1, t->menu_border);
    gfx_fill(mx, my + mh - 1, (int32_t)MENU_WIDTH, 1, t->menu_border);
    gfx_fill(mx + (int32_t)MENU_WIDTH - 1, my, 1, mh, t->menu_border);
}

static void render(void) {
    draw_wallpaper();
    for (int i = 0; i < reached_count(); ++i)
        draw_window(&windows[zorder[i]]);
    draw_taskbar();
    draw_menu();
    save_under_cursor();
    gfx_sprite(cursor_sprite, cursor_x, cursor_y, CURSOR_W, CURSOR_H);
}

static void save_under_cursor(void) {
    for (int r = 0; r < CURSOR_H; ++r)
        for (int c = 0; c < CURSOR_W; ++c)
            under_cursor[r * CURSOR_W + c] = gfx_read_pixel(cursor_x + c, cursor_y + r);
}

static void restore_under_cursor(void) {
    for (int r = 0; r < CURSOR_H; ++r)
        for (int c = 0; c < CURSOR_W; ++c)
            if (under_cursor[r * CURSOR_W + c] != GFX_COLOR_TRANSPARENT)
                gfx_put(cursor_x + c, cursor_y + r, under_cursor[r * CURSOR_W + c]);
}

static void move_cursor(int32_t nx, int32_t ny) {
    restore_under_cursor();
    cursor_x = nx;
    cursor_y = ny;
    if (cursor_x < 0) cursor_x = 0;
    if (cursor_y < 0) cursor_y = 0;
    if ((int32_t)cursor_x > (int32_t)gfx_width() - (int32_t)CURSOR_W)
        cursor_x = (int32_t)gfx_width() - (int32_t)CURSOR_W;
    if ((int32_t)cursor_y > (int32_t)gfx_height() - (int32_t)CURSOR_H)
        cursor_y = (int32_t)gfx_height() - (int32_t)CURSOR_H;
    save_under_cursor();
    gfx_sprite(cursor_sprite, cursor_x, cursor_y, CURSOR_W, CURSOR_H);
}

/* --- Interactions ---------------------------------------------------------- */
static int hit_window(int32_t px, int32_t py) {
    for (int i = reachable - 1; i >= 0; --i) {
        int w = zorder[i];
        if (point_in(px, py, windows[w].x, windows[w].y, windows[w].w, windows[w].h)) return w;
    }
    return -1;
}

/* 0 = rien, 1 = bord droit, 2 = bord bas, 3 = coin bas-droit. */
static unsigned resize_hit(const window_t *w, int32_t px, int32_t py) {
    unsigned res = 0;
    if (px >= w->x + w->w - (int32_t)RESIZE_MARGIN) res |= 1u;
    if (py >= w->y + w->h - (int32_t)RESIZE_MARGIN) res |= 2u;
    return res;
}

static void toggle_maximize(int index) {
    window_t *w = &windows[index];
    int32_t screen_w = (int32_t)gfx_width();
    int32_t screen_h = (int32_t)gfx_height() - TASKBAR_HEIGHT;
    if (w->maximized) {
        w->maximized = false;
        w->x = w->rx; w->y = w->ry; w->w = w->rw; w->h = w->rh;
    } else {
        w->maximized = true;
        w->rx = w->x; w->ry = w->y; w->rw = w->w; w->rh = w->h;
        w->x = 0; w->y = 0; w->w = screen_w; w->h = screen_h;
    }
    raise_window(index);
    redraw_pending = true;
}

static void start_drag(int index, int32_t px, int32_t py, unsigned edges) {
    window_t *w = &windows[index];
    dragging = true;
    drag_window = index;
    drag_edges = edges;
    if (edges == 0) {
        if (w->maximized) { /* déplacer une maximisée : restaurer d'abord */
            int32_t rw = w->rw;
            int32_t rh = w->rh;
            w->maximized = false;
            w->w = rw; w->h = rh;
            w->x = px - w->w / 2;
            w->y = py - 12;
            if (w->y < 0) w->y = 0;
            if (w->x > (int32_t)gfx_width() - MIN_WINDOW_W) w->x = (int32_t)gfx_width() - MIN_WINDOW_W;
        }
        drag_dx = px - w->x;
        drag_dy = py - w->y;
    }
}

static void toggle_menu(void) { menu_open = !menu_open; if (menu_open) menu_selected = 0; redraw_pending = true; }

static void menu_launch(int row) {
    menu_open = false;
    redraw_pending = true;
    if (row >= 0 && row < (int)MENU_APP_ITEMS) { desktop_open((app_id_t)row); return; }
    if (row == (int)MENU_APP_ITEMS) desktop_execute(0);      /* Redémarrer */
    else if (row == (int)MENU_APP_ITEMS + 1) desktop_execute(1); /* Arrêter */
}

static void handle_click(int32_t px, int32_t py) {
    if (menu_open) {
        int32_t my = menu_y();
        int32_t mh = (int32_t)(MENU_ITEMS * MENU_ROW) + 8;
        if (point_in(px, py, 8, my, (int32_t)MENU_WIDTH, mh)) {
            int row = (int)((py - my - 5) / (int32_t)MENU_ROW);
            if (row >= 0 && row < (int)MENU_ITEMS) { menu_launch(row); return; }
        }
        toggle_menu();
        return;
    }
    int32_t bar_y = (int32_t)gfx_height() - TASKBAR_HEIGHT;
    if (py >= bar_y) {
        const char *label = "Démarrer";
        int32_t bx = 8;
        int32_t bw = 8 * str_len(label) + 20;
        if (point_in(px, py, bx, bar_y, bw, TASKBAR_HEIGHT)) { toggle_menu(); return; }
        int32_t ox = bx + bw + 10;
        for (int i = 0; i < MAX_WINDOWS; ++i) {
            if (!windows[i].used) continue;
            const char *ttl = app_titles[windows[i].app];
            int32_t ow = 8 * str_len(ttl) + 18;
            if (ox + ow > (int32_t)gfx_width() - 90) break;
            if (point_in(px, py, ox, bar_y, ow, TASKBAR_HEIGHT)) { raise_window(i); return; }
            ox += ow + 8;
        }
        return;
    }
    /* prise des bords/coin de la fenêtre du dessous : toujours possible */
    {
        unsigned edges = 0;
        for (int i = reachable - 1; i >= 0; --i) {
            edges = resize_hit(&windows[zorder[i]], px, py);
            if (edges) {
                raise_window(zorder[i]);
                start_drag(zorder[i], px, py, edges);
                return;
            }
        }
        (void)edges;
    }
    int index = hit_window(px, py);
    if (index >= 0) {
        window_t *w = &windows[index];
        if (point_in(px, py, w->x + w->w - 26, w->y + 4, 22, 17)) { desktop_close_window(index); return; }
        bool on_title = py < w->y + 24;
        if (on_title) {
            uint32_t now = timer_ticks();
            if (last_click_window == index && now - last_click_ticks <= DBLCLICK_TICKS &&
                last_click_pos[0] == px && last_click_pos[1] == py) {
                last_click_window = -1;
                toggle_maximize(index);
                return;
            }
            last_click_ticks = now;
            last_click_window = index;
            last_click_pos[0] = px; last_click_pos[1] = py;
        }
        bool already = w->focused;
        raise_window(index);
        if (!already && on_title) return;
        if (on_title) { /* déplacement par la barre de titre */
            start_drag(index, px, py, 0);
        }
    }
}

void desktop_close_window(int index) {
    if (index < 0 || index >= MAX_WINDOWS || !windows[index].used) return;
    windows[index].used = false;
    int pos = 0;
    while (pos < reachable && zorder[pos] != index) ++pos;
    for (int i = pos; i < reachable - 1; ++i) zorder[i] = zorder[i + 1];
    if (pos < reachable) reachable--;
    int top = top_window();
    focus_only(top);
    redraw_pending = true;
}

/* --- Boucles d'entrées ------------------------------------------------------ */
static void pump_mouse(void) {
    mouse_event_t ev;
    while (mouse_poll(&ev)) {
        move_cursor((int32_t)ev.dx, (int32_t)ev.dy);
        uint32_t now = ev.buttons;
        if (dragging && (mouse_buttons & 1u) && drag_window >= 0) {
            window_t *w = &windows[drag_window];
            if (drag_edges) {
                /* redimensionnement continu, borné par l'écran et le minimum */
                int32_t max_w = (int32_t)gfx_width() - 2 - w->x;
                int32_t max_h = (int32_t)gfx_height() - TASKBAR_HEIGHT - 2 - w->y;
                if (drag_edges & 1u) {
                    int32_t nw = (int32_t)ev.dx - w->x;
                    if (nw < MIN_WINDOW_W) nw = MIN_WINDOW_W;
                    if (nw > max_w) nw = max_w;
                    if (nw != w->w) { w->w = nw; w->maximized = false; redraw_pending = true; }
                }
                if (drag_edges & 2u) {
                    int32_t nh = (int32_t)ev.dy - w->y;
                    if (nh < MIN_WINDOW_H) nh = MIN_WINDOW_H;
                    if (nh > max_h) nh = max_h;
                    if (nh != w->h) { w->h = nh; w->maximized = false; redraw_pending = true; }
                }
            } else {
                w->x = (int32_t)ev.dx - drag_dx;
                w->y = (int32_t)ev.dy - drag_dy;
                if (w->x < 0) w->x = 0;
                if (w->y < 0) w->y = 0;
                if (w->x > (int32_t)gfx_width() - MIN_WINDOW_W) w->x = (int32_t)gfx_width() - MIN_WINDOW_W;
                if (w->y > (int32_t)gfx_height() - 24) w->y = (int32_t)gfx_height() - 24;
                redraw_pending = true;
            }
        }
        bool pressed = (now & 1u) && !(mouse_buttons & 1u);
        mouse_buttons = now;
        if (pressed) handle_click(cursor_x, cursor_y);
        if (!(now & 1u)) { dragging = false; drag_window = -1; drag_edges = 0; }
    }
}

static void pump_keyboard(void) {
    char k;
    while (keyboard_read(&k)) {
        uint8_t code = (uint8_t)k;
        if (code == DESK_KEY_MENU) { toggle_menu(); continue; }
        if (code == DESK_KEY_ALTTAB) { int other = next_under(); if (other >= 0) raise_window(other); continue; }
        if (menu_open) {
            if (k == 27) { toggle_menu(); continue; }
            if (code == DESK_KEY_DOWN) { menu_selected = (int)((menu_selected + 1u) % MENU_ITEMS); redraw_pending = true; continue; }
            if (code == DESK_KEY_UP) { menu_selected = (int)(((uint32_t)menu_selected + MENU_ITEMS - 1u) % MENU_ITEMS); redraw_pending = true; continue; }
            if (k == '\n') { menu_launch(menu_selected); continue; }
            continue;
        }
        int index = focused_index();
        if (index >= 0 && apps[windows[index].app] && apps[windows[index].app]->key) {
            apps[windows[index].app]->key(k);
            desktop_request_redraw();
        }
    }
}

static int next_under(void) {
    for (int i = reachable - 2; i >= 0; --i) return zorder[i];
    return reachable > 0 ? zorder[reachable - 1] : -1;
}

static void pump_clock(void) {
    char previous = clock_text[4];
    clock_write();
    if (clock_text[4] != previous) redraw_pending = true;
}

/* --- Ouvertures ------------------------------------------------------------- */
void desktop_open(app_id_t id) {
    for (int i = 0; i < MAX_WINDOWS; ++i)
        if (windows[i].used && windows[i].app == id) { raise_window(i); menu_open = false; return; }
    for (int i = 0; i < MAX_WINDOWS; ++i) {
        if (!windows[i].used) {
            windows[i].used = true;
            windows[i].app = id;
            windows[i].w = 720;
            windows[i].h = 520;
            if (windows[i].w > (int32_t)gfx_width() - 96)
                windows[i].w = (int32_t)gfx_width() - 96;
            if (windows[i].h > (int32_t)gfx_height() - TASKBAR_HEIGHT - 67)
                windows[i].h = (int32_t)gfx_height() - TASKBAR_HEIGHT - 67;
            if (windows[i].w < MIN_WINDOW_W) windows[i].w = MIN_WINDOW_W;
            if (windows[i].h < MIN_WINDOW_H) windows[i].h = MIN_WINDOW_H;
            windows[i].maximized = false;
            windows[i].x = 48 + ((reachable * 24) % 120);
            windows[i].y = 40 + ((reachable * 18) % 90);
            windows[i].rx = windows[i].x;
            windows[i].ry = windows[i].y;
            windows[i].rw = windows[i].w;
            windows[i].rh = windows[i].h;
            zorder[reachable++] = i;
            raise_window(i);
            menu_open = false;
            redraw_pending = true;
            return;
        }
    }
}

bool desktop_is_open(app_id_t id) {
    for (int i = 0; i < MAX_WINDOWS; ++i)
        if (windows[i].used && windows[i].app == id) return true;
    return false;
}

void desktop_request_redraw(void) { redraw_pending = true; }

void desktop_execute(uint32_t kind) {
    if (kind == 0) { /* redémarrage : ligne 0xFE sur le contrôleur */
        cpu_disable_interrupts();
        while (inb(0x64) & 0x02) { }
        outb(0x64, 0xFE);
        for (;;) cpu_halt();
    }
    cpu_disable_interrupts();
    for (;;) cpu_halt(); /* arrêt */
}

/* --- Enregistrement des applications ---------------------------------------- */
static const app_t app_registry[APP_COUNT] = {
    { APP_BROWSER,  app_browser_draw,  app_browser_key  },
    { APP_SETTINGS, app_settings_draw, app_settings_key },
    { APP_SECURITY, app_security_draw, app_security_key },
    { APP_TERMINAL, app_terminal_draw, app_terminal_key },
};
void desktop_apps_register(void) {
    for (uint32_t i = 0; i < APP_COUNT; ++i) apps[i] = &app_registry[i];
}

void desktop_run(void) {
    cursor_x = (int32_t)gfx_width() / 2;
    cursor_y = (int32_t)gfx_height() / 2;
    reachable = 0;
    menu_open = false;
    redraw_pending = false;
    desktop_apps_register();
    render();
    klog("INFO", "bureau graphique pret");
    for (;;) {
        pump_mouse();
        pump_keyboard();
        pump_clock();
        if (redraw_pending) { render(); redraw_pending = false; }
        __asm__ volatile("hlt");
    }
}

/* Reached count avoids signed mismatch warnings in rendering loops. */
static int reached_count(void) { return reachable; }
