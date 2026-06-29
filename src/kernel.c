#include "../lib/boot.h"
#include "../lib/utils.h"
#include "gdt.h"
#include "idt.h"
#include "io.h"
#include "keyboard.h"
#include "serial.h"
#include "limine.h"
#include "memory/heap.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "pic.h"
#include "pit.h"

// Disable the Local APIC to allow legacy PIC interrupts to reach the CPU
static void disable_lapic(void) {
    uint32_t low, high;
    // APIC_BASE MSR is 0x1B. Bit 11 is the global enable bit.
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(0x1B));
    low &= ~(1 << 11);
    __asm__ volatile("wrmsr" : : "a"(low), "d"(high), "c"(0x1B));
}

void kmain(void) {
    struct limine_framebuffer *framebuffer = initBoot();
    serial_init();
    io_init(framebuffer);
    kprint("#######################\n");
    kprint("##     KLOVER OS     ##\n");
    kprint("#######################\n\n");

    gdt_init();
    kprint("GDT initialized\n");

    idt_init();
    kprint("IDT initialized\n");

    disable_lapic();
    kprint("LAPIC disabled\n");

    pic_init();
    kprint("PIC remapped\n");

    pit_init(1000);
    pic_unmask(0); // unmask timer — and don't mask it again!
    kprint("PIT initialized\n");

    pmm_init();
    kprint("PMM initialized\n");

    vmm_init();
    kprint("VMM initialized\n");

    heap_init();
    kprint("Heap initialized\n");

    keyboard_init();
    kprintf("Keyboard initialized\n");

    kprint("Booted succsessfully\n");

    while (1) {
        char c = keyboard_getchar();
        kputchar(c);
    }
}
