#pragma once

void serial_init(void);
void serial_putc(char c);
void kprint_hex(unsigned long long val);
void kprint_decc(unsigned long long val);