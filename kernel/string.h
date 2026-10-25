/* En-tête chaîne libre d'implémentation libc (kernel/string.c). */
#ifndef NEXOS_STRING_H
#define NEXOS_STRING_H
#include <stddef.h>

void *memset(void *dst, int value, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
size_t strlen(const char *s);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);
char *strcpy(char *dst, const char *src);
char *strcat(char *dst, const char *src);
char *strstr(const char *hay, const char *needle);

#endif /* NEXOS_STRING_H */
