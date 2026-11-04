#pragma once

#include "defines.h"

void *cmem_alloc(size_t size);

void *cmem_realloc(void *block, size_t size);

void cmem_free(void *block);

void cmem_zmem(void *block, size_t size);

void cmem_mcpy(void *dest, void *src, size_t size);

void cmem_memmove(void *dest, const void *src, size_t size);

#define cmem_new(T, ...) ({                  \
    T *_p = cmem_alloc(sizeof(T));           \
    *_p = (T){__VA_ARGS__};                  \
    _p;                                      \
})