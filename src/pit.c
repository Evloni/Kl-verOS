#include "pit.h"
#include "io.h"
#include <stdint.h>

static volatile uint64_t ticks = 0; // make sure this is volatile

void pit_init(uint32_t hz) {
  uint32_t divisor = PIT_BASE_HZ / hz;

  // Channel 0, lobyte/hibyte, square wave generator (mode 3)
  outb(PIT_CMD, 0x36);
  outb(PIT_CHANNEL0, divisor & 0xFF);
  outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
}

uint64_t pit_ticks(void) { return ticks; }

void pit_handler(void) { ticks++; }