#pragma once

#include "defines.h"

void *cmem_alloc(size_t size);

void cmem_free(void *block);

void cmem_zmem(void *block, size_t size);

void cmem_mcpy(void *dest, void *src, size_t size);

void cmem_memmove(void *dest, const void *src, size_t size);