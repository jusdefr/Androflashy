#include "../kernel/kernel.h"

static volatile char buffer[KEYBUF_SIZE];
static volatile uint16_t head, tail;
static bool shift_down, caps_lock, extended;

static const char normal[128] = {
    [1]=27, [2]='1',[3]='2',[4]='3',[5]='4',[6]='5',[7]='6',[8]='7',[9]='8',[10]='9',[11]='0',
    [12]='-',[13]='=',[14]='\b',[15]='\t',[16]='q',[17]='w',[18]='e',[19]='r',[20]='t',[21]='y',
    [22]='u',[23]='i',[24]='o',[25]='p',[26]='[',[27]=']',[28]='\n',[30]='a',[31]='s',[32]='d',
    [33]='f',[34]='g',[35]='h',[36]='j',[37]='k',[38]='l',[39]=';',[40]='\'',[41]='`',[43]='\\',
    [44]='z',[45]='x',[46]='c',[47]='v',[48]='b',[49]='n',[50]='m',[51]=',',[52]='.',[53]='/',[57]=' '
};
static const char shifted[128] = {
    [1]=27, [2]='!',[3]='@',[4]='#',[5]='$',[6]='%',[7]='^',[8]='&',[9]='*',[10]='(',[11]=')',
    [12]='_',[13]='+',[14]='\b',[15]='\t',[16]='Q',[17]='W',[18]='E',[19]='R',[20]='T',[21]='Y',
    [22]='U',[23]='I',[24]='O',[25]='P',[26]='{',[27]='}',[28]='\n',[30]='A',[31]='S',[32]='D',
    [33]='F',[34]='G',[35]='H',[36]='J',[37]='K',[38]='L',[39]=':',[40]='"',[41]='~',[43]='|',
    [44]='Z',[45]='X',[46]='C',[47]='V',[48]='B',[49]='N',[50]='M',[51]='<',[52]='>',[53]='?',[57]=' '
};

void keyboard_init(void) { head = tail = 0; shift_down = caps_lock = extended = false; }
void keyboard_irq(void) {
    uint8_t code = inb(0x60);
    if (code == 0xE0) { extended = true; return; }
    if (extended) { extended = false; return; }
    if (code == 0x2A || code == 0x36) { shift_down = true; return; }
    if (code == 0xAA || code == 0xB6) { shift_down = false; return; }
    if (code == 0x3A) { caps_lock = !caps_lock; return; }
    if (code & 0x80) return;
    if (code >= 128) return;
    char c = (shift_down ? shifted : normal)[code];
    if (caps_lock && c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    if (!c) return;
    uint16_t next = (uint16_t)((head + 1u) % KEYBUF_SIZE);
    if (next != tail) { buffer[head] = c; head = next; }
}
int keyboard_read(char *out) {
    if (!out || tail == head) return 0;
    *out = buffer[tail];
    tail = (uint16_t)((tail + 1u) % KEYBUF_SIZE);
    return 1;
}
