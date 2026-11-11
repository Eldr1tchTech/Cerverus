#pragma once

#include "core/memory/cmem.h"
#include "core/util/bitwise.h"

typedef struct pool_allocator
{
    size_t stride;
    bitarray* availability;
    void* data;
} pool_alloc_t;

pool_alloc_t* pool_allocator_create(size_t stride, size_t size);
void pool_allocator_destroy(pool_alloc_t* pool_alloc);

void* pool_allocator_alloc(pool_alloc_t* pool_alloc);
void pool_allocator_free(pool_alloc_t* pool_alloc, void* block);