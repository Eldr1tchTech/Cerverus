#include "cmem.h"

#include "core/util/logger.h"

#include <string.h>

#define X_TAG(name, tag) tag,
const char* memory_tags[] = {
    MEMORY_TAGS(X_TAG)
};
#undef X_TAG

cmem_state state;

// TODO: Add proper tracking for cmem_stats
void* cmem_alloc(memory_tag mem_tag, size_t size) {
    state.allocated[mem_tag] += size;
    state.allocations[mem_tag]++;
    void* block = malloc(size);
    cmem_zmem(block, size);
    return block;
}

void cmem_free(memory_tag mem_tag, void* block) {
    state.allocated[mem_tag] -= sizeof(block);
    free(block);
}

void cmem_zmem(void* block, size_t size) {
    memset(block, 0, size);
}

void cmem_mcpy(void* dest, void* src, size_t size) {
    memcpy(dest, src, size);
}

void cmem_print_stats() {
    LOG_INFO("==== Memory Stats ====");

    LOG_INFO("---- Allocated Size ----");
    for (int i = 0; i < memory_tag_max_tags; i++)
    {
        LOG_INFO("%s: %i", memory_tags[i], state.allocated[i]);
    }
    
    LOG_INFO("---- Allocations ----");
    for (int i = 0; i < memory_tag_max_tags; i++)
    {
        LOG_INFO("%s: %i", memory_tags[i], state.allocations[i]);
    }
}