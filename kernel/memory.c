#include "kernel.h"

/* Tas statique de base ; pas de libc, allocation à blocs et fusion au free. */
#define HEAP_BYTES (64u * 1024u)
#define HEAP_MAGIC 0x4E584850u
#define ALIGN_UP(n) (((n) + 7u) & ~7u)

typedef struct block {
    uint32_t magic, size;
    struct block *next;
    uint8_t free;
    uint8_t reserved[3];
} block_t;
static union { uint64_t align; uint8_t bytes[HEAP_BYTES]; } heap;
static block_t *first;
static bool initialized;

void memory_init(void) {
    first = (block_t *)heap.bytes;
    first->magic = HEAP_MAGIC;
    first->size = HEAP_BYTES - (uint32_t)sizeof(block_t);
    first->next = NULL;
    first->free = 1;
    initialized = true;
    klog("INFO", "allocateur noyau statique initialise (64 KiB)");
}
void *kmalloc(size_t requested) {
    if (!requested || requested > HEAP_BYTES - sizeof(block_t)) return NULL;
    if (!initialized) memory_init();
    uint32_t size = ALIGN_UP((uint32_t)requested);
    uint32_t flags;
    __asm__ volatile("pushfl; popl %0; cli" : "=r"(flags) :: "memory");
    for (block_t *b = first; b; b = b->next) {
        if (b->magic != HEAP_MAGIC) kernel_panic("corruption du tas noyau");
        if (!b->free || b->size < size) continue;
        if (b->size >= size + sizeof(block_t) + 8u) {
            block_t *tail = (block_t *)((uint8_t *)(b + 1) + size);
            tail->magic = HEAP_MAGIC;
            tail->size = b->size - size - (uint32_t)sizeof(block_t);
            tail->next = b->next;
            tail->free = 1;
            b->next = tail;
            b->size = size;
        }
        b->free = 0;
        __asm__ volatile("pushl %0; popfl" :: "r"(flags) : "memory", "cc");
        return b + 1;
    }
    __asm__ volatile("pushl %0; popfl" :: "r"(flags) : "memory", "cc");
    return NULL;
}
void kfree(void *ptr) {
    if (!ptr) return;
    if ((uint8_t *)ptr < heap.bytes + sizeof(block_t) || (uint8_t *)ptr >= heap.bytes + HEAP_BYTES)
        kernel_panic("kfree sur adresse hors du tas");
    uint32_t flags;
    __asm__ volatile("pushfl; popl %0; cli" : "=r"(flags) :: "memory");
    block_t *b = (block_t *)ptr - 1;
    if (b->magic != HEAP_MAGIC || b->free) kernel_panic("kfree invalide ou double liberation");
    b->free = 1;
    for (block_t *cur = first; cur && cur->next;) {
        if (cur->free && cur->next->free) {
            if (cur->next->magic != HEAP_MAGIC) kernel_panic("corruption du tas noyau");
            cur->size += (uint32_t)sizeof(block_t) + cur->next->size;
            cur->next = cur->next->next;
        } else cur = cur->next;
    }
    __asm__ volatile("pushl %0; popfl" :: "r"(flags) : "memory", "cc");
}
