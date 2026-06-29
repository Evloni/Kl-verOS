#include "idt.h"
#include "io.h"
#include "keyboard.h"
#include "pic.h"
#include "pit.h"
#include "serial.h"
#include <stdint.h>

static void serial_puts(const char *s) {
  for (; *s; s++) serial_putc(*s);
}

static void serial_hex(uint64_t v) {
  serial_puts("0x");
  for (int i = 60; i >= 0; i -= 4) {
    int n = (v >> i) & 0xF;
    serial_putc(n < 10 ? '0' + n : 'a' + n - 10);
  }
}

#define IDT_ENTRIES 256

static idt_entry_t idt[IDT_ENTRIES];
static volatile idtr_t idtr;

// ISR stubs declared in isr.asm
extern void *isr_table[IDT_ENTRIES]; // array of stub addresses

static void set_gate(int vec, void *handler, uint8_t type_attr) {
  uint64_t addr = (uint64_t)handler;
  idt[vec].offset_low = addr & 0xFFFF;
  idt[vec].offset_mid = (addr >> 16) & 0xFFFF;
  idt[vec].offset_high = (addr >> 32) & 0xFFFFFFFF;
  idt[vec].selector = 0x08; // kernel CS
  idt[vec].ist = 0;
  idt[vec].type_attr = type_attr;
  idt[vec].reserved = 0;
}

// The C handler — called from assembly stubs
void exception_handler(uint64_t vec, uint64_t err, uint64_t rip) {
  if (vec >= 32) {
    uint8_t irq = vec - 32;
    // hardware IRQ — acknowledge and dispatch
    switch (irq) {
    case 0:
      pit_handler();
      break;
    case 1:
      keyboard_handler(); // add this
      break;
    default:
      break;
    }

    pic_eoi(irq);
    return;
  }

  // CPU exception — print via serial (safe after vmm_switch) and framebuffer
  serial_puts("EXCEPTION: vector=");
  serial_hex(vec);
  serial_puts(" err=");
  serial_hex(err);
  serial_puts(" rip=");
  serial_hex(rip);
  uint64_t cr2;
  __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
  serial_puts(" cr2=");
  serial_hex(cr2);
  serial_puts("\n");
  kprint("EXCEPTION: vector=");
  kprint_dec(vec);
  kprint(" err=");
  kprint_hex(err);
  kprint(" rip=");
  kprint_hex(rip);
  kprint("\n");

  for (;;)
    __asm__ volatile("hlt");
}

void idt_init(void) {
  // 0x8E = Present, DPL=0, Interrupt gate (clears IF on entry)
  for (int i = 0; i < IDT_ENTRIES; i++)
    set_gate(i, isr_table[i], 0x8E);

  idtr.limit = sizeof(idt) - 1;
  idtr.base = (uint64_t)&idt;

  // Use the address of idtr explicitly to ensure the 10-byte structure is
  // loaded
  __asm__ volatile("lidt (%0)" : : "r"(&idtr) : "memory");
  __asm__ volatile("sti");
}