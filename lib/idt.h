#pragma once
#include "serial.h"
#include <stdint.h>
typedef struct __attribute__((packed)) {
  uint16_t offset_low;
  uint16_t selector; // kernel code segment (0x08)
  uint8_t ist;       // interrupt stack table offset (0 = none)
  uint8_t type_attr; // gate type, DPL, present
  uint16_t offset_mid;
  uint32_t offset_high;
  uint32_t reserved;
} idt_entry_t;

typedef struct __attribute__((packed)) {
  uint16_t limit;
  uint64_t base;
} idtr_t;

void idt_init(void);