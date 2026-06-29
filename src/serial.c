#include "serial.h"
#include "io.h" // For outb/inb
#include <stdint.h>

#define COM1 0x3F8

// outb and inb are now defined in io.c and declared in io.h
// serial.c includes io.h to use them.

void serial_init(void) {
  outb(COM1 + 1, 0x00); // disable interrupts
  outb(COM1 + 3, 0x80); // enable DLAB (set baud rate divisor)
  outb(COM1 + 0, 0x03); // divisor low byte: 3 → 38400 baud
  outb(COM1 + 1, 0x00); // divisor high byte
  outb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
  outb(COM1 + 2, 0xC7); // enable FIFO, clear, 14-byte threshold
  outb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

static int serial_ready(void) { return inb(COM1 + 5) & 0x20; }

void serial_putc(char c) {
  while (!serial_ready())
    ;
  outb(COM1, (uint8_t)c);
  if (c == '\n') // also send \r so terminals behave
    serial_putc('\r');
}

void kprint_hex(unsigned long long val) {
  kprint("0x");
  char buf[17];
  buf[16] = '\0';
  for (int i = 15; i >= 0; i--) {
    int nibble = val & 0xF;
    buf[i] = nibble < 10 ? '0' + nibble : 'a' + nibble - 10;
    val >>= 4;
  }
  // trim leading zeros (keep at least one digit)
  char *p = buf;
  while (*p == '0' && *(p + 1))
    p++;
  kprint(p);
}

void kprint_decc(unsigned long long val) {
  if (val == 0) {
    serial_putc('0');
    return;
  }
  char buf[21];
  int i = 20;
  buf[i] = '\0';
  while (val > 0) {
    buf[--i] = '0' + (val % 10);
    val /= 10;
  }
  kprint(&buf[i]);
}