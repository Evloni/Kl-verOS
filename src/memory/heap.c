#include "heap.h"
#include "pmm.h"
#include "strings.h"
#include "vmm.h"
#include <stdint.h>

#define HEAP_START 0xFFFF900000000000ull // arbitrary high virt address
#define HEAP_INIT_PAGES 16               // 64 KiB to start
#define MIN_SPLIT_SIZE 32                // don't split if remainder < this

typedef struct block_header {
  size_t size;               // usable bytes (excluding header)
  int free;                  // 1 = free, 0 = used
  struct block_header *next; // next block in list
  struct block_header *prev; // previous block in list
} block_header_t;

static block_header_t *heap_head = NULL;
static uint64_t heap_virt = HEAP_START;
static size_t heap_size = 0;

extern pagemap_t kernel_pagemap;
extern uint64_t hhdm_offset; // expose from vmm.c

// Grow the heap by mapping more pages
static void heap_grow(size_t needed) {
  size_t pages = (needed + PAGE_SIZE - 1) / PAGE_SIZE;
  if (pages < HEAP_INIT_PAGES)
    pages = HEAP_INIT_PAGES;

  for (size_t i = 0; i < pages; i++) {
    void *phys = pmm_alloc();
    vmm_map(&kernel_pagemap, heap_virt + heap_size + i * PAGE_SIZE,
            (uint64_t)phys, PAGE_WRITE);
  }
  heap_size += pages * PAGE_SIZE;
}

void heap_init(void) {
  heap_grow(HEAP_INIT_PAGES * PAGE_SIZE);

  // Place the initial block covering all heap space
  heap_head = (block_header_t *)heap_virt;
  heap_head->size = heap_size - sizeof(block_header_t);
  heap_head->free = 1;
  heap_head->next = NULL;
  heap_head->prev = NULL;
}

// Split a block if it's large enough to fit 'size' + a new header +
// MIN_SPLIT_SIZE
static void split_block(block_header_t *blk, size_t size) {
  if (blk->size < size + sizeof(block_header_t) + MIN_SPLIT_SIZE)
    return;

  block_header_t *new_blk =
      (block_header_t *)((uint8_t *)blk + sizeof(block_header_t) + size);

  new_blk->size = blk->size - size - sizeof(block_header_t);
  new_blk->free = 1;
  new_blk->next = blk->next;
  new_blk->prev = blk;

  if (blk->next)
    blk->next->prev = new_blk;
  blk->next = new_blk;
  blk->size = size;
}

// Merge block with next if both are free
static void coalesce(block_header_t *blk) {
  if (blk->next && blk->next->free) {
    blk->size += sizeof(block_header_t) + blk->next->size;
    blk->next = blk->next->next;
    if (blk->next)
      blk->next->prev = blk;
  }
  if (blk->prev && blk->prev->free) {
    blk->prev->size += sizeof(block_header_t) + blk->size;
    blk->prev->next = blk->next;
    if (blk->next)
      blk->next->prev = blk->prev;
  }
}

void *kmalloc(size_t size) {
  if (size == 0)
    return NULL;

  // Align size to 16 bytes
  size = (size + 15) & ~(size_t)15;

  block_header_t *blk = heap_head;
  while (blk) {
    if (blk->free && blk->size >= size) {
      split_block(blk, size);
      blk->free = 0;
      return (void *)((uint8_t *)blk + sizeof(block_header_t));
    }
    blk = blk->next;
  }

  // No block found — grow the heap
  size_t grow_size = size + sizeof(block_header_t);
  heap_grow(grow_size);

  // The new pages are unmapped virtual memory — create a block for them
  // Find the last block and expand/append
  block_header_t *last = heap_head;
  while (last->next)
    last = last->next;

  if (last->free) {
    // Extend the last free block
    last->size +=
        heap_size - ((uint64_t)last - heap_virt) - sizeof(block_header_t);
  } else {
    // Append a new block after the last one
    block_header_t *new_blk =
        (block_header_t *)((uint8_t *)last + sizeof(block_header_t) +
                           last->size);
    new_blk->size =
        heap_size - ((uint64_t)new_blk - heap_virt) - sizeof(block_header_t);
    new_blk->free = 1;
    new_blk->next = NULL;
    new_blk->prev = last;
    last->next = new_blk;
  }

  return kmalloc(size); // retry
}

void *kcalloc(size_t count, size_t size) {
  void *ptr = kmalloc(count * size);
  if (ptr)
    memset(ptr, 0, count * size);
  return ptr;
}

void *krealloc(void *ptr, size_t size) {
  if (!ptr)
    return kmalloc(size);
  if (!size) {
    kfree(ptr);
    return NULL;
  }

  block_header_t *blk =
      (block_header_t *)((uint8_t *)ptr - sizeof(block_header_t));

  if (blk->size >= size)
    return ptr; // already big enough

  void *new_ptr = kmalloc(size);
  if (!new_ptr)
    return NULL;
  memcpy(new_ptr, ptr, blk->size);
  kfree(ptr);
  return new_ptr;
}

void kfree(void *ptr) {
  if (!ptr)
    return;

  block_header_t *blk =
      (block_header_t *)((uint8_t *)ptr - sizeof(block_header_t));
  blk->free = 1;
  coalesce(blk);
}
