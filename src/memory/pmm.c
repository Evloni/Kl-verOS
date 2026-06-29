#include <stdint.h>

#include "../limine.h"
#include "pmm.h"
#include <strings.h>

extern volatile struct limine_memmap_request memmap_request;
extern volatile struct limine_hhdm_request hhdm_request;

static uint8_t *bitmap = NULL;
static uint64_t bitmap_size = 0;
static uint64_t total_pages = 0;
static uint64_t hhdm_offset = 0;

// Helper macros
#define BIT_SET(i) (bitmap[(i) / 8] |= (1 << ((i) % 8)))
#define BIT_CLEAR(i) (bitmap[(i) / 8] &= ~(1 << ((i) % 8)))
#define BIT_TEST(i) (bitmap[(i) / 8] & (1 << ((i) % 8)))

void pmm_init(void) {
    struct limine_memmap_response *memmap = memmap_request.response;
    hhdm_offset = hhdm_request.response->offset;

    uint64_t highest_addr = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *e = memmap->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE) {
            uint64_t top = e->base + e->length;
            if (top > highest_addr) {
                highest_addr = top;
            }
        }
    }

    total_pages = highest_addr / PAGE_SIZE;
    bitmap_size = (total_pages + 7) / 8;

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *e = memmap->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE && e->length >= bitmap_size) {
            // Place bitmap here (use HHDM virtual address to write to it)
            bitmap = (uint8_t *)(e->base + hhdm_offset);
            break;
        }
    }

    memset(bitmap, 0xFF, bitmap_size);
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *e = memmap->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE) {
            for (uint64_t page = e->base / PAGE_SIZE;
                 page < (e->base + e->length) / PAGE_SIZE; page++) {
                BIT_CLEAR(page);
            }
        }
    }

    uint64_t bitmap_phys = (uint64_t)bitmap - hhdm_offset;
    uint64_t bitmap_pages = (bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t page = bitmap_phys / PAGE_SIZE;
         page < bitmap_phys / PAGE_SIZE + bitmap_pages; page++) {
        BIT_SET(page);
    }
}

void *pmm_alloc(void) {
    for (uint64_t i = 0; i < total_pages; i++) {
        if (!BIT_TEST(i)) {
            BIT_SET(i);
            return (void *)(i * PAGE_SIZE);
        }
    }
    return NULL;
}

void pmm_free(void *addr) {
    uint64_t page = (uint64_t)addr / PAGE_SIZE;
    BIT_CLEAR(page);
}