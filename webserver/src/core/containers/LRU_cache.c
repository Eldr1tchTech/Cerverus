#include "LRU_cache.h"

#include "core/memory/cmem.h"

typedef struct LRU_cache_entry
{
    void* hmap_entry;
    void* data;
} LRU_cache_entry;


LRU_cache* LRU_cache_create(size_t size, size_t stride, eviction_handler evic_handler) {
    LRU_cache* cache = cmem_alloc(memory_tag_unknown, sizeof(LRU_cache));
    cache->size = size; // TODO: do we even need these?
    cache->stride = stride;
    cache->dll = doubly_linked_list_create(stride);
    cache->hmap = hashmap_create(size, 0.67, sizeof(doubly_linked_list_node*), NULL);
    cache->evic_handler = evic_handler;
}

void LRU_cache_destroy(LRU_cache* cache) {
    hashmap_destroy(cache->hmap);
    doubly_linked_list_destroy(cache->dll);
    cmem_free(memory_tag_unknown, cache);
}

void* LRU_cache_get(LRU_cache* cache, char* label) {
    return ((doubly_linked_list_node*)hashmap_get(cache->hmap, label))->data;
}

void LRU_cache_add(LRU_cache* cache, char* label, void* item) {
    doubly_linked_list_node* value = hashmap_get(cache->hmap, label);
    if (value) {
        cmem_mcpy(value->data, item, cache->stride);
        doubly_linked_list_push_node_to_front(cache->dll, value);
        return;
    }

    if (cache->dll->length == cache->size)
    {
        doubly_linked_list_pop_back(cache->dll, NULL); // TODO: Add a wrapper to the content since you don't know what hashmap entry corresponds to which dll entry.
    }
    
}