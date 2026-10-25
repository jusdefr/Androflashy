#ifndef NEXOS_APP_BROWSER_H
#define NEXOS_APP_BROWSER_H
#include <stdint.h>
#include "../kernel/desktop.h"

void app_browser_draw(int32_t x, int32_t y, int32_t w, int32_t h);
void app_browser_key(char key);

#endif /* NEXOS_APP_BROWSER_H */
