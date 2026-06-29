#include "gdt.h"

// Indices: 0=null, 1=kernel code, 2=kernel data
static gdt_entry_t gdt[3];
static gdtr_t gdtr;

static void set_entry(int i, uint32_t base, uint32_t limit,
                      uint8_t access, uint8_t gran) {
    gdt[i].base_low  = base & 0xFFFF;
    gdt[i].base_mid  = (base >> 16) & 0xFF;
    gdt[i].base_high = (base >> 24) & 0xFF;
    gdt[i].limit_low  = limit & 0xFFFF;
    gdt[i].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[i].access = access;
}

// Defined in gdt_flush.asm
extern void gdt_flush(uint64_t gdtr_addr, uint16_t cs, uint16_t ds);

void gdt_init(void) {
    set_entry(0, 0, 0,          0x00, 0x00); // null
    set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xA0); // kernel code (64-bit)
    set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xC0); // kernel data

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base  = (uint64_t)&gdt;

    gdt_flush((uint64_t)&gdtr, 0x08, 0x10); // cs=0x08, ds=0x10
}