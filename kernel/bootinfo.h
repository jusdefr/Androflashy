/* NexOS — structure d'information passée par le chargeur (adresse fixe 0x0600). */
#ifndef NEXOS_BOOTINFO_H
#define NEXOS_BOOTINFO_H
#include <stdint.h>

#define BOOTINFO_ADDR 0x600u
#define BOOTINFO_MAGIC 0x4E455853u /* "NEXS" */

typedef struct {
    uint32_t magic;     /* BOOTINFO_MAGIC si renseigné */
    uint32_t version;   /* 1 */
    uint32_t fb_base;   /* adresse physique du framebuffer linéaire (0 = mode texte) */
    uint32_t pitch;     /* octets par ligne */
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t mode;      /* numéro de mode VBE, 0 si indisponible */
} bootinfo_t;

#endif /* NEXOS_BOOTINFO_H */
