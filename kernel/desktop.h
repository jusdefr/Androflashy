/* NexOS — bureau graphique : fenêtres, barre des tâches, menu Démarrer. */
#ifndef NEXOS_DESKTOP_H
#define NEXOS_DESKTOP_H
#include <stdint.h>
#include <stdbool.h>

/* Marqueurs partagés avec drivers/keyboard.c (hors ASCII imprimable). */
#define DESK_KEY_MENU   0x1F /* Ctrl+Échap */
#define DESK_KEY_ALTTAB 0x1E /* Alt+Tab    */
#define DESK_KEY_UP     0x80
#define DESK_KEY_DOWN   0x81
#define DESK_KEY_LEFT   0x82
#define DESK_KEY_RIGHT  0x83

typedef enum {
    APP_BROWSER = 0,
    APP_SETTINGS,
    APP_SECURITY,
    APP_TERMINAL,
    APP_COUNT
} app_id_t;

typedef struct {
    app_id_t id;
    void (*draw)(int32_t x, int32_t y, int32_t w, int32_t h);
    void (*key)(char key);
} app_t;

void desktop_run(void);
void desktop_request_redraw(void);
void desktop_open(app_id_t id);
void desktop_close_window(int index);
bool desktop_is_open(app_id_t id);
void desktop_execute(uint32_t kind); /* 0 redémarrage, 1 arrêt */

#endif /* NEXOS_DESKTOP_H */
