#pragma once
#include <stdint.h>

#define PIT_CHANNEL0 0x40
#define PIT_CMD 0x43
#define PIT_BASE_HZ 1193182 // PIT runs at ~1.19 MHz

void pit_init(uint32_t hz);
uint64_t pit_ticks(void);
void pit_handler(void);