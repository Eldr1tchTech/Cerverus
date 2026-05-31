#include "bump_allocator.h"

#include "core/memory/cmem.h"
#include "core/util/logger.h"

bump_allocator* bump_allocator_create(size_t size) {
    bump_allocator* bump_alloc = cmem_alloc(memory_tag_unknown, sizeof(bump_alloc));
    bump_alloc->data = cmem_alloc(memory_tag_unknown, size);    // NOTE: Should I round up to the nearest multiple of 8, so taht it's on a byte boundary?
    bump_alloc->cursor = bump_alloc->data;
    bump_alloc->size = size;

    return bump_alloc;
}

void bump_allocator_destroy(bump_allocator* bump_alloc) {
    cmem_free(memory_tag_unknown, bump_alloc->data);
    cmem_free(memory_tag_unknown, bump_alloc);
}

void* bump_allocator_alloc(bump_allocator* bump_alloc, size_t size) {
    if (bump_alloc->cursor + size > bump_alloc->data + bump_alloc->size)
    {
        LOG_ERROR("bump_allocator_alloc - Not enough memory available.");
        return NULL;
    }
    void* r = bump_alloc->cursor;
    bump_alloc->cursor += size;
    return r;
}

void bump_alloctor_reset(bump_allocator* bump_alloc) {
    cmem_zmem(bump_alloc->data, bump_alloc->size);
    bump_alloc->cursor = bump_alloc->data;
}