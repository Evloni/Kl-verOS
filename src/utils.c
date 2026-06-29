// Halt and catch fire function.
void hcf(void) {
    for (;;) {
        __asm__("hlt");
    }
}