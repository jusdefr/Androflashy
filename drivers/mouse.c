#include "../kernel/kernel.h"
#include "mouse.h"
#include "../kernel/bootinfo.h"

/* Souris PS/2 en mode relatif par défaut (paquets à 3 octets).
 * Connecteur des données : bit0 de 0x64 lit, bit1 de 0x64 écrit ; les commandes
 * vers la souris partent avec 0xD4 sur 0x64 puis la commande sur 0x60.
 * L'IRQ12 décode dx/dy codés en 9 bits deux-complément et pousse un événement. */

#define MOUSE_EVENTS 128

static volatile mouse_event_t events[MOUSE_EVENTS];
static volatile uint16_t ev_head, ev_tail;

static volatile mouse_state_t state = { 640, 400, 0 };
static volatile uint32_t buttons;

static bool wait_write(void) {
    for (uint32_t i = 0; i < 100000u; ++i) if (!(inb(0x64) & 0x02)) return true;
    return false;
}
static bool wait_read(void) {
    for (uint32_t i = 0; i < 100000u; ++i) if (inb(0x64) & 0x01) return true;
    return false;
}
static bool send_cmd(uint8_t command) {
    if (!wait_write()) return false;
    outb(0x64, 0xD4);
    if (!wait_write()) return false;
    outb(0x60, command);
    if (!wait_read()) return false;
    return inb(0x60) == 0xFA;
}

/*
static void set_resolution(uint8_t value) {
    if (send_cmd(0xE8)) {
        if (!wait_write()) return;
        outb(0x64, 0xD4);
        if (!wait_write()) return;
        outb(0x60, value);
    }
}
*/

void mouse_init(void) {
    ev_head = ev_tail = 0;
    buttons = 0;

    /* Active le périphérique auxiliaire puis l'IRQ12 dans la config clavier/souris. */
    if (!wait_write()) return;
    outb(0x64, 0xA8);
    if (!wait_write()) return;
    outb(0x64, 0x20);
    if (!wait_read()) return;
    uint8_t config = inb(0x60);
    config |= 0x02; /* autorise l'IRQ12 */
    config |= 0x01; /* autorise l'IRQ1 */
    if (!wait_write()) return;
    outb(0x64, 0x60);
    if (!wait_write()) return;
    outb(0x60, config);

    /* Souris : valeurs par défaut puis flux activé. */
    if (!send_cmd(0xF6)) return; /* set defaults */
    if (!send_cmd(0xF4)) return; /* enable data reporting */
    state.x = 512;
    state.y = 384;
    klog("INFO", "souris PS/2 activee (mode relatif, IRQ12)");
}

void mouse_irq(void) {
    static uint8_t pk[3];
    static uint8_t cyc;
    uint8_t b = inb(0x60);
    if (cyc == 0 && !(b & 0x08)) return; /* resynchronise sur le bit anonyme */
    pk[cyc++] = b;
    if (cyc < 3) return;
    cyc = 0;
    if (pk[0] & 0xC0) return; /* débordement : abandon du paquet */
    int dx = (int)pk[1];
    if (pk[0] & 0x10) dx -= 256;
    int dy = (int)pk[2];
    if (pk[0] & 0x20) dy -= 256;
    buttons = pk[0] & 0x07;
    int nx = (int)state.x + dx;
    int ny = (int)state.y - dy; /* l'axe Y pointe vers le bas à l'écran */
    uint16_t next = (uint16_t)((ev_head + 1u) % MOUSE_EVENTS);
    if (next == ev_tail) return; /* file pleine : on écrase le plus ancien */
    events[ev_head].dx = nx;
    events[ev_head].dy = ny;
    events[ev_head].buttons = (uint8_t)buttons;
    ev_head = next;
}

bool mouse_poll(mouse_event_t *out) {
    if (!out || ev_tail == ev_head) return false;
    out->dx = events[ev_tail].dx;
    out->dy = events[ev_tail].dy;
    out->buttons = events[ev_tail].buttons;
    ev_tail = (uint16_t)((ev_tail + 1u) % MOUSE_EVENTS);
    return true;
}

void mouse_state(mouse_state_t *out) {
    out->x = state.x;
    out->y = state.y;
    out->buttons = state.buttons;
}

/* Les valeurs réelles sont mises à jour par le bureau, pas par le driver. */
void mouse_set_state(int16_t x, int16_t y, uint32_t new_buttons) {
    state.x = x;
    state.y = y;
    state.buttons = new_buttons;
}
