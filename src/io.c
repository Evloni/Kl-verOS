#include "io.h"
#include "font8x16.h"
#include "limine.h"
#include <stdarg.h>
#include <stdint.h>

// Global variables for framebuffer and cursor
int cursor_x = 0;
int cursor_y = 0;
uint32_t *g_pixels;
uint64_t g_pitch;

// Limine framebuffer pointer (initialized by kmain)
struct limine_framebuffer *fb;

// Initialize framebuffer globals
void io_init(struct limine_framebuffer *fb_info) {
  fb = fb_info;
  g_pixels = (uint32_t *)fb->address;
  g_pitch = fb->pitch / 4; // pitch is in bytes, g_pixels is uint32_t
}

// Hardware I/O functions
void outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

uint8_t inb(uint16_t port) {
  uint8_t val;
  __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
  return val;
}

// Printing Chars and strings to framebuffer
void scroll() {
  // Move every row up by 16 pixels (one line)
  for (uint64_t y = 0; y < fb->height - 16; y++)
    for (uint64_t x = 0; x < fb->width; x++)
      g_pixels[y * g_pitch + x] = g_pixels[(y + 16) * g_pitch + x];

  // Clear the last line
  for (uint64_t y = fb->height - 16; y < fb->height; y++)
    for (uint64_t x = 0; x < fb->width; x++)
      g_pixels[y * g_pitch + x] = 0x000000;

  cursor_y -= 16;
}

void drawChar(uint32_t *pixels, uint64_t pitch, char c, int x, int y,
              uint32_t fg, uint32_t bg) {
  uint8_t *glyph = &font8x16[((uint8_t)c - 0x20) * 16];
  for (int row = 0; row < 16; row++) {
    for (int col = 0; col < 8; col++) {
      uint32_t color = (glyph[row] >> (7 - col)) & 1 ? fg : bg;
      pixels[(y + row) * pitch + (x + col)] = color;
    }
  }
}

void kputchar(char c) {
  if (c == '\n') {
    cursor_x = 0;
    cursor_y += 16;
  } else {
    drawChar(g_pixels, g_pitch, c, cursor_x, cursor_y, 0xFFFFFF, 0x000000);
    cursor_x += 8;
    if (cursor_x + 8 > (int)fb->width) {
      cursor_x = 0;
      cursor_y += 16;
    }
  }

  if (cursor_y + 16 > (int)fb->height)
    scroll();
}

void kprint(const char *s) {
  for (; *s; s++) {
    kputchar(*s);
  }
}
void kprint_dec(unsigned long long val) {
  if (val == 0) {
    kputchar('0');
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

void kprintf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  for (const char *p = fmt; *p != '\0'; p++) {
    if (*p != '%') {
      kputchar(*p);
      continue;
    }

    p++;
    switch (*p) {
    case 's': {
      char *s = va_arg(args, char *);
      kprint(s);
      break;
    }
    case 'd': {
      long long val = va_arg(args, long long);
      kprint_dec(val);
      break;
    }
    case 'x': {
      unsigned int val = va_arg(args, unsigned int);
      kprint_hex((unsigned long long)val);
      break;
    }
    case 'p': {
      unsigned long long val = va_arg(
          args, unsigned long long); // pointers are 64-bit, this one is fine
      kprint_hex(val);
      break;
    }
    case 'c': {
      char c = (char)va_arg(args, int);
      kputchar(c);
      break;
    }
    case '%': {
      kputchar('%');
      break;
    }
    default:
      kputchar(*p);
      break;
    }
  }
  va_end(args);
}
