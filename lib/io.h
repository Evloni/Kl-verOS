#pragma once
#include "limine.h"
#include "serial.h"
#include <stdint.h>

extern int cursor_x;
extern int cursor_y;
extern uint32_t *g_pixels;
extern uint64_t g_pitch;

void io_init(struct limine_framebuffer *fb_info);

extern void outb(uint16_t port, uint8_t val);
extern uint8_t inb(uint16_t port);

void drawChar(uint32_t *pixels, uint64_t pitch, char c, int x, int y,
              uint32_t fg, uint32_t bg);
void kputchar(char c);
void kprint(const char *s);
void kprintf(const char *fmt, ...);
void kprint_dec(unsigned long long val);