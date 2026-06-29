// vmm.h
#pragma once
#include <stddef.h>
#include <stdint.h>

#define PAGE_PRESENT (1ull << 0)
#define PAGE_WRITE (1ull << 1)
#define PAGE_USER (1ull << 2) // set for user-space pages
#define PAGE_NX (1ull << 63)  // no-execute

// Extract indices from a virtual address
#define PML4_IDX(va) (((uint64_t)(va) >> 39) & 0x1FF)
#define PDPT_IDX(va) (((uint64_t)(va) >> 30) & 0x1FF)
#define PD_IDX(va) (((uint64_t)(va) >> 21) & 0x1FF)
#define PT_IDX(va) (((uint64_t)(va) >> 12) & 0x1FF)

// Strip flags from an entry to get the physical address
#define ENTRY_PHYS(e) ((e) & 0x000FFFFFFFFFF000ull)

typedef uint64_t pte_t;

typedef struct {
  pte_t *pml4; // physical address of PML4
} pagemap_t;

void vmm_init(void);
void vmm_map(pagemap_t *pm, uint64_t virt, uint64_t phys, uint64_t flags);
void vmm_switch(pagemap_t *pm);
void *phys_to_virt(uint64_t phys);

extern pagemap_t kernel_pagemap;