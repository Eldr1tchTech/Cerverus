#pragma once

#include "defines.h"

typedef struct bump_allocator
{
    size_t size;
    void* cursor;
    void* data;
} bump_allocator;

bump_allocator* bump_allocator_create(size_t size);
void bump_allocator_destroy(bump_allocator* bump_alloc);

void* bump_allocator_alloc(bump_allocator* bump_alloc, size_t size);
void bump_alloctor_reset(bump_allocator* bump_alloc);