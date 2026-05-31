#include "pool_allocator.h"

#include "core/memory/cmem.h"
#include "core/util/bitwise.h"
#include "core/util/logger.h"

pool_allocator* pool_allocator_create(size_t stride, size_t size) {
    pool_allocator* pool_alloc = cmem_alloc(memory_tag_unknown, sizeof(pool_allocator));
    pool_alloc->availability = bitarray_create(size);
    pool_alloc->stride = stride;
    pool_alloc->data = cmem_alloc(memory_tag_unknown, pool_alloc->availability->size * stride);

    return pool_alloc;
}

void pool_allocator_destroy(pool_allocator* pool_alloc) {
    cmem_free(memory_tag_unknown, pool_alloc->data);
    bitarray_destroy(pool_alloc->availability);
    cmem_free(memory_tag_unknown, pool_alloc);
}

void* pool_allocator_alloc(pool_allocator* pool_alloc) {
    for (size_t i = 0; i < pool_alloc->availability->size; i++)
    {
        if (bitwise_get(pool_alloc->availability, i)) continue;
        bitwise_set(pool_alloc->availability, i);
        return pool_alloc->data + pool_alloc->stride * i;
    }
    return NULL;
}

void pool_allocator_free(pool_allocator* pool_alloc, void* block) {
    if (block < pool_alloc->data || block > pool_alloc->data + pool_alloc->stride * (pool_alloc->availability->size - 1))
    {
        LOG_ERROR("pool_allocator_free - Attempted to free out of bounds memory.");
        return;
    }
    cmem_zmem(block, pool_alloc->stride);
    size_t index = (block - pool_alloc->data) / pool_alloc->stride;
    bitwise_clear(pool_alloc->availability, index);
}