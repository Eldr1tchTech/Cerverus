#include "LRU_cache.h"

#include "core/containers/doubly_linked_list.h"
#include "core/containers/hashmap.h"
#include "core/memory/cmem.h"

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

void LRU_cache_add(LRU_cache *cache, char *label, void *item) {}
