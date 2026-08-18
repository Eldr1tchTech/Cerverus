#include "cmem.h"

#include <stdlib.h>
#include <string.h>

void *cmem_alloc(size_t size) {
  void *block = malloc(size);
  cmem_zmem(block, size);
  return block;
}

void cmem_free(void *block) { free(block); }

void cmem_zmem(void *block, size_t size) { memset(block, 0, size); }

void cmem_mcpy(void *dest, void *src, size_t size) { memcpy(dest, src, size); }
