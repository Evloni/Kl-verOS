// vmm.c
#include "vmm.h"
#include "../limine.h"
#include "pmm.h"
#include <io.h>
#include <strings.h>

// Limine requests (defined in your kernel entry file)
extern volatile struct limine_memmap_request memmap_request;
extern volatile struct limine_hhdm_request hhdm_request;
extern volatile struct limine_executable_address_request kaddr_request;

pagemap_t kernel_pagemap;
static uint64_t hhdm_offset;

// Convert physical address to virtual (via HHDM) so we can write to it
void *phys_to_virt(uint64_t phys) { return (void *)(phys + hhdm_offset); }

// Allocate a zeroed page for a new page table level
static pte_t *alloc_table(void) {
    pte_t *table = pmm_alloc();
    memset(phys_to_virt((uint64_t)table), 0, PAGE_SIZE);
    return table; // physical address
}

// Walk one level of the page table, creating the next level if missing
static pte_t *get_or_create_table(pte_t *table_phys, size_t idx,
                                  uint64_t flags) {
    pte_t *table = phys_to_virt((uint64_t)table_phys);
    pte_t entry = table[idx];

    if (entry & PAGE_PRESENT) {
        // Already exists — return the physical address of the next level
        return (pte_t *)ENTRY_PHYS(entry);
    }

    // Allocate a new table
    pte_t *child = alloc_table();
    table[idx] = (uint64_t)child | flags | PAGE_PRESENT;
    return child;
}

// Map one 4KiB page: virt → phys with given flags
void vmm_map(pagemap_t *pm, uint64_t virt, uint64_t phys, uint64_t flags) {
    pte_t *pdpt = get_or_create_table(pm->pml4, PML4_IDX(virt), PAGE_WRITE);
    pte_t *pd = get_or_create_table(pdpt, PDPT_IDX(virt), PAGE_WRITE);
    pte_t *pt = get_or_create_table(pd, PD_IDX(virt), PAGE_WRITE);

    // Write the final PT entry
    pte_t *pt_virt = phys_to_virt((uint64_t)pt);
    pt_virt[PT_IDX(virt)] = phys | flags | PAGE_PRESENT;
}

// Load a pagemap into CR3
void vmm_switch(pagemap_t *pm) {
    __asm__ volatile("mov %0, %%cr3" : : "r"((uint64_t)pm->pml4) : "memory");
}

void vmm_init(void) {
    hhdm_offset = hhdm_request.response->offset;

    // Allocate a fresh PML4 for the kernel
    kernel_pagemap.pml4 = alloc_table();

    struct limine_memmap_response *memmap = memmap_request.response;
    /* kaddr_request's type may be incomplete here; avoid accessing its
      .response member directly. */
    struct limine_executable_address_response *kaddr = kaddr_request.response;

    // 1. Map all usable physical memory into the HHDM
    //    (same layout Limine gave us, so existing pointers keep working)
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *e = memmap->entries[i];

        // Map every meaningful region
        if (e->type == LIMINE_MEMMAP_USABLE ||
            e->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE ||
            e->type == LIMINE_MEMMAP_EXECUTABLE_AND_MODULES ||
            e->type == LIMINE_MEMMAP_FRAMEBUFFER) {

            // Align base down, top up
            uint64_t base = e->base & ~(uint64_t)(PAGE_SIZE - 1);
            uint64_t top = (e->base + e->length + PAGE_SIZE - 1) &
                           ~(uint64_t)(PAGE_SIZE - 1);

            for (uint64_t phys = base; phys < top; phys += PAGE_SIZE) {
                vmm_map(&kernel_pagemap,
                        phys + hhdm_offset, // virtual
                        phys,               // physical
                        PAGE_WRITE);
            }
        }
    }

    // 2. Map the kernel itself at its virtual address
    //    kaddr gives us both the physical and virtual base Limine used
    if (kaddr == NULL)
        return;

    uint64_t kern_phys = kaddr->physical_base;
    uint64_t kern_virt = kaddr->virtual_base;

    // Map 64 MiB — more than enough for any starter kernel
    // In a real kernel you'd parse the ELF and map each section precisely
    for (uint64_t off = 0; off < 0x4000000; off += PAGE_SIZE) {
        vmm_map(&kernel_pagemap, kern_virt + off, kern_phys + off, PAGE_WRITE);
    }

    // 3. Switch to our new page tables
    vmm_switch(&kernel_pagemap);
}