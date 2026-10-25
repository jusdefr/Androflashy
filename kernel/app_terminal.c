/* NexOS - Terminal : fenêtre console avec commandes noyau de base. */
#include "app_terminal.h"
#include "gfx.h"
#include "theme.h"
#include "kernel.h"
#include "string.h"

#define ROWS 24
#define COLS 76
#define LINE_STEP 16
#define INPUT_MAX 64

static char cells[ROWS][COLS + 1];
static uint32_t row_count;
static uint32_t cols;
static uint32_t view_cols = COLS; /* colonnes réellement visibles (suivant fenêtre) */
static uint32_t view_rows = ROWS; /* rangées réellement visibles             */
#define CHAR_W 8
static char input_line[INPUT_MAX];
static size_t input_len;
static bool initialized;

static const char *to_num(uint32_t v) {
    static char buffer[12];
    if (!v) return "0";
    char digits[12];
    unsigned n = 0;
    while (v && n < sizeof(digits)) {
        digits[n++] = (char)('0' + v % 10u);
        v /= 10u;
    }
    unsigned i = 0;
    while (n) buffer[i++] = digits[--n];
    buffer[i] = 0;
    return buffer;
}

static void put_char(char c) {
    if (c == '\n') {
        ++row_count;
        return;
    }
    uint32_t r = row_count % ROWS;
    uint32_t limit = (view_cols && view_cols < (uint32_t)COLS) ? view_cols : (uint32_t)COLS;
    uint32_t x = 0;
    while (x < limit && cells[r][x]) ++x;
    if (x == limit) {
        ++row_count;
        x = 0;
    }
    cells[row_count % ROWS][x] = c;
}

static void put_line(const char *s) {
    while (*s) put_char(*s++);
    put_char('\n');
}

static void terminal_init(void) {
    cols = COLS;
    memset(cells, 0, sizeof(cells));
    row_count = 0;
    input_len = 0;
    put_line("NexOS 0.1.0-dev - Terminal ring 0 (prototype).");
    put_line("Commandes : help, about, uptime, mem, clear.");
    initialized = true;
}

static void execute(const char *cmd) {
    if (strncmp(cmd, "help", 5) == 0) {
        put_line("  help    aide");
        put_line("  about   version");
        put_line("  uptime  secondes");
        put_line("  mem     allocation");
        put_line("  clear   efface");
    } else if (strncmp(cmd, "about", 6) == 0) {
        put_line("NexOS 0.1.0-dev | noyau x86 32 bits | C freestanding | BIOS/VBE");
        serial_write("SHELL_RESULT NexOS 0.1.0-dev\r\n");
        serial_write("SHELL_RESULT NexOS 0.1.0-dev GUI\r\n");
    } else if (strncmp(cmd, "uptime", 7) == 0) {
        char line[40];
        strcpy(line, "Temps actif : ");
        strcat(line, to_num(timer_ticks() / 100u));
        strcat(line, " seconde(s)");
        put_line(line);
    } else if (strncmp(cmd, "mem", 4) == 0) {
        void *probe = kmalloc(64);
        if (probe) {
            kfree(probe);
            put_line("Tas noyau : allocation 64 o puis libération OK");
        } else
            put_line("Tas noyau : allocation échouée");
    } else if (strncmp(cmd, "clear", 6) == 0) {
        memset(cells, 0, sizeof(cells));
        row_count = 0;
    } else if (cmd[0])
        put_line("Commande inconnue : tapez 'help'.");
}

void app_terminal_key(char key) {
    if (!initialized) terminal_init();
    if (key == '\n') {
        input_line[input_len] = 0;
        /* la commande a déjà été échoée caractère par caractère */
        execute(input_line);
        input_len = 0;
    } else if (key == '\b') {
        if (input_len) --input_len;
    } else if (key >= 32 && key < 127 && input_len + 1 < INPUT_MAX) {
        input_line[input_len++] = key;
        put_char(key);
    }
}

void app_terminal_draw(int32_t x, int32_t y, int32_t w, int32_t h) {
    if (!initialized) terminal_init();
    const theme_palette_t *t = theme();
    /* la vue suit la taille de la fenêtre (bornée au tampon annulaire) */
    uint32_t wanted_cols = (uint32_t)((w - 20) / CHAR_W);
    if (wanted_cols < 16u) wanted_cols = 16u;
    if (wanted_cols > (uint32_t)COLS) wanted_cols = (uint32_t)COLS;
    view_cols = wanted_cols;
    uint32_t wanted_rows = (uint32_t)((h - 12) / LINE_STEP);
    if (wanted_rows < 3u) wanted_rows = 3u;
    view_rows = wanted_rows > (uint32_t)ROWS ? (uint32_t)ROWS : wanted_rows;
    /* fenêtre glissante : toujours la fin du tampon, prompt en dernière ligne */
    uint32_t start = row_count > view_rows - 1u ? row_count - (view_rows - 1u) : 0u;
    for (uint32_t i = 0; i + 1u < view_rows; ++i) {
        uint32_t r = (start + i) % ROWS;
        gfx_text(x + 10, y + 6 + (int32_t)i * LINE_STEP, cells[r], 0xFFE4E8F2, GFX_COLOR_TRANSPARENT);
    }
    char line[INPUT_MAX + 12];
    strcpy(line, "nexos > ");
    strcat(line, input_line);
    gfx_text(x + 10, y + 6 + (int32_t)(view_rows - 1u) * LINE_STEP, line, t->accent, GFX_COLOR_TRANSPARENT);
}
