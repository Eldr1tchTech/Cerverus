#pragma once

#include "core/memory/cmem.h"
#include "core/util/bitwise.h"

typedef struct pool_allocator
{
    size_t stride;
    bitarray* availability;
    void* data;
} pool_allocator;

pool_allocator* pool_allocator_create(size_t stride, size_t size);
void pool_allocator_destroy(pool_allocator* pool_alloc);

void* pool_allocator_alloc(pool_allocator* pool_alloc);
void pool_allocator_free(pool_allocator* pool_alloc, void* block);