#include "keyboard.h"
#include "io.h"
#include "pic.h"

#define KEYBOARD_DATA_PORT 0x60

// Ring buffer
#define KB_BUFFER_SIZE 256
static char kb_buffer[KB_BUFFER_SIZE];
static volatile uint8_t kb_read = 0;
static volatile uint8_t kb_write = 0;

static void kb_buffer_push(char c) {
  uint8_t next = (kb_write + 1) % KB_BUFFER_SIZE;
  if (next != kb_read) {
    kb_buffer[kb_write] = c;
    kb_write = next;
  }
}

static char kb_buffer_pop(void) {
  if (kb_read == kb_write)
    return 0;
  char c = kb_buffer[kb_read];
  kb_read = (kb_read + 1) % KB_BUFFER_SIZE;
  return c;
}

// US QWERTY scancode set 1
static const char scancode_table[128] = {
    0,    0,    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-',  '=',
    '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[',  ']',
    '\n', 0,    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,    '*',
    0,    ' ',  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,
    0,    0,    0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,   0,    0,
    0,    0,    0,   0,   0,   0,   0,   0,
};

static const char scancode_table_shift[128] = {
    0,    0,    '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
    '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
    '\n', 0,    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,    '|',  'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*',
    0,    ' ',  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,    0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,   0,   0,
    0,    0,    0,   0,   0,   0,   0,   0,
};

#define SC_LSHIFT 0x2A
#define SC_RSHIFT 0x36
#define SC_LSHIFT_REL 0xAA
#define SC_RSHIFT_REL 0xB6
#define SC_CAPS 0x3A

static int shift_held = 0;
static int caps_lock = 0;

void keyboard_handler(void) {
  uint8_t scancode = inb(KEYBOARD_DATA_PORT);

  if (scancode == SC_LSHIFT || scancode == SC_RSHIFT) {
    shift_held = 1;
    return;
  }
  if (scancode == SC_LSHIFT_REL || scancode == SC_RSHIFT_REL) {
    shift_held = 0;
    return;
  }
  if (scancode == SC_CAPS) {
    caps_lock = !caps_lock;
    return;
  }
  if (scancode & 0x80)
    return; // key release, ignore
  if (scancode >= 128)
    return; // extended, no mapping

  char c =
      shift_held ? scancode_table_shift[scancode] : scancode_table[scancode];
  if (!shift_held && caps_lock && c >= 'a' && c <= 'z')
    c -= 32;
  if (c)
    kb_buffer_push(c);
}

void keyboard_init(void) { pic_unmask(1); }

int keyboard_haschar(void) { return kb_read != kb_write; }

char keyboard_getchar(void) {
  while (!keyboard_haschar()) {
    __asm__ volatile("sti");
  }
  return kb_buffer_pop();
}