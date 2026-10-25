#include <stddef.h>
#include <stdint.h>

void *memset(void *dst, int value, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    for (size_t i = 0; i < n; ++i) d[i] = (uint8_t)value;
    return dst;
}
void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    for (size_t i = 0; i < n; ++i) d[i] = s[i];
    return dst;
}
size_t strlen(const char *s) { size_t n = 0; if (s) while (s[n]) ++n; return n; }
int strcmp(const char *a, const char *b) {
    if (!a || !b) return (a != b);
    while (*a && *a == *b) { ++a; ++b; }
    return (uint8_t)*a - (uint8_t)*b;
}
int strncmp(const char *a, const char *b, size_t n) {
    while (n && *a && *a == *b) { ++a; ++b; --n; }
    return n ? (uint8_t)*a - (uint8_t)*b : 0;
}
char *strcpy(char *dst, const char *src) {
    char *out = dst;
    while ((*dst++ = *src++) != 0) { }
    return out;
}
char *strcat(char *dst, const char *src) { strcpy(dst + strlen(dst), src); return dst; }
char *strstr(const char *hay, const char *needle) {
    if (!*needle) return (char *)hay;
    for (; *hay; ++hay) {
        const char *h = hay, *n = needle;
        while (*h && *n && *h == *n) { ++h; ++n; }
        if (!*n) return (char *)hay;
    }
    return NULL;
}
