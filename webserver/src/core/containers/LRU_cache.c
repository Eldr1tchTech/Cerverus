#include "LRU_cache.h"

#include "core/memory/cmem.h"

#include <string.h>

typedef struct LRU_cache_entry
{
    char* key;     // heap copy, owned by this node, freed on evict/destroy/overwrite
    char value[];  // `stride` bytes of caller data
} LRU_cache_entry;

static void LRU_cache_evict_oldest(LRU_cache* cache)
{
    size_t entry_size = sizeof(LRU_cache_entry) + cache->stride;
    char evicted_buf[entry_size];

    doubly_linked_list_pop_back(cache->dll, evicted_buf);
    LRU_cache_entry* evicted = (LRU_cache_entry*)evicted_buf;

    if (cache->evic_handler)
    {
        cache->evic_handler(evicted->value);
    }

    hashmap_delete(cache->hmap, evicted->key);
    cmem_free(memory_tag_string, evicted->key);
}

LRU_cache* LRU_cache_create(size_t size, size_t stride, eviction_handler evic_handler) {
    LRU_cache* cache = cmem_alloc(memory_tag_unknown, sizeof(LRU_cache));
    cache->size = size;
    cache->stride = stride;
    cache->dll = doubly_linked_list_create(sizeof(LRU_cache_entry) + stride);
    cache->hmap = hashmap_create(size, 0.67, sizeof(doubly_linked_list_node*), NULL);
    cache->evic_handler = evic_handler;

    return cache;
}

void LRU_cache_destroy(LRU_cache* cache) {
    // Walk the dll, not the hashmap, so each live value gets exactly one
    // evic_handler call and each key copy gets freed.
    doubly_linked_list_node* curr_node = cache->dll->head;
    while (curr_node)
    {
        LRU_cache_entry* entry = (LRU_cache_entry*)curr_node->data;

        if (cache->evic_handler)
        {
            cache->evic_handler(entry->value);
        }

        cmem_free(memory_tag_string, entry->key);
        curr_node = curr_node->next;
    }

    hashmap_destroy(cache->hmap);
    doubly_linked_list_destroy(cache->dll);
    cmem_free(memory_tag_unknown, cache);
}

void* LRU_cache_get(LRU_cache* cache, char* label) {
    void* slot = hashmap_get(cache->hmap, label);
    if (!slot)
    {
        return NULL; // cache miss
    }

    doubly_linked_list_node* node = *(doubly_linked_list_node**)slot;
    doubly_linked_list_move_to_front(cache->dll, node);

    LRU_cache_entry* entry = (LRU_cache_entry*)node->data;
    return entry->value;
}

void LRU_cache_add(LRU_cache* cache, char* label, void* item) {
    void* slot = hashmap_get(cache->hmap, label);
    if (slot)
    {
        doubly_linked_list_node* node = *(doubly_linked_list_node**)slot;
        LRU_cache_entry* entry = (LRU_cache_entry*)node->data;

        // Overwriting an existing slot's payload — let the caller clean up
        // the old value (e.g. close the previous fd) before it's clobbered.
        if (cache->evic_handler)
        {
            cache->evic_handler(entry->value);
        }

        cmem_mcpy(entry->value, item, cache->stride);
        doubly_linked_list_move_to_front(cache->dll, node);
        return;
    }

    if (cache->dll->length == cache->size)
    {
        LRU_cache_evict_oldest(cache);
    }

    size_t entry_size = sizeof(LRU_cache_entry) + cache->stride;
    char new_entry_buf[entry_size];
    LRU_cache_entry* new_entry = (LRU_cache_entry*)new_entry_buf;

    new_entry->key = cmem_alloc(memory_tag_string, strlen(label) + 1);
    strcpy(new_entry->key, label);
    cmem_mcpy(new_entry->value, item, cache->stride);

    // push_front always leaves the new node as dll->head, so grab it from
    // there afterward to store in the hashmap.
    doubly_linked_list_push_front(cache->dll, new_entry);
    doubly_linked_list_node* new_node = cache->dll->head;

    hashmap_set(cache->hmap, label, &new_node);
}

void LRU_cache_rehash(LRU_cache* cache) {
    cache->hmap = hashmap_rehash(cache->hmap);
}