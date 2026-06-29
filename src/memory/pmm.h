#pragma once
#include <stddef.h>

#define PAGE_SIZE 4096

void pmm_init(void);
void *pmm_alloc(void);
void pmm_free(void *addr);