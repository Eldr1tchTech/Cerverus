#include "LRU_cache.h"

#include "core/containers/doubly_linked_list.h"
#include "core/containers/hashmap.h"
#include "core/memory/cmem.h"
#include "core/util/logger.h"
#include <string.h>

typedef struct dll_entry {
  char *hmap_key;
  char data[];
} dll_entry;

LRU_cache *LRU_cache_create(size_t size, size_t stride,
                            eviction_handler evic_handler) {
  LRU_cache *cache = cmem_alloc(sizeof(LRU_cache));
  cache->evic_handler = evic_handler;
  cache->size = size;
  cache->stride = stride;
  cache->hmap =
      hashmap_create(size, .67, sizeof(doubly_linked_list_node *), NULL);
  cache->dll = doubly_linked_list_create(sizeof(dll_entry) + stride);

  return cache;
}
void LRU_cache_destroy(LRU_cache *cache) {
  doubly_linked_list_destroy(cache->dll);
  hashmap_destroy(cache->hmap);
  cmem_free(cache);
}

void *LRU_cache_get(LRU_cache *cache, char *label) {
  // Check hashmap, hashmap stores pointer to dll, dll cotnains data, move entry
  // to front
  doubly_linked_list_node *item = hashmap_get(cache->hmap, label);
  if (item == NULL) {
    return NULL;
  }
  doubly_linked_list_move_to_front(cache->dll, item);

  return ((dll_entry *)item->data)->data;
}

void LRU_cache_add(LRU_cache *cache, char *label, void *item) {
  if (hashmap_get(cache->hmap, label) != NULL) {
    LOG_DEBUG("LRU_cache_add - label is already in use.");
  }

  dll_entry *new_entry = cmem_alloc(sizeof(dll_entry) + cache->stride);
  new_entry->hmap_key = strdup(label);
  cmem_mcpy(new_entry->data, item, cache->stride);

  doubly_linked_list_node *new_node =
      doubly_linked_list_push_front(cache->dll, new_entry);

  cmem_free(new_entry);

  if (cache->dll->length > cache->size) {
    dll_entry *evicted_entry = cmem_alloc(sizeof(dll_entry) + cache->stride);
    doubly_linked_list_pop_tail(cache->dll, evicted_entry);
    cmem_free(evicted_entry->hmap_key);
    cache->evic_handler(evicted_entry->data);
    cmem_free(evicted_entry);
  }

  hashmap_set(cache->hmap, label, new_node);
}
