#ifndef NEXOS_MOUSE_H
#define NEXOS_MOUSE_H
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t x, y;      /* position écran */
    uint32_t buttons;   /* bit0 gauche, bit1 droite, bit2 milieu */
} mouse_state_t;

typedef struct {
    int32_t dx, dy;     /* position absolue cible décrite par le paquet */
    uint32_t buttons;
} mouse_event_t;

void mouse_init(void);
void mouse_irq(void);
bool mouse_poll(mouse_event_t *out);
void mouse_state(mouse_state_t *out);
void mouse_set_state(int16_t x, int16_t y, uint32_t buttons);

#endif /* NEXOS_MOUSE_H */
