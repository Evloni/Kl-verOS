#pragma once
#include <stdint.h>

void keyboard_init(void);
char keyboard_getchar(void);
int keyboard_haschar(void);
void keyboard_handler(void);