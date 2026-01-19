#include "cmem.h"

#include <string.h>

// TODO: Add proper tracking for cmem_stats
void* cmem_alloc(memory_tag mem_tag, size_t size) {
    return malloc(size);
}

void cmem_free(memory_tag mem_tag, void* block) {
    free(block);
}

void cmem_zmem(void* block, size_t size) {
    memset(block, 0, size);
}

void cmem_mcpy(void* dest, void* source, size_t size) {
    memcpy(dest, source, size);
}