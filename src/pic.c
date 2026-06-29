#include "pic.h"
#include "io.h"
#include <stdint.h>

static inline void io_wait(void) {
  outb(0x80, 0); // write to unused port = small delay, needed on old hardware
}

void pic_init(void) {
  // Mask everything on both PICs first to avoid spurious interrupts during
  // remap
  outb(PIC1_DATA, 0xFF);
  outb(PIC2_DATA, 0xFF);

  // ICW1
  outb(PIC1_CMD, 0x11);
  io_wait();
  outb(PIC2_CMD, 0x11);
  io_wait();

  // ICW2: remap
  outb(PIC1_DATA, IRQ_BASE);
  io_wait();
  outb(PIC2_DATA, IRQ_BASE + 8);
  io_wait();

  // ICW3
  outb(PIC1_DATA, 0x04);
  io_wait();
  outb(PIC2_DATA, 0x02);
  io_wait();

  // ICW4
  outb(PIC1_DATA, 0x01);
  io_wait();
  outb(PIC2_DATA, 0x01);
  io_wait();

  // Leave everything masked — caller unmasks what it needs
  outb(PIC1_DATA, 0xFF);
  outb(PIC2_DATA, 0xFF);
}

void pic_eoi(uint8_t irq) {
  if (irq >= 8)
    outb(PIC2_CMD, PIC_EOI); // also ack slave
  outb(PIC1_CMD, PIC_EOI);
}

void pic_mask(uint8_t irq) {
  uint16_t port = irq < 8 ? PIC1_DATA : PIC2_DATA;
  uint8_t bit = irq < 8 ? irq : irq - 8;
  outb(port, inb(port) | (1 << bit));
}

void pic_unmask(uint8_t irq) {
  uint16_t port = irq < 8 ? PIC1_DATA : PIC2_DATA;
  uint8_t bit = irq < 8 ? irq : irq - 8;
  outb(port, inb(port) & ~(1 << bit));
}