#pragma once
#include <stdint.h>

// A GDT entry is 8 bytes
typedef struct __attribute__((packed)) {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;      // Present, DPL, S, Type
    uint8_t  granularity; // G, DB, L, AVL, limit_high
    uint8_t  base_high;
} gdt_entry_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;  // sizeof(gdt) - 1
    uint64_t base;   // address of the GDT
} gdtr_t;

void gdt_init(void);