#include "../kernel/kernel.h"

/* Marqueurs spéciaux injectés dans le buffer (hors ASCII imprimable). */
#define KEY_DIR_UP    0x80
#define KEY_DIR_DOWN  0x81
#define KEY_DIR_LEFT  0x82
#define KEY_DIR_RIGHT 0x83
#define KEY_MENU      0x1F /* Ctrl+Échap : menu Démarrer */
#define KEY_ALTTAB    0x1E /* Alt+Tab : cycle des fenêtres */

static volatile char buffer[KEYBUF_SIZE];
static volatile uint16_t head, tail;
static bool shift_down, caps_lock, ctrl_down, alt_down, extended;

static const char normal[128] = {
    [1]=27, [2]='1',[3]='2',[4]='3',[5]='4',[6]='5',[7]='6',[8]='7',[9]='8',[10]='9',[11]='0',
    [12]='-',[13]='=',[14]=8,[15]=9,[16]='q',[17]='w',[18]='e',[19]='r',[20]='t',[21]='y',
    [22]='u',[23]='i',[24]='o',[25]='p',[26]='[',[27]=']',[28]='\n',[30]='a',[31]='s',[32]='d',
    [33]='f',[34]='g',[35]='h',[36]='j',[37]='k',[38]='l',[39]=';',[40]='\'',[41]='`',[43]='\\',
    [44]='z',[45]='x',[46]='c',[47]='v',[48]='b',[49]='n',[50]='m',[51]=',',[52]='.',[53]='/',[57]=' '
};
static const char shifted[128] = {
    [1]=27, [2]='!',[3]='@',[4]='#',[5]='$',[6]='%',[7]='^',[8]='&',[9]='*',[10]='(',[11]=')',
    [12]='_',[13]='+',[14]=8,[15]=9,[16]='Q',[17]='W',[18]='E',[19]='R',[20]='T',[21]='Y',
    [22]='U',[23]='I',[24]='O',[25]='P',[26]='{',[27]='}',[28]='\n',[30]='A',[31]='S',[32]='D',
    [33]='F',[34]='G',[35]='H',[36]='J',[37]='K',[38]='L',[39]=':',[40]='"',[41]='~',[43]='|',
    [44]='Z',[45]='X',[46]='C',[47]='V',[48]='B',[49]='N',[50]='M',[51]='<',[52]='>',[53]='?',[57]=' '
};

static void push_key(char c) {
    uint16_t next = (uint16_t)((head + 1u) % KEYBUF_SIZE);
    if (next != tail) { buffer[head] = c; head = next; }
}

void keyboard_init(void) { head = tail = 0; shift_down = caps_lock = ctrl_down = alt_down = extended = false; }

void keyboard_irq(void) {
    uint8_t code = inb(0x60);
    if (code == 0xE0) { extended = true; return; }
    if (extended) {
        extended = false;
        switch (code) {
            case 0x48: push_key((char)KEY_DIR_UP); return;
            case 0x50: push_key((char)KEY_DIR_DOWN); return;
            case 0x4B: push_key((char)KEY_DIR_LEFT); return;
            case 0x4D: push_key((char)KEY_DIR_RIGHT); return;
            default: return; /* touches étendues non mappées */
        }
    }
    if (code == 0x1D) { ctrl_down = true; return; }
    if (code == 0x9D) { ctrl_down = false; return; }
    if (code == 0x38) { alt_down = true; return; }
    if (code == 0xB8) { alt_down = false; return; }
    if (code == 0x2A || code == 0x36) { shift_down = true; return; }
    if (code == 0xAA || code == 0xB6) { shift_down = false; return; }
    if (code == 0x3A) { caps_lock = !caps_lock; return; }
    if (code & 0x80) return;
    if (code >= 128) return;
    if (code == 0x01 && ctrl_down) { push_key((char)KEY_MENU); return; }   /* Ctrl+Échap */
    if (code == 0x0F && alt_down) { push_key((char)KEY_ALTTAB); return; }  /* Alt+Tab */
    if (code == 0x01) { push_key(27); return; } /* Échap nu */
    char c = (shift_down ? shifted : normal)[code];
    if (caps_lock && c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    if (!c) return;
    push_key(c);
}

int keyboard_read(char *out) {
    if (!out || tail == head) return 0;
    *out = buffer[tail];
    tail = (uint16_t)((tail + 1u) % KEYBUF_SIZE);
    return 1;
}
